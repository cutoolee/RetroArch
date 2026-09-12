#include <assert.h>
#include <string.h>

#include "handheld/bridge/hh_bridge.h"

static hh_runtime_event_callback_t callback;
static void *callback_data;
static uint64_t next_id = 1;
static hh_command_type_t last_command;

hh_result_t hh_runtime_init(void) { return HH_OK; }
void hh_runtime_deinit(void) {}
hh_result_t hh_runtime_set_event_callback(hh_runtime_event_callback_t cb, void *data)
{ callback = cb; callback_data = data; return HH_OK; }
hh_result_t hh_runtime_get_snapshot(hh_runtime_snapshot_t *s)
{ memset(s, 0, sizeof(*s)); s->initialized = true; s->content_loaded = true; s->runtime_mode = HH_RUNTIME_MODE_CONTENT_RUNNING; return HH_OK; }
bool hh_runtime_has_capability(hh_capability_t c) { return c != HH_CAP_SCREENSHOT; }
hh_result_t hh_runtime_submit_command(hh_command_type_t c, int arg, uint64_t *id)
{ (void)arg; last_command = c; *id = next_id++; return HH_OK; }
const char *hh_result_to_string(hh_result_t r) { return r == HH_OK ? "OK" : "ERROR"; }

static void emit(uint64_t id, hh_event_type_t type, hh_result_t result)
{
   hh_runtime_event_t event;
   memset(&event, 0, sizeof(event));
   event.request_id = id; event.type = type; event.result = result;
   callback(&event, callback_data);
}

int main(void)
{
   hh_bridge_t bridge;
   hh_bridge_init(&bridge);
   assert(hh_bridge_open(&bridge));
   assert(hh_bridge_is_open(&bridge));
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_DOWN);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(last_command == HH_CMD_SAVE_STATE);
   assert(bridge.pending_request_id != 0);
   emit(bridge.pending_request_id, HH_EVENT_STATE_SAVE_ACCEPTED, HH_OK);
   assert(bridge.menu.state == HH_QUICK_MENU_ACTION_PENDING);
   emit(bridge.pending_request_id + 100, HH_EVENT_STATE_SAVE_COMPLETED, HH_OK);
   assert(bridge.menu.state == HH_QUICK_MENU_ACTION_PENDING);
   emit(bridge.pending_request_id, HH_EVENT_STATE_SAVE_COMPLETED, HH_OK);
   assert(bridge.menu.view.feedback == HH_QUICK_MENU_SAVE_SUCCESS);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_BACK);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_DOWN);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   bridge.menu.view.slots[0].occupied = true;
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(last_command == HH_CMD_LOAD_STATE);
   emit(bridge.pending_request_id, HH_EVENT_STATE_LOAD_COMPLETED, HH_ERR_LOAD_FAILED);
   assert(bridge.menu.view.feedback == HH_QUICK_MENU_ACTION_ERROR);
   hh_bridge_deinit(&bridge);

   hh_bridge_init(&bridge);
   assert(hh_bridge_open(&bridge));
   assert(bridge.pause_request_id != 0);
   emit(bridge.pause_request_id, HH_EVENT_PAUSED, HH_OK);
   assert(bridge.quick_menu_owns_pause);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_DOWN);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_DOWN);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_DOWN);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_DOWN);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(last_command == HH_CMD_RESUME);
   assert(bridge.advanced_wait_resume);
   emit(bridge.pending_request_id, HH_EVENT_RESUMED, HH_OK);
   assert(last_command == HH_CMD_OPEN_RA_MENU);
   hh_bridge_deinit(&bridge);
   return 0;
}
