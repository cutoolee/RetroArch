#ifndef HH_QUICK_MENU_H
#define HH_QUICK_MENU_H

#include "hh_quick_menu_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hh_quick_menu
{
   hh_quick_menu_state_t state;
   hh_quick_menu_view_t view;
   unsigned long open_generation;
   bool touch_active;
   bool touch_dragged;
   float touch_start_x;
   float touch_start_y;
   float touch_last_x;
   float touch_last_y;
   float list_scroll_fraction;
   bool touch_scrolled;
   hh_quick_menu_page_t touch_scroll_page;
   hh_quick_menu_input_t touch_input;
   size_t touch_index;
   float main_scroll;
   float main_scroll_origin;
   float main_scroll_target;
   float main_scroll_elapsed;
   float slot_scroll;
   float slot_scroll_origin;
   float slot_scroll_target;
   float slot_scroll_elapsed;
   unsigned long animation_time_ms;
   unsigned buttons_previous;
   unsigned buttons_repeating;
   unsigned long button_time_ms[4];
} hh_quick_menu_t;

bool hh_quick_menu_is_shader_page(const hh_quick_menu_t *menu);
const char *hh_quick_menu_shader_scope_label(size_t scope);
const char *hh_quick_menu_shader_scope_detail(size_t scope);
void hh_quick_menu_init(hh_quick_menu_t *menu);
void hh_quick_menu_open(hh_quick_menu_t *menu);
void hh_quick_menu_finish_open(hh_quick_menu_t *menu);
void hh_quick_menu_close(hh_quick_menu_t *menu);
void hh_quick_menu_finish_close(hh_quick_menu_t *menu);

hh_quick_menu_state_t hh_quick_menu_get_state(
      const hh_quick_menu_t *menu);
const hh_quick_menu_view_t *hh_quick_menu_get_view(
      const hh_quick_menu_t *menu);

void hh_quick_menu_move_up(hh_quick_menu_t *menu);
void hh_quick_menu_move_down(hh_quick_menu_t *menu);
hh_ui_action_t hh_quick_menu_confirm(hh_quick_menu_t *menu);
void hh_quick_menu_action_complete(hh_quick_menu_t *menu);
void hh_quick_menu_back(hh_quick_menu_t *menu);

void hh_quick_menu_dialog_move_left(hh_quick_menu_t *menu);
void hh_quick_menu_dialog_move_right(hh_quick_menu_t *menu);
hh_ui_action_t hh_quick_menu_dialog_confirm(hh_quick_menu_t *menu);

void hh_quick_menu_slot_prev(hh_quick_menu_t *menu);
void hh_quick_menu_slot_next(hh_quick_menu_t *menu);
void hh_quick_menu_update_animation(hh_quick_menu_t *menu,
      unsigned long time_ms);
void hh_quick_menu_set_busy(hh_quick_menu_t *menu, bool busy);

void hh_quick_menu_set_game(hh_quick_menu_t *menu,
      const hh_quick_menu_game_t *game);
void hh_quick_menu_set_slot(hh_quick_menu_t *menu,
      const hh_quick_menu_slot_t *slot);
void hh_quick_menu_set_capabilities(hh_quick_menu_t *menu,
      const hh_quick_menu_capabilities_t *capabilities);
hh_quick_menu_item_state_t hh_quick_menu_item_state(
      const hh_quick_menu_t *menu, size_t index);
bool hh_quick_menu_slot_disabled(const hh_quick_menu_t *menu, int slot);
hh_ui_action_t hh_quick_menu_input(hh_quick_menu_t *menu,
      hh_quick_menu_input_t input);
/* Button bits use hh_quick_menu_input_t indices. */
unsigned hh_quick_menu_poll_buttons(hh_quick_menu_t *menu,
      unsigned buttons, unsigned long time_ms);
bool hh_quick_menu_touch(hh_quick_menu_t *menu, float x, float y,
      bool pressed, float width, float height, hh_quick_menu_input_t *input);
void hh_quick_menu_action_result(hh_quick_menu_t *menu,
      hh_quick_menu_feedback_t feedback, const char *message);
void hh_quick_menu_clear_feedback(hh_quick_menu_t *menu);
bool hh_quick_menu_is_controls_page(const hh_quick_menu_t *menu);
unsigned hh_quick_menu_control_count(const hh_quick_menu_t *menu);
hh_ui_action_t hh_quick_menu_controls_input(hh_quick_menu_t *menu,
      hh_quick_menu_input_t input);
void hh_quick_menu_control_label(const hh_quick_menu_t *menu, unsigned row,
      char *label, size_t size);

const char *hh_ui_action_to_string(hh_ui_action_t action);
const char *hh_quick_menu_state_to_string(hh_quick_menu_state_t state);

#ifdef __cplusplus
}
#endif

#endif
