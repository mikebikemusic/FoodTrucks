#include <pebble.h>

#define dbg false
#define unitTest false
#define msgTest false
#define newRetry true
#define useRetryTimer false

#define MAX_MENU_ITEMS 100
#define MAX_ANNOUNCE 2200
const int FIRST_DETAIL = 1000;
const int RETRY_COUNT = 70;
const bool animated = true;
static Window *menuWindow;
static TextLayer *text_layer;

static int truckCount;
static time_t endTime;
static char *details;
static char *announcement;
static char blank[] = "";
static char wait[] = "wait";

static SimpleMenuLayer *simple_menu_layer;
static SimpleMenuSection menu_section;
static SimpleMenuItem menu_items[MAX_MENU_ITEMS];
static int selected;

enum { KEY_COUNT, KEY_INDEX, KEY_TITLE, KEY_SUBTITLE, KEY_ENDTIME, KEY_DETAILS};

bool noTrucks;
int fetched;
int pending;
bool menuRemoved;
bool menuCreated;
int menuItemSet;
bool refreshing;
static AppTimer *retry_timer = NULL;
static int retryCount = 0;
static void fetch_msg(int index);

char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

static void retryCallback(void* data) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Retry requesting index %d from timer callback", fetched);
	fetch_msg(fetched);
}

static void out_sent_handler(DictionaryIterator *sent, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "out_sent_handler %d was acked", pending);
	if (retryCount != 0) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Retry requesting index %d from sent_handler callback", fetched);
		fetch_msg(fetched);
	}
}

static void fetch_msg(int index) {
	fetched = index;
#if !unitTest
	DictionaryIterator *iter; // = &dict;

	for (int i = 1; i <= RETRY_COUNT; ++i) {
		AppMessageResult result = app_message_outbox_begin(&iter);
		if (result == APP_MSG_OK) {
			retryCount = 0;
			pending = index;
			break;
		}
		// ****** start of new retry logic
		if (newRetry) {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_outbox_begin error %s", translate_error(result));
			if (retryCount == 0)
				retryCount = RETRY_COUNT;
			--retryCount;
			if (useRetryTimer && retryCount > 1 && retryCount < RETRY_COUNT - 1) {
				APP_LOG(APP_LOG_LEVEL_DEBUG, "Looks like we still need a timer");
				retry_timer = app_timer_register(1, retryCallback, NULL);				
			}
			return;
		}
		// ***** end of new retry logic. Start of old retry logic:
		if(dbg && i == 1)APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_outbox_begin error %s", translate_error(result));

		if (i == RETRY_COUNT) {
			if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_outbox_begin error %s", translate_error(result));
			return;
		}
		psleep(i);
	}
	for (int i = 1; i <= RETRY_COUNT; ++i) {
		Tuplet value = TupletInteger(KEY_INDEX, index);
		DictionaryResult result = dict_write_tuplet(iter, &value);

		//DictionaryResult result = dict_write_int(iter, KEY_INDEX, &index, 2, true);

		if (result == DICT_OK) 
			break;
		if (i == RETRY_COUNT) {
			if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "dict_write_uint8 error %d", result);
			return;
		}
		psleep(i);
	}
	dict_write_end(iter);
	AppMessageResult result = app_message_outbox_send();
	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_outbox_send result = %d", result);

	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG,"fetch_msg requesting %d of %d", index, truckCount);
#endif
}

static void menu_select_callback(int index, void *ctx) {
	selected = index;	
	if (strcmp(menu_items[index].subtitle, "wait") == 0) {
		fetch_msg(selected);
	} else {
		fetch_msg(selected + FIRST_DETAIL);
	}
}

static void replace_menu_item(void** item, void* str) {
	if (*item != str) {
		if (*item != blank && *item != wait) {
			free(*item);
		}
		*item = str;
	}
}

static void remove_menu() {
	for (int i = 0; i < MAX_MENU_ITEMS; i++) {
		replace_menu_item((void**)&(menu_items[i].title), blank);
		replace_menu_item((void**)&(menu_items[i].subtitle), wait);
	}
	if (simple_menu_layer != NULL) {
		layer_remove_from_parent(simple_menu_layer_get_layer(simple_menu_layer));
		simple_menu_layer_destroy(simple_menu_layer);
		simple_menu_layer = NULL;
	}
	menuRemoved = true;
}

static void create_menu() {
	text_layer_set_text(text_layer, "");
	menu_section = (SimpleMenuSection){
		.num_items = truckCount,
		.items = menu_items,
	};
	Layer *window_layer = window_get_root_layer(menuWindow);
	GRect bounds = layer_get_frame(window_layer);
	simple_menu_layer = simple_menu_layer_create(bounds, menuWindow, &menu_section, 1, NULL);
	layer_add_child(window_layer, simple_menu_layer_get_layer(simple_menu_layer));
	selected = 0;
	simple_menu_layer_set_selected_index(simple_menu_layer, selected, true);
	menuCreated = true;
}

static void build_menu(int count) {
	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG,"build_menu with %d itmes", count);
	if (truckCount == count) {
		//app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
		fetch_msg(0); // reuse existing menu and ask for first truck
		return;
	}
	truckCount = count;
	remove_menu();		
	if (truckCount > 0) {
		noTrucks = false;
		create_menu();			
		fetch_msg(0); // ask for first truck
	} else {
		noTrucks = true;
		text_layer_set_text(text_layer, "No food trucks are currently open for business. Try again later.");
	}
}

static void set_menu_item(int index, char *ttl, char *sub) {
	menuItemSet = index;
#if msgTest
	static char pass;
	if (index == 0)
		pass++;
	snprintf(announcement, MAX_ANNOUNCE, "Pass %d\nReceived %d:\n%s\n%s", pass, index, ttl, sub);
	text_layer_set_text(text_layer, announcement);
#endif
	replace_menu_item((void**)&(menu_items[index].title), ttl);
	replace_menu_item((void**)&(menu_items[index].subtitle), sub);
	if (simple_menu_layer != NULL) {
		menu_layer_reload_data(simple_menu_layer_get_menu_layer(simple_menu_layer));
	}
	if (index + 1 < truckCount) {
		fetch_msg(index + 1); // ask for next truck
	} else {
		refreshing = false;
		//app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
	}
#if msgTest
	// Stress test infinite loop. Sending the truck count causes the JS to rebuild the truck list and re-send the count.
	if (index + 1 == truckCount) {
		fetch_msg(index + 1);
	}	
#endif
}

//******** start scroll window code

static Window *scroll_window;
static ScrollLayer *scroll_layer;
static TextLayer *scroll_text_layer;
static const int vert_scroll_text_padding = 4;
static GRect scroll_bounds;
bool announcing;

void update_announcement() {
	text_layer_set_text(scroll_text_layer, announcement);
	GSize max_size = text_layer_get_content_size(scroll_text_layer);
	text_layer_set_size(scroll_text_layer, max_size);
	scroll_layer_set_content_size(scroll_layer, GSize(scroll_bounds.size.w, max_size.h + vert_scroll_text_padding));
	scroll_layer_set_content_offset(scroll_layer, GPointZero, !animated); // 0.2 Returning to scrolled details did not go back to top
}

static void scroll_window_load(Window *window) {
	announcing = true;
}

static void scroll_window_unload(Window *wind) {
	announcing = false;
}

static void scroll_window_create(Window *window) {
	announcement = malloc(MAX_ANNOUNCE + 1);
	scroll_window = window_create();
	window_set_window_handlers(scroll_window, (WindowHandlers) {
    		.load = scroll_window_load,
		.unload = scroll_window_unload,
	});	

	Layer *window_layer = window_get_root_layer(scroll_window);
	scroll_bounds = layer_get_frame(window_layer);
	scroll_layer = scroll_layer_create(scroll_bounds);
	scroll_layer_set_click_config_onto_window(scroll_layer, scroll_window);
	
	GRect max_text_bounds = GRect(0, 0, scroll_bounds.size.w, 3000);
	scroll_text_layer = text_layer_create(max_text_bounds);
	text_layer_set_font(scroll_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	scroll_layer_add_child(scroll_layer, text_layer_get_layer(scroll_text_layer));
	layer_add_child(window_layer, scroll_layer_get_layer(scroll_layer));
}

static void scroll_window_destroy() {	
	text_layer_destroy(scroll_text_layer);
	scroll_layer_destroy(scroll_layer);
	window_destroy(scroll_window);
	free(announcement);
}

static void announce() {
	char time_text[20];
	strftime(time_text, sizeof(time_text), "%l:%M%P", localtime(&endTime));
	snprintf(announcement, MAX_ANNOUNCE, "%s\nuntil %s at %s\n%s", 
		menu_items[selected].title, time_text, menu_items[selected].subtitle, details);
	free(details);
	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "announcement: %s", announcement);
	if (!announcing) { // 0.2 prevents rapid press of select from creating multiple scroll windows
 		window_stack_push(scroll_window, animated);
	}
	update_announcement();
}

//******** end scroll window code

