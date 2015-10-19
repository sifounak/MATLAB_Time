#include <pebble.h>

#define KEY_TEXT_COLOR 0
#define KEY_INVERT_COLORS 1
#define KEY_TEMPERATURE 2
#define KEY_TEMPERATURE_IN_C 3
#define KEY_CONDITIONS 4
#define KEY_SHAKE_FOR_WEATHER 5
#define KEY_USE_CELSIUS 6
#define KEY_BACKGROUND_COLOR 7
#define KEY_SHOW_WEATHER 8
#define KEY_VIBE_ON_DISCONNECT 9
#define KEY_VIBE_ON_CONNECT 10
#define KEY_REFLECT_BATT 12
	
static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer, *s_charge_layer, *s_bluetooth_layer, *s_temp_layer, *s_conditions_layer, *s_temp_layer_unanimated, *s_conditions_layer_unanimated;
static GFont s_time_font, s_date_font, s_weather_font;
static Layer *s_batt_layer, *s_static_layer, *s_weather_layer, *s_weather_layer_unanimated;
static bool invert_colors = 0;
static bool use_celsius = 0;
static bool shake_for_weather = 0;
static bool show_weather = 1;
static bool vibe_on_disconnect = 1;
static bool vibe_on_connect = 1;
static bool reflect_batt = 1;

static TextLayer *s_myweather_layer, *s_battery_text_layer;
static int currFrameNum = 0;
static int totFrames = 21;
static bool initComplete = false;
static GBitmap *s_background_bitmap, *s_bitmap;
static BitmapLayer *s_bitmap_layer;
static GBitmapSequence *s_sequence = NULL;

// =======================
// ROTATING LOGO FUNCTIONS
// =======================
static void load_first_image() {
  gbitmap_sequence_restart(s_sequence);
  gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_bitmap, NULL);
  currFrameNum = 1;
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));
}

static void logo_rotator(void *context) {
  uint32_t next_delay;

  // Advance to the next APNG frame
  if(gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_bitmap, &next_delay) && currFrameNum < totFrames) {
    currFrameNum++;
    bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));

    // Timer for that delay
    app_timer_register(next_delay, logo_rotator, NULL);
  } else {
    // Reset to first image
    load_first_image();
  }
}

static void rotate_logo() {
  app_timer_register(1, logo_rotator, NULL);
}

// ========================
// TEXT ANIMATION FUNCTIONS
// ========================
void on_animation_stopped(Animation *anim, bool finished, void *context) {
    //Free the memory used by the Animation
    property_animation_destroy((PropertyAnimation*) anim);
}

static void bluetooth_handler(bool connected) {
	if (!connected) {
		layer_set_hidden(text_layer_get_layer(s_bluetooth_layer), false);
		if (vibe_on_disconnect == 1) {
			vibes_long_pulse();
		}
	} else {
		layer_set_hidden(text_layer_get_layer(s_bluetooth_layer), true);
		if (vibe_on_connect == 1) {
			vibes_double_pulse();
		}
	}
}
 
void animate_layer(Layer *layer, GRect *start, GRect *finish, int duration, int delay) {
    //Declare animation
    PropertyAnimation *anim = property_animation_create_layer_frame(layer, start, finish);
 
    //Set characteristics
    animation_set_duration((Animation*) anim, duration);
    animation_set_delay((Animation*) anim, delay);
 
    //Set stopped handler to free memory
    AnimationHandlers handlers = {
        //The reference to the stopped handler is the only one in the array
        .stopped = (AnimationStoppedHandler) on_animation_stopped
    };
    animation_set_handlers((Animation*) anim, handlers, NULL);
 
    //Start animation!
    animation_schedule((Animation*) anim);
}

static void init_animations() {
	GRect timestart = GRect(0, -60, 144, 168);
	GRect timefinish = GRect(0, 90, 144, 168);

	GRect datestart = GRect(0, 190, 144, 168);
	GRect datefinish = GRect(0,126, 144, 168);

	GRect battstart = GRect(-140, 0, 144, 168);
	GRect battfinish = GRect(0, 0, 144, 168);

	GRect chargestart = GRect(0, 210, 144, 168);
	GRect chargefinish = GRect(0,160, 144, 168);

	int animlen = 500;

	animate_layer(text_layer_get_layer(s_time_layer), &timestart, &timefinish, animlen, 0);
	animate_layer(text_layer_get_layer(s_date_layer), &datestart, &datefinish, animlen, 0);
	animate_layer(text_layer_get_layer(s_charge_layer), &chargestart, &chargefinish, animlen, 0);
	animate_layer(s_batt_layer, &battstart, &battfinish, animlen, 0);
}

