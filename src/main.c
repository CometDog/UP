#include "pebble.h"

#ifdef PBL_COLOR
  #include "gcolor_definitions.h" // Allows the use of color
#endif

static Window *s_main_window; // Main window
static TextLayer *s_date_label; // Layer for date
static Layer *s_solid_layer, *s_time_layer; // Background layers
static bool hide; // Tells if the date is hidden

// Buffers
static char s_date_buffer[] = "MMDD";

// Update background when called
static void update_bg(Layer *layer, GContext *ctx) {
  
  GRect bounds = layer_get_bounds(layer); // Set bounds to full window
  GPoint center = grect_center_point(&bounds); // Find center
  
  // Anti-aliasing
  #ifdef PBL_COLOR
    graphics_context_set_antialiased(ctx, true);
  #endif
  
  //Background color
  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorVividCerulean); // Set the fill color.
  #else 
    graphics_context_set_fill_color(ctx, GColorBlack); // Set the fill color.
  #endif
  graphics_fill_rect(ctx, bounds, 0, GCornerNone); // Fill the screen
  
  //Background circle
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, 64);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 61);
  
  //Center dot
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, 5);
}

// Handles updating time
static void update_time(Layer *layer, GContext *ctx) {
  
  GRect bounds = layer_get_bounds(layer); // Set bounds to full window
  GPoint center = grect_center_point(&bounds); // Find center
  
  // Anti-aliasing
  #ifdef PBL_COLOR
    graphics_context_set_antialiased(ctx, true);
  #endif
  
  // Get time and structure
  time_t temp = time(NULL); 
  struct tm *t = localtime(&temp);
  
  //Angles
  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  int32_t minute_angle = TRIG_MAX_ANGLE * t->tm_min / 60;
  int32_t hour_angle = (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6);
  
  //Length of hands
  int16_t minute_hand_length = 74;
  int16_t hour_hand_length = 54;
  
  //X and Y of second dot.
  int secY = -64 * cos_lookup(second_angle) / TRIG_MAX_RATIO + center.y; // Get the Y position
  int secX = 64 * sin_lookup(second_angle) / TRIG_MAX_RATIO + center.x; // Get the X position

  // Second dot
  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorYellow); 
    graphics_fill_circle(ctx, GPoint(secX, secY), 5);
    graphics_context_set_antialiased(ctx, true);
  #else
    graphics_context_set_fill_color(ctx, GColorWhite); 
    graphics_fill_circle(ctx, GPoint(secX, secY), 5);
  #endif
    
  //Minute hand
  GPoint minute_hand = {
    .x = (int16_t)(sin_lookup(minute_angle) * (int32_t)minute_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(minute_angle) * (int32_t)minute_hand_length / TRIG_MAX_RATIO) + center.y,
  };
  
  //Hour hand
  GPoint hour_hand = {
    .x = (int16_t)(sin_lookup(hour_angle) * (int32_t)hour_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)hour_hand_length / TRIG_MAX_RATIO) + center.y,
  };
  
  //Draw the hands
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, minute_hand, center);
  graphics_draw_line(ctx, hour_hand, center);

  //Write and display dates
  strftime(s_date_buffer, sizeof(s_date_buffer), "%m%d", t);
  text_layer_set_text(s_date_label, s_date_buffer);
}

// Update time shown on screen
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_main_window));
}

// Loads the layers onto the main window
static void main_window_load(Window *window) {
  
  // Creates window_layer as root and sets its bounds
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create background the layers
  s_solid_layer = layer_create(bounds);
  s_time_layer = layer_create(bounds);
  
  // Update background layers
  layer_set_update_proc(s_solid_layer, update_bg);
  layer_set_update_proc(s_time_layer, update_time);
  
  // Create the label
  s_date_label = text_layer_create(GRect(0,130,144,40));
  
  //Set font
  text_layer_set_font(s_date_label, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  
  // Set background and text colors
  #ifdef PBL_COLOR 
    text_layer_set_background_color(s_date_label, GColorVividCerulean);
  #else
    text_layer_set_background_color(s_date_label, GColorBlack);
  #endif
  text_layer_set_text_color(s_date_label, GColorWhite);
  
  // Avoid blank screen in case updating time fails
  text_layer_set_text(s_date_label, "NULL");
  
  // Align text
  text_layer_set_text_alignment(s_date_label, GTextAlignmentCenter);
  
  // Apply layers to screen
  layer_add_child(window_layer, s_solid_layer); 
  layer_add_child(window_layer, s_time_layer);
  layer_add_child(window_layer, text_layer_get_layer(s_date_label));
}

//Control the shake gesture
static void tap_handler(AccelAxisType axis, int32_t direction) {
  // Hide/show date on shake
  if (hide != true) {
    layer_set_hidden((Layer *)s_date_label, true);
    hide = true;
  }
  else {
    layer_set_hidden((Layer *)s_date_label, false);
    hide = false;
  }
}

// Unloads the layers on the main window
static void main_window_unload(Window *window) {
  
  // Destroy the background layers
  layer_destroy(s_solid_layer);
  layer_destroy(s_time_layer);
  
  // Destroy the labels
  text_layer_destroy(s_date_label);
}
  
// Initializes the main window
static void init() {
  s_main_window = window_create(); // Create the main window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true); // Show the main window. Animations = true.
  
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler); // Update time every minute
  accel_tap_service_subscribe(tap_handler);
}

// Deinitializes the main window
static void deinit() {
  window_destroy(s_main_window); // Destroy the main window
  accel_tap_service_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}