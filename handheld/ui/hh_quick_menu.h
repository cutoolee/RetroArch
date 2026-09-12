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
} hh_quick_menu_t;

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
void hh_quick_menu_set_busy(hh_quick_menu_t *menu, bool busy);

const char *hh_ui_action_to_string(hh_ui_action_t action);
const char *hh_quick_menu_state_to_string(hh_quick_menu_state_t state);

#ifdef __cplusplus
}
#endif

#endif
