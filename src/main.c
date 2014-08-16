#include <pebble.h>

#define dbg false
#define unitTest false
#define msgTest true

#define MAX_MENU_ITEMS 50
const int FIRST_DETAIL = 1000;
const bool animated = true;
static Window *menuWindow;
static TextLayer *text_layer;

static int truckCount;
static time_t endTime;
char details[1000];
static char announcement[2000];

static char title[MAX_MENU_ITEMS][40];
static char subtitle[MAX_MENU_ITEMS][80];
static int selected;

enum { KEY_COUNT, KEY_INDEX, KEY_TITLE, KEY_SUBTITLE, KEY_ENDTIME, KEY_DETAILS};

bool noTrucks;
int fetched;
bool menuRemoved;
bool menuCreated;
int menuItemSet;

static void fetch_msg(int index) {
	fetched = index;
#if !unitTest
	if(msgTest)APP_LOG(APP_LOG_LEVEL_DEBUG,"fetch_msg start %d", index);
	if(msgTest)psleep(100);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	if (iter == NULL)
		return;
	dict_write_uint8(iter, KEY_INDEX, index);
	app_message_outbox_send();
	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG,"fetch_msg requesting %d of %d", index, truckCount);
#endif
}

static void menu_select_callback(int index, void *ctx) {
	selected = index;	
	fetch_msg(selected + FIRST_DETAIL);
}

static void remove_menu() {
	menuRemoved = true;
}

static void create_menu() {
	menuCreated = true;
}

static void build_menu(int count) {
	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG,"build_menu with %d itmes", count);
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
	snprintf(announcement, sizeof(announcement), "Received %d: %s, %s", index, ttl, sub);
	text_layer_set_text(text_layer, announcement);
#endif
	strcpy(title[index], ttl);
	strcpy(subtitle[index], sub);
	if (index + 1 < truckCount) {
		fetch_msg(index + 1); // ask for next truck
	}	
#if msgTest
	// Stress test infinite loop. Sending the truck count causes the JS to rebuild the truck list and re-send the count.
	if (index + 1 == truckCount) {
		fetch_msg(index + 1);
	}	
#endif
}

static void announce() {
	char time_text[20];
	strftime(time_text, sizeof(time_text), "%l:%M%P", localtime(&endTime));
	snprintf(announcement, sizeof(announcement) - 1, "%s\nuntil %s at %s\n%s", 
		title[selected], time_text, subtitle[selected], details);
	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "announcement: %s", announcement);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
	static char title[40];
	static char subtitle[80];
	int index = -1;
	Tuple *tuple;

	tuple = dict_read_first(iter);
	while (tuple != NULL) {
		if (tuple->type == TUPLE_CSTRING) {
			if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG,"in_received_handler string key %ld: %s", tuple->key, tuple->value->cstring);
			switch (tuple->key) {
				case KEY_TITLE:
					strcpy(title, tuple->value->cstring);
					break;
				case KEY_SUBTITLE:
					strcpy(subtitle, tuple->value->cstring);
					break;
				default:
					break;
			}
		} else if (tuple->type == TUPLE_INT) {
			if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG,"in_received_handler int key %ld = %ld", tuple->key, tuple->value->int32);
			switch (tuple->key) {
				case KEY_COUNT:
					build_menu(tuple->value->int32);
					break;
				case KEY_INDEX:
					index = tuple->value->int32;
					break;
				default:
					break;
			}
		} else {
			if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG,"in_received_handler key %ld type %d", tuple->key, tuple->type);
		}
		tuple = dict_read_next(iter);
	}
	if (index >= 0) {
		set_menu_item(index, title, subtitle);
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
	assert(strcmp(title[menuItemSet], "Only Truck") == 0, "F 1 Truck 7\n");
	assert(strcmp(subtitle[menuItemSet], "Only Location") == 0, "F 1 Truck 8\n");
	
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
		assert(strcmp(title[menuItemSet], ttl) == 0, "F 10 Trucks 7\n");
		assert(strcmp(subtitle[menuItemSet], sub) == 0, "F 1 0Truck 8\n");
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

static void menuWindow_load(Window *menuWindow) {
	Layer *window_layer = window_get_root_layer(menuWindow);
	GRect bounds = layer_get_frame(window_layer);
	text_layer = text_layer_create(bounds);
	text_layer_set_text(text_layer, "Food Trucks test. Waiting for javascript.");
	text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(text_layer));
#if unitTest
	test();
#endif
}

static void menuWindow_unload(Window *window) {
	text_layer_destroy(text_layer);
}

static void handle_init(void) {
	menuWindow = window_create();
	window_set_window_handlers(menuWindow, (WindowHandlers) {
		.load = menuWindow_load,
		.unload = menuWindow_unload,
	});
	window_stack_push(menuWindow, animated);
	
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_failed(out_failed_handler);
	const uint32_t inbound_size = app_message_inbox_size_maximum();
	const uint32_t outbound_size = app_message_outbox_size_maximum();
	app_message_open(inbound_size, outbound_size);
}

static void handle_deinit(void) {
	window_destroy(menuWindow);
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}
