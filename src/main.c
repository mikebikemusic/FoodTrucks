#include <pebble.h>

#define dbg false
static Window *menuWindow;
static TextLayer *text_layer;


static void menuWindow_load(Window *menuWindow) {
	Layer *window_layer = window_get_root_layer(menuWindow);
	GRect bounds = layer_get_frame(window_layer);
	text_layer = text_layer_create(bounds);
	text_layer_set_text(text_layer, "Food Trucks test. Waiting for javascript.");
	text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(text_layer));
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
	const bool animated = true;
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