static void in_received_handler(DictionaryIterator *iter, void *context) {
	static char *title;
	static char *subtitle;
	int index = -1;
	Tuple *tuple;

	endTime = 0;
	int count = -1;
	tuple = dict_read_first(iter);
	while (tuple != NULL) {
		if (tuple->type == TUPLE_CSTRING) {
			//if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG,"in_received_handler string key %ld: %s", tuple->key, tuple->value->cstring);
			switch (tuple->key) {
				case KEY_TITLE:
					title = malloc(strlen(tuple->value->cstring) + 1);
					strcpy(title, tuple->value->cstring);
					break;
				case KEY_SUBTITLE:
					subtitle = malloc(strlen(tuple->value->cstring) + 1);
					strcpy(subtitle, tuple->value->cstring);
					break;
				case KEY_DETAILS:
					details = malloc(strlen(tuple->value->cstring) + 1);
					strcpy(details, tuple->value->cstring);
					break;
				default:
					break;
			}
		} else if (tuple->type == TUPLE_INT) {
			//if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG,"in_received_handler int key %ld = %ld", tuple->key, tuple->value->int32);
			switch (tuple->key) {
				case KEY_COUNT:
					if (announcing && !refreshing) { // 0.2 scroll window was not getting freed when selecting a different city
						window_stack_pop(true);						
					}
					count = tuple->value->int32;
					build_menu(count);		
					count = -1;
					break;
				case KEY_INDEX:
					index = tuple->value->int32;
					break;
				case KEY_ENDTIME:
					endTime = tuple->value->int32;
					break;
				default:
					break;
			}
		} else {
			if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG,"in_received_handler key %ld type %d", tuple->key, tuple->type);
		}
		tuple = dict_read_next(iter);
	}
	if (count > 0) {
		build_menu(count);		
	}
	if (index >= 0) {
		set_menu_item(index, title, subtitle);
	}
	if (endTime > 0 && simple_menu_layer != NULL) {
		announce();
	}
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
	int max = app_message_inbox_size_maximum();
	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped! %d, max=%d", reason, max);
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send!");
}

#if unitTest
static int passCount;
static int failCount;
static char results[200];
static void assert(bool assertion, char* failMsg) {
	if (assertion) {
		passCount++;
	} else {
		failCount++;
		if (failCount <= 5)
			strcat(results, failMsg);
	}
}

static void test_in_received_handler(int key, int value, char* str) {
	static char title[40];
	static char subtitle[80];
	int index = -1;
	
	switch (key) {
		case KEY_COUNT: // js sends number of trucks n
			build_menu(value);
			break;
		case KEY_INDEX: // js sends truck 0 to n-1 with title and subtitle
			index = value;
			break;
		case KEY_TITLE:
			strcpy(title, str);
			break;
		case KEY_SUBTITLE:
			strcpy(subtitle, str);
			break;
		case KEY_ENDTIME: // js sends truck endtime and details
			endTime = value;
			break;
		case KEY_DETAILS:
			strcpy(details, str);
			break;
		default:
			break;
	}
	if (index >= 0) {
		set_menu_item(index, title, subtitle);
	}
	if (endTime > 0) {
		announce();
	}
}