static void animate_layers() {
  // Weather moves in from bottom
	GRect wins = GRect(0, 182, 144, 14);
	GRect winf = GRect(0, 150, 144, 14);
	animate_layer(text_layer_get_layer(s_conditions_layer), &wins, &winf, 1000, 0);

	GRect wouts = GRect(0, 150, 144, 14);
	GRect woutf = GRect(0, 182, 144, 14);
	animate_layer(text_layer_get_layer(s_conditions_layer), &wouts, &woutf, 1000, 5000);

	// Temp moves in from bottom
	GRect tins = GRect(0, 182, 144, 14);
	GRect tinf = GRect(0, 150, 144, 14);
	animate_layer(text_layer_get_layer(s_temp_layer), &tins, &tinf, 1000, 0);

	GRect touts = GRect(0, 150, 144, 14);
	GRect toutf = GRect(0, 182, 144, 14);
	animate_layer(text_layer_get_layer(s_temp_layer), &touts, &toutf, 1000, 5000);
}

// ==================
// CALLBACK FUNCTIONS
// ==================

static void battery_layer_update() {	
	BatteryChargeState state = battery_state_service_peek();
	int pct = state.charge_percent;
  static char batt_buffer[] = "100%";
	
  // Create batter string
	snprintf(batt_buffer,sizeof(batt_buffer),"%d%%",pct);
  
	// Display battery string
	text_layer_set_text(s_battery_text_layer, batt_buffer);
}

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
	
  static char time_buffer[] = "00:00 AM";
	static char date_buffer[] = "WWW  MMM  DD";
  
  if(clock_is_24h_style() == false) {
    strftime(time_buffer, sizeof(time_buffer), "%l:%M %p", tick_time);
  } else {
    strftime(time_buffer, sizeof(time_buffer), "%H:%M", tick_time);
  }
	
	text_layer_set_text(s_time_layer, time_buffer);
	
	strftime(date_buffer, sizeof(date_buffer), "%a  %b  %d", tick_time);
	
	text_layer_set_text(s_date_layer, date_buffer);
}

static void charge_handler() {
	BatteryChargeState state = battery_state_service_peek();
	bool charging = state.is_charging;
	
	if (charging == true) {
		layer_set_hidden(text_layer_get_layer(s_charge_layer), false);
    layer_set_hidden(s_weather_layer, true);
    layer_set_hidden(s_weather_layer_unanimated, true);
	} else {
		layer_set_hidden(text_layer_get_layer(s_charge_layer), true);
    layer_set_hidden(s_weather_layer, false);
    layer_set_hidden(s_weather_layer_unanimated, false);
	}
  battery_layer_update();
}

static void battery_handler(BatteryChargeState state) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Battery change registered!");
	layer_mark_dirty(s_batt_layer);
	charge_handler();
}

static void batt_layer_draw(Layer *layer, GContext *ctx) {	
	BatteryChargeState state = battery_state_service_peek();
	int pct = state.charge_percent;

	#ifdef PBL_COLOR // If on basalt
		if (persist_exists(KEY_TEXT_COLOR)) { // Check for existing colour
			int text_color = persist_read_int(KEY_TEXT_COLOR);
    		GColor fill_color = GColorFromHEX(text_color);
			graphics_context_set_fill_color(ctx, fill_color); // Make the remaining battery that colour
		} else {
			graphics_context_set_fill_color(ctx, GColorWhite); // Otherwise, default to white
		}
	#else // If on aplite
		if (persist_exists(KEY_INVERT_COLORS)) { // Check for invert setting
			if (invert_colors == 1) {
				graphics_context_set_fill_color(ctx, GColorBlack); // If inverted, make the remaining battery black
			} else {
				graphics_context_set_fill_color(ctx, GColorWhite); // Otherwise, default to white
			}
		} else {
			graphics_context_set_fill_color(ctx, GColorWhite); // If no invert setting, default to white
		}
	#endif

	//graphics_fill_rect(ctx, GRect(2, 92, 140-(((100-pct)/10)*14), 2), 0, GCornerNone); // Draw battery

	graphics_fill_rect(ctx, GRect(72, 127,  (140-(((100-pct)/10)*14))/2, 2), 0, GCornerNone); // Centre to right
	graphics_fill_rect(ctx, GRect(72, 127, -(140-(((100-pct)/10)*14))/2, 2), 0, GCornerNone); // Centre to left
}

static void static_layer_draw(Layer *layer, GContext *ctx) {
	#ifdef PBL_COLOR // If on basalt
		if (persist_exists(KEY_TEXT_COLOR)) { // Check for existing colour
			int text_color = persist_read_int(KEY_TEXT_COLOR);
    		GColor fill_color = GColorFromHEX(text_color);
			graphics_context_set_fill_color(ctx, fill_color); // Make the remaining battery that colour
		} else {
			graphics_context_set_fill_color(ctx, GColorWhite); // Otherwise, default to white
		}
	#else // If on aplite
		if (persist_exists(KEY_INVERT_COLORS)) { // Check for invert setting
			if (invert_colors == 1) {
				graphics_context_set_fill_color(ctx, GColorBlack); // If inverted, make the remaining battery black
			} else {
				graphics_context_set_fill_color(ctx, GColorWhite); // Otherwise, default to white
			}
		} else {
			graphics_context_set_fill_color(ctx, GColorWhite); // If no invert setting, default to white
		}
	#endif

	graphics_fill_rect(ctx, GRect(2, 127, 140, 2), 0, GCornerNone); // Draw static bar
}

static void update_layers() {
	if (show_weather == 0) {
		layer_set_hidden(s_weather_layer, true);
		layer_set_hidden(s_weather_layer_unanimated, true);
	} else {
		if (shake_for_weather == 0) {
			layer_set_hidden(s_weather_layer, true);
			layer_set_hidden(s_weather_layer_unanimated, false);
		} else {
			layer_set_hidden(s_weather_layer, false);
			layer_set_hidden(s_weather_layer_unanimated, true);
		}
	}
}

static void set_text_color(int color) {
  #ifdef PBL_COLOR
		GColor text_color = GColorFromHEX(color);
		text_layer_set_text_color(s_time_layer, text_color);
		text_layer_set_text_color(s_date_layer, text_color);
		text_layer_set_text_color(s_temp_layer, text_color);
		text_layer_set_text_color(s_conditions_layer, text_color);
		text_layer_set_text_color(s_temp_layer_unanimated, text_color);
		text_layer_set_text_color(s_conditions_layer_unanimated, text_color);
		text_layer_set_text_color(s_charge_layer, text_color);
		text_layer_set_text_color(s_bluetooth_layer, text_color);
		//text_layer_set_text_color(s_myweather_layer, text_color);
		text_layer_set_text_color(s_battery_text_layer, text_color);
  #endif
}

static void set_background_color(int bgcolor) {
	#ifdef PBL_COLOR
		GColor bg_color = GColorFromHEX(bgcolor);
		window_set_background_color(s_main_window, bg_color);
  	#else

  	#endif
}

