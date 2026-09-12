#ifndef HH_BRIDGE_H
#define HH_BRIDGE_H

#include "../runtime/hh_runtime.h"
#include "../ui/hh_quick_menu.h"

typedef struct hh_bridge
{
   hh_quick_menu_t menu;
   bool initialized;
   bool quick_menu_owns_pause;
   uint64_t pause_request_id;
   uint64_t pending_request_id;
   hh_ui_action_t pending_action;
   int pending_slot;
   bool advanced_wait_resume;
   bool pause_cancel_requested;
} hh_bridge_t;

void hh_bridge_init(hh_bridge_t *bridge);
void hh_bridge_deinit(hh_bridge_t *bridge);
bool hh_bridge_open(hh_bridge_t *bridge);
void hh_bridge_close(hh_bridge_t *bridge);
bool hh_bridge_is_open(const hh_bridge_t *bridge);
bool hh_bridge_input(hh_bridge_t *bridge, hh_quick_menu_input_t input);
void hh_bridge_tick(hh_bridge_t *bridge);
const hh_quick_menu_t *hh_bridge_menu(const hh_bridge_t *bridge);
hh_bridge_t *hh_bridge_active(void);

#endif
