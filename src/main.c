#include <pebble.h>

#define dbg true
#define unitTest true

const bool animated = true;
static Window *menuWindow;
static TextLayer *text_layer;

static int truckCount;
static time_t endTime;
char details[1000];


enum { KEY_COUNT, KEY_INDEX, KEY_TITLE, KEY_SUBTITLE, KEY_ENDTIME, KEY_DETAILS};


static void fetch_msg(int index) {
}

static void remove_menu() {
}

static void create_menu() {
}

static void build_menu(int count) {
	truckCount = count;
	remove_menu();
	if (truckCount > 0) {
		create_menu();
		fetch_msg(0); // ask for first truck
	} else {
		if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "no trucks");
		text_layer_set_text(text_layer, "No food trucks are currently open for business. Try again later.");
	}
}

static void set_menu_item(int index, char *ttl, char *sub) {
	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "set_menu_item %d, %s, %s", index, ttl, sub);
	if (index + 1 < truckCount) {
		fetch_msg(index + 1);
	}	
}

static void announce() {
	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "announce ends %ld, %s", endTime, details);
}

#if unitTest
static char title[40];
static char subtitle[80];
static void test_in_received_handler(int key, int value, char* str) {
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
		set_menu_item(value, title, subtitle);
	}
	if (endTime > 0) {
		announce();
	}
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
	if(dbg)APP_LOG(APP_LOG_LEVEL_DEBUG, "unittest");
	//test_in_received_handler(KEY_COUNT, 0, NULL); // Test no trucks are available
	
	//test_in_received_handler(KEY_COUNT, 1, NULL); // Test 1 truck
	//test_in_received_handler(KEY_TITLE, 0, "Only Truck");
	//test_in_received_handler(KEY_SUBTITLE, 0, "Only Location");
	//test_in_received_handler(KEY_INDEX, 0, NULL);
	
	test_in_received_handler(KEY_COUNT, 10, NULL);// Test 10 trucks
	for (int i= 0; i<10; ++i) {
		char ttl[40];
		char sub[80];
		snprintf(ttl, sizeof(ttl), "Truck %d", i);
		snprintf(sub,  sizeof(sub),"location %d", i);
		test_in_received_handler(KEY_TITLE, 0, ttl);
		test_in_received_handler(KEY_SUBTITLE, 0, sub);
		test_in_received_handler(KEY_INDEX, i, NULL);
	}
	test_in_received_handler(KEY_DETAILS, 0, "Details"); // Test sending details
	test_in_received_handler(KEY_ENDTIME, 1408161600, NULL); // Test sending endtime
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
}

static void handle_deinit(void) {
	window_destroy(menuWindow);
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}