static void inverter() {
	if (invert_colors == 1) {
	    	window_set_background_color(s_main_window, GColorWhite);
			text_layer_set_text_color(s_time_layer, GColorBlack);
			text_layer_set_text_color(s_date_layer, GColorBlack);
			text_layer_set_text_color(s_temp_layer, GColorBlack);
			text_layer_set_text_color(s_conditions_layer, GColorBlack);
			text_layer_set_text_color(s_temp_layer_unanimated, GColorBlack);
			text_layer_set_text_color(s_conditions_layer_unanimated, GColorBlack);
			text_layer_set_text_color(s_charge_layer, GColorBlack);
			text_layer_set_text_color(s_bluetooth_layer, GColorBlack);
			//text_layer_set_text_color(s_myweather_layer, GColorBlack);
			text_layer_set_text_color(s_battery_text_layer, GColorBlack);
	    } else {
	    	window_set_background_color(s_main_window, GColorBlack);
			text_layer_set_text_color(s_time_layer, GColorWhite);
			text_layer_set_text_color(s_date_layer, GColorWhite);
			text_layer_set_text_color(s_temp_layer, GColorWhite);
			text_layer_set_text_color(s_conditions_layer, GColorWhite);
			text_layer_set_text_color(s_temp_layer_unanimated, GColorWhite);
			text_layer_set_text_color(s_conditions_layer_unanimated, GColorWhite);
			text_layer_set_text_color(s_charge_layer, GColorWhite);
			text_layer_set_text_color(s_bluetooth_layer, GColorWhite);
			//text_layer_set_text_color(s_myweather_layer, GColorWhite);
			text_layer_set_text_color(s_battery_text_layer, GColorWhite);
	    }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  static char temp_buffer[15];
  static char temp_c_buffer[15];
  static char conditions_buffer[100];
  static char weather_buffer[50];

  Tuple *text_color_t = dict_find(iter, KEY_TEXT_COLOR);
  Tuple *invert_colors_t = dict_find(iter, KEY_INVERT_COLORS);
  Tuple *temperature_t = dict_find(iter, KEY_TEMPERATURE);
  Tuple *temperature_in_c_t = dict_find(iter, KEY_TEMPERATURE_IN_C);
  Tuple *conditions_t = dict_find(iter, KEY_CONDITIONS);
  Tuple *shake_for_weather_t = dict_find(iter, KEY_SHAKE_FOR_WEATHER);
  Tuple *show_weather_t = dict_find(iter, KEY_SHOW_WEATHER);
  Tuple *use_celsius_t = dict_find(iter, KEY_USE_CELSIUS);
  Tuple *background_color_t = dict_find(iter, KEY_BACKGROUND_COLOR);
  Tuple *vibe_on_connect_t = dict_find(iter, KEY_VIBE_ON_CONNECT);
  Tuple *vibe_on_disconnect_t = dict_find(iter, KEY_VIBE_ON_DISCONNECT);
  Tuple *reflect_batt_t = dict_find(iter, KEY_REFLECT_BATT);

  if (text_color_t) {
    int text_color = text_color_t->value->int32;
    APP_LOG(APP_LOG_LEVEL_INFO, "KEY_TEXT_COLOR received!");

    persist_write_int(KEY_TEXT_COLOR, text_color);
    #ifdef PBL_COLOR
    	set_text_color(text_color);
    #else

    #endif
  }

  if (background_color_t) {
  	int bg_color = background_color_t->value->int32;
  	APP_LOG(APP_LOG_LEVEL_INFO, "KEY_BACKGROUND_COLOR received!");

  	persist_write_int(KEY_BACKGROUND_COLOR, bg_color);
  	#ifdef PBL_COLOR
    	set_background_color(bg_color);
    #else

    #endif
  }

  if (invert_colors_t) {
    invert_colors = invert_colors_t->value->int8;


    persist_write_int(KEY_INVERT_COLORS, invert_colors);

    #ifdef PBL_COLOR

    #else
    	inverter();
    #endif

  /*if (invert_colors == 1) {
    #ifdef PBL_COLOR
    	// Do not try to invert
    #else
	    if (invert_colors == 1) {
	    	window_set_background_color(s_main_window, GColorWhite);
			text_layer_set_text_color(s_time_layer, GColorBlack);
			text_layer_set_text_color(s_date_layer, GColorBlack);
			text_layer_set_text_color(s_temp_layer, GColorBlack);
			text_layer_set_text_color(s_conditions_layer, GColorBlack);
			text_layer_set_text_color(s_temp_layer_unanimated, GColorBlack);
			text_layer_set_text_color(s_conditions_layer_unanimated, GColorBlack);
	    } else {
	    	window_set_background_color(s_main_window, GColorBlack);
			text_layer_set_text_color(s_time_layer, GColorWhite);
			text_layer_set_text_color(s_date_layer, GColorWhite);
			text_layer_set_text_color(s_temp_layer, GColorWhite);
			text_layer_set_text_color(s_conditions_layer, GColorWhite);
			text_layer_set_text_color(s_temp_layer_unanimated, GColorWhite);
			text_layer_set_text_color(s_conditions_layer_unanimated, GColorWhite);
	    }
	#endif
    }*/
  }

  if (use_celsius_t) {
  	APP_LOG(APP_LOG_LEVEL_INFO, "KEY_USE_CELSIUS received!");

  	use_celsius = use_celsius_t->value->int8;

  	persist_write_int(KEY_USE_CELSIUS, use_celsius);
  }

  if (shake_for_weather_t) {
  	APP_LOG(APP_LOG_LEVEL_INFO, "KEY_SHAKE_FOR_WEATHER received!");

  	shake_for_weather = shake_for_weather_t->value->int8;

  	persist_write_int(KEY_SHAKE_FOR_WEATHER, shake_for_weather);
  }

  if (show_weather_t) {
  	APP_LOG(APP_LOG_LEVEL_INFO, "KEY_SHOW_WEATHER received!");

  	show_weather = show_weather_t->value->int8;

  	persist_write_int(KEY_SHOW_WEATHER, show_weather);
  }

  if (temperature_t) {
  	APP_LOG(APP_LOG_LEVEL_INFO, "KEY_TEMPERATURE received!");

  	snprintf(temp_buffer, sizeof(temp_buffer), "%d F", (int)temperature_t->value->int32);
  }

  if (temperature_in_c_t) {
  	APP_LOG(APP_LOG_LEVEL_INFO, "KEY_TEMPERATURE_IN_C received!");

  	snprintf(temp_c_buffer, sizeof(temp_c_buffer), "%d C", (int)temperature_in_c_t->value->int32);
  }

  if (conditions_t) {
  	APP_LOG(APP_LOG_LEVEL_INFO, "KEY_CONDITIONS received!");

  	snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_t->value->cstring);
  	text_layer_set_text(s_conditions_layer, conditions_buffer);
  	text_layer_set_text(s_conditions_layer_unanimated, conditions_buffer);
  }

  if (vibe_on_connect_t) {
  	APP_LOG(APP_LOG_LEVEL_INFO, "KEY_VIBE_ON_CONNECT received!");
  	vibe_on_connect = vibe_on_connect_t->value->int8;
  }

  if (vibe_on_disconnect_t) {
  	APP_LOG(APP_LOG_LEVEL_INFO, "KEY_VIBE_ON_DISCONNECT received!");
  	vibe_on_disconnect = vibe_on_disconnect_t->value->int8;
  }

  if (use_celsius == 1) {
  	text_layer_set_text(s_temp_layer, temp_c_buffer);
  	text_layer_set_text(s_temp_layer_unanimated, temp_c_buffer);
    //snprintf(weather_buffer, sizeof(weather_buffer),"%s   %s",temp_c_buffer,conditions_buffer);
  } else {
  	text_layer_set_text(s_temp_layer, temp_buffer);
  	text_layer_set_text(s_temp_layer_unanimated, temp_buffer);
    //snprintf(weather_buffer, sizeof(weather_buffer),"%s   %s",temp_buffer,conditions_buffer);
  }
  //text_layer_set_text(s_myweather_layer, weather_buffer);

  if (reflect_batt_t) {
  	APP_LOG(APP_LOG_LEVEL_INFO, "KEY_REFLECT_BATT received!");

  	reflect_batt = reflect_batt_t->value->int8;

  	persist_write_int(KEY_REFLECT_BATT, reflect_batt);
  }

  if (reflect_batt == 1) {
	layer_set_hidden(s_static_layer, true);
	layer_set_hidden(s_batt_layer, false);
  } else {
	layer_set_hidden(s_static_layer, false);
	layer_set_hidden(s_batt_layer, true);
  }


  update_layers();
}