static void test() {
	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "unittest");
	fetched = -1;
	test_in_received_handler(KEY_COUNT, 0, NULL); // Test no trucks are available
	assert(noTrucks, "F No Trucks 1\n"); noTrucks = false;
	assert(menuRemoved, "F No Trucks 2\n"); menuRemoved = false;
	assert(!menuCreated, "F No Trucks 3\n"); menuCreated = false;
	assert(truckCount == 0, "F No Trucks 4\n");
	assert(fetched == -1, "F No Trucks 5\n"); fetched = -1;
	
	test_in_received_handler(KEY_COUNT, 1, NULL); // Test 1 truck
	assert(!noTrucks, "F 1 Truck 1\n"); noTrucks = false;
	assert(menuRemoved, "F 1 Truck 2\n"); menuRemoved = false;
	assert(menuCreated, "F 1 Truck 3\n"); menuCreated = false;
	assert(truckCount == 1, "F 1 Truck 4\n");
	assert(fetched == 0, "F 1 Truck 5\n"); fetched = -1;
	//
	test_in_received_handler(KEY_TITLE, 0, "Only Truck");
	test_in_received_handler(KEY_SUBTITLE, 0, "Only Location");
	test_in_received_handler(KEY_INDEX, 0, NULL);
	assert(fetched == -1, "F 1 Truck 6\n"); fetched = -1;
	assert(strcmp(menu_items[menuItemSet].title, "Only Truck") == 0, "F 1 Truck 7\n");
	assert(strcmp(menu_items[menuItemSet].subtitle, "Only Location") == 0, "F 1 Truck 8\n");
	
	test_in_received_handler(KEY_COUNT, 10, NULL);// Test 10 trucks
	assert(!noTrucks, "F 10 Trucks 1\n"); noTrucks = false;
	assert(menuRemoved, "F 10 Trucks 2\n"); menuRemoved = false;
	assert(menuCreated, "F 10 Trucks 3\n"); menuCreated = false;
	assert(truckCount == 10, "F 10 Trucks 4\n");
	assert(fetched == 0, "F 10 Trucks 5\n"); fetched = -1;
	for (int i= 0; i<10; ++i) {
		char ttl[40];
		char sub[80];
		snprintf(ttl, sizeof(ttl), "Truck %d", i);
		snprintf(sub,  sizeof(sub),"location %d", i);
		test_in_received_handler(KEY_TITLE, 0, ttl);
		test_in_received_handler(KEY_SUBTITLE, 0, sub);
		test_in_received_handler(KEY_INDEX, i, NULL);
		assert(fetched == (i<9 ? i+1 : -1), "F 10 Trucks 6\n"); fetched = -1;
		assert(strcmp(menu_items[menuItemSet].title, ttl) == 0, "F 10 Trucks 7\n");
		assert(strcmp(menu_items[menuItemSet].subtitle, sub) == 0, "F 1 0Truck 8\n");
	}
	
	menu_select_callback(4, NULL);
	assert(selected == 4, "F Details 1\n");
	assert(fetched == 4+FIRST_DETAIL, "F Details 2\n");
	test_in_received_handler(KEY_DETAILS, 0, "Details"); // Test sending details
	test_in_received_handler(KEY_ENDTIME, 1407873600, NULL); // Test sending endtime
	assert(strcmp(details, "Details") == 0, "F Details 3\n");
	assert(strcmp(announcement, "Truck 4\nuntil  8:00pm at location 4\nDetails") == 0, "F Details 4\n");
	
	char final[80];
	snprintf(final, sizeof(final), "%d Failed, %d Passed", failCount, passCount);
	strcat(results, final);
	text_layer_set_text(text_layer, results);
	
}
#endif

static void handle_tick(struct tm* now, TimeUnits units_changed){
 	// New trucks open typically every half hour. Add 1 minute to allow for js time variation.
	// Also, avoid the rare time when the tick occurs while creating a menu (fetched is below truckCount)
	if ((now->tm_min == 0 || now->tm_min == 30) && fetched + 1 >= truckCount) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Half Hour Update");
		refreshing = true;
		fetch_msg(-1);
	}
}

static void menuWindow_load(Window *menuWindow) {
	Layer *window_layer = window_get_root_layer(menuWindow);
	GRect bounds = layer_get_frame(window_layer);
	text_layer = text_layer_create(bounds);
	// 0.2 replace test message with final messaging
	text_layer_set_text(text_layer, "If you don't see the food truck list in a few seconds, please start your Pebble App from your phone.");
	text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(text_layer));
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
#if unitTest
	test();
#endif
}

static void menuWindow_unload(Window *window) {
	remove_menu();
	layer_remove_from_parent(text_layer_get_layer(text_layer));
	text_layer_destroy(text_layer);
}

static void handle_init(void) {
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_failed(out_failed_handler);
	app_message_register_outbox_sent(out_sent_handler);
	const uint32_t inbound_size = app_message_inbox_size_maximum();
	const uint32_t outbound_size = app_message_outbox_size_maximum();
	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_inbox_size %d", (int)inbound_size);

	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_outbox_size %d", (int)outbound_size);

	app_message_open(inbound_size, outbound_size);

	for (int i = 0; i < MAX_MENU_ITEMS; i++) {
		menu_items[i] = (SimpleMenuItem){
			.title = blank,
			.subtitle = wait,
    		.callback = menu_select_callback,
		};
	}
	menuWindow = window_create();
	window_set_window_handlers(menuWindow, (WindowHandlers) {
		.load = menuWindow_load,
		.unload = menuWindow_unload,
	});
	window_stack_push(menuWindow, animated);
	
	scroll_window_create(menuWindow);
	tick_timer_service_subscribe(MINUTE_UNIT, &handle_tick);
}

static void handle_deinit(void) {
  	tick_timer_service_unsubscribe();
	scroll_window_destroy();
	app_message_deregister_callbacks();
	remove_menu();
	window_destroy(menuWindow);
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}