static void main_window_load(Window *window) {
	// Create fonts
	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MATLAB_34));
	s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MATLAB_22));
	s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MATLAB_14));
  
  // Create logo
  s_sequence = gbitmap_sequence_create_with_resource(RESOURCE_ID_LOGO_ANIMATION);
  s_bitmap = gbitmap_create_blank(gbitmap_sequence_get_bitmap_size(s_sequence), GBitmapFormat8Bit);
  s_bitmap_layer = bitmap_layer_create(GRect(27, 2, 90, 95));
  load_first_image();
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bitmap_layer));

	// Weather parent layers
	s_weather_layer = layer_create(GRect(0, 0, 144, 168));
	s_weather_layer_unanimated = layer_create(GRect(0, 0, 144, 168));
	
	// Battery bar
	s_batt_layer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(s_batt_layer, batt_layer_draw);

	// Static bar
	s_static_layer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(s_static_layer, static_layer_draw);

	// Time layer
	s_time_layer = text_layer_create(GRect(0, 90, 144, 168));
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_font(s_time_layer, s_time_font);
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	//text_layer_set_text(s_time_layer, "12:34");
	
	// Date layer
	s_date_layer = text_layer_create(GRect(0, 126, 144, 168));
	text_layer_set_background_color(s_date_layer, GColorClear);
	text_layer_set_font(s_date_layer, s_date_font);
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
	//text_layer_set_text(s_date_layer, "THU OCT 15");

	
	// Charging status
	s_charge_layer = text_layer_create(GRect(0, 150, 144, 14));
	text_layer_set_background_color(s_charge_layer, GColorClear);
	text_layer_set_font(s_charge_layer, s_weather_font);
	text_layer_set_text_alignment(s_charge_layer, GTextAlignmentCenter);
	text_layer_set_text(s_charge_layer, "...CHARGING...");
	layer_set_hidden(text_layer_get_layer(s_charge_layer), true);

	// Bluetooth status
	s_bluetooth_layer = text_layer_create(GRect(0, 0, 144, 14));
	text_layer_set_background_color(s_bluetooth_layer, GColorClear);
	text_layer_set_font(s_bluetooth_layer, s_weather_font);
	text_layer_set_text_alignment(s_bluetooth_layer, GTextAlignmentLeft);
	text_layer_set_text(s_bluetooth_layer, "BT OFF!");



	// Temperature
  s_temp_layer = text_layer_create(GRect(0, 182, 144, 14));
	text_layer_set_background_color(s_temp_layer, GColorClear);
	text_layer_set_font(s_temp_layer, s_weather_font);
	text_layer_set_text_alignment(s_temp_layer, GTextAlignmentLeft);

	// Conditions
	s_conditions_layer = text_layer_create(GRect(0, 182, 144, 14));
	text_layer_set_font(s_conditions_layer, s_weather_font);
	text_layer_set_background_color(s_conditions_layer, GColorClear);
	text_layer_set_text_alignment(s_conditions_layer, GTextAlignmentCenter);

	// Temperature unanimated
  s_temp_layer_unanimated = text_layer_create(GRect(0, 150, 144, 14));
	text_layer_set_background_color(s_temp_layer_unanimated, GColorClear);
	text_layer_set_font(s_temp_layer_unanimated, s_weather_font);
	text_layer_set_text_alignment(s_temp_layer_unanimated, GTextAlignmentLeft);

	// Conditions unanimated
	s_conditions_layer_unanimated = text_layer_create(GRect(0, 150, 144, 14));
	text_layer_set_font(s_conditions_layer_unanimated, s_weather_font);
	text_layer_set_background_color(s_conditions_layer_unanimated, GColorClear);
	text_layer_set_text_alignment(s_conditions_layer_unanimated, GTextAlignmentCenter);
	
	// Battery text
	s_battery_text_layer = text_layer_create(GRect(0, 150, 144, 14));
	text_layer_set_background_color(s_battery_text_layer, GColorClear);
	text_layer_set_font(s_battery_text_layer, s_weather_font);
	text_layer_set_text_alignment(s_battery_text_layer, GTextAlignmentRight);
	text_layer_set_text(s_battery_text_layer, "100%");
	
  // My weather layer
  /*
  s_myweather_layer = text_layer_create(GRect(0, 150, 144, 14));
	text_layer_set_font(s_myweather_layer, s_weather_font);
	text_layer_set_background_color(s_myweather_layer, GColorClear);
	text_layer_set_text_alignment(s_myweather_layer, GTextAlignmentLeft);
  */
  
	/* Add children */

	// Main elements
	layer_add_child(window_get_root_layer(window), s_batt_layer);
	layer_add_child(window_get_root_layer(window), s_static_layer);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

	// Extra elements
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_charge_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_bluetooth_layer));

	// Weather elements
	layer_add_child(window_get_root_layer(window), s_weather_layer);
	layer_add_child(window_get_root_layer(window), s_weather_layer_unanimated);
	layer_add_child(s_weather_layer, text_layer_get_layer(s_temp_layer));
	layer_add_child(s_weather_layer, text_layer_get_layer(s_conditions_layer));
	layer_add_child(s_weather_layer_unanimated, text_layer_get_layer(s_temp_layer_unanimated));
	layer_add_child(s_weather_layer_unanimated, text_layer_get_layer(s_conditions_layer_unanimated));
	
  //layer_add_child(s_weather_layer_unanimated, text_layer_get_layer(s_myweather_layer));
  layer_add_child(s_weather_layer_unanimated, text_layer_get_layer(s_battery_text_layer));

	/* Check for existing keys */
	
	#ifdef PBL_COLOR
		if (persist_exists(KEY_TEXT_COLOR)) {
	    	int text_color = persist_read_int(KEY_TEXT_COLOR);
	    	APP_LOG(APP_LOG_LEVEL_INFO, "KEY_TEXT_COLOR exists!");
	    	set_text_color(text_color);
	    } else {
	    	set_text_color(0xFFFFFF); // white
	    }

	    if (persist_exists(KEY_BACKGROUND_COLOR)) {
	    	int bg_color = persist_read_int(KEY_BACKGROUND_COLOR);
	    	APP_LOG(APP_LOG_LEVEL_INFO, "KEY_BACKGROUND_COLOR exists!");
	    	set_background_color(bg_color);
	    } else {
	    	set_background_color(0x000000); // black
	    }
	#endif


	#ifdef PBL_BW
	    if (persist_exists(KEY_INVERT_COLORS)) {
	    	APP_LOG(APP_LOG_LEVEL_INFO, "KEY_INVERT_COLORS exists!");
	    	invert_colors = persist_read_int(KEY_INVERT_COLORS);
	    }

	    inverter();
	#endif

	if (persist_exists(KEY_USE_CELSIUS)) {
  	  use_celsius = persist_read_int(KEY_USE_CELSIUS);
  	}

  	if (persist_exists(KEY_SHAKE_FOR_WEATHER)) {
  	  shake_for_weather = persist_read_int(KEY_SHAKE_FOR_WEATHER);
  	}

  	if (persist_exists(KEY_REFLECT_BATT)) {
  	  reflect_batt = persist_read_int(KEY_REFLECT_BATT);

  	  	if (reflect_batt == 1) {
  			layer_set_hidden(s_static_layer, true);
  			layer_set_hidden(s_batt_layer, false);
  		} else {
  			layer_set_hidden(s_static_layer, false);
  			layer_set_hidden(s_batt_layer, true);
  		}
  	} else {
  		layer_set_hidden(s_static_layer, true);
  		layer_set_hidden(s_batt_layer, false);
  	}

  	if (persist_exists(KEY_SHOW_WEATHER)) {
  		show_weather = persist_read_int(KEY_SHOW_WEATHER);

  		if (show_weather == 1) {
  			update_layers();
  		} else {
  			layer_set_hidden(s_weather_layer, true);
  			layer_set_hidden(s_weather_layer_unanimated, true);
  		}
  	}

  	bool connected = bluetooth_connection_service_peek();

  	if (!connected) {
 		layer_set_hidden(text_layer_get_layer(s_bluetooth_layer), false);
 	} else {
  		layer_set_hidden(text_layer_get_layer(s_bluetooth_layer), true);
 	}

  charge_handler(); // Is the battery charging?
	update_time();
}

static void main_window_unload(Window *window) {
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
	text_layer_destroy(s_charge_layer);
	text_layer_destroy(s_conditions_layer);
	text_layer_destroy(s_conditions_layer_unanimated);
	text_layer_destroy(s_temp_layer);
	text_layer_destroy(s_temp_layer_unanimated);
	layer_destroy(s_batt_layer);
	layer_destroy(s_weather_layer);
	layer_destroy(s_weather_layer_unanimated);
	text_layer_destroy(s_myweather_layer);
	text_layer_destroy(s_battery_text_layer);

	fonts_unload_custom_font(s_time_font);
	fonts_unload_custom_font(s_date_font);
	fonts_unload_custom_font(s_weather_font);
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();

	if (show_weather == 1) {
		// Update weather every 30 minutes
		if(tick_time->tm_min % 30 == 0) {
			// Begin dictionary
			DictionaryIterator *iter;
			app_message_outbox_begin(&iter);

			// Add a key-value pair
			dict_write_uint8(iter, 0, 0);

			// Send the message!
			app_message_outbox_send();
		}
	}
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
	if (shake_for_weather == 1) {
		if (shake_for_weather == 0) {
			APP_LOG(APP_LOG_LEVEL_INFO, "Tap! Not animating.");
			// Do not animate
		} else {
			APP_LOG(APP_LOG_LEVEL_INFO, "Tap! Animating");
			//animate_layers();
		}
	}
  rotate_logo();
}

static void init() {
	s_main_window = window_create();

	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});

	window_stack_push(s_main_window, true);
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	battery_state_service_subscribe(battery_handler);
	accel_tap_service_subscribe(tap_handler);
	bluetooth_connection_service_subscribe(bluetooth_handler);

	app_message_register_inbox_received(inbox_received_handler);
  	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  	//init_animations();
}

static void deinit() {
  window_destroy(s_main_window);
  tick_timer_service_unsubscribe();
 	battery_state_service_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}