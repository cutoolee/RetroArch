#include <assert.h>
#include <string.h>

#include "handheld/bridge/hh_bridge.h"
#include "handheld/ui/hh_quick_menu_render.h"

static hh_runtime_event_callback_t callback;
static void *callback_data;
static uint64_t next_id = 1;
static hh_command_type_t last_command;
static bool fake_ra_menu_open;
static int last_argument;
static hh_result_t submit_result = HH_OK;
static int shader_active_id;

hh_result_t hh_runtime_get_controls(unsigned player, hh_runtime_controls_t *out)
{
   unsigned i;
   memset(out, 0, sizeof(*out));
   out->player_count = 4;
   out->device_index = player;
   for (i = 0; i < 16; i++)
   {
      out->available[i] = true;
      out->masks[i] = 1U << i;
   }
   return HH_OK;
}

hh_result_t hh_runtime_get_shader_list(bool recommended, hh_runtime_shader_list_t *out)
{
   (void)recommended;
   memset(out, 0, sizeof(*out));
   out->count = 2;
   out->valid = true;
   out->active_id = shader_active_id;
   out->entries[0].id = 0;
   out->entries[1].id = 1;
   strcpy(out->entries[0].name, "进入前的效果");
   strcpy(out->entries[1].name, "关闭着色器");
   return HH_OK;
}

hh_result_t hh_runtime_init(void) { return HH_OK; }
void hh_runtime_deinit(void) {}
hh_result_t hh_runtime_set_event_callback(hh_runtime_event_callback_t cb, void *data)
{ callback = cb; callback_data = data; return HH_OK; }
hh_result_t hh_runtime_get_snapshot(hh_runtime_snapshot_t *s)
{ memset(s, 0, sizeof(*s)); s->initialized = true; s->content_loaded = true; s->ra_menu_open = fake_ra_menu_open; s->runtime_mode = HH_RUNTIME_MODE_CONTENT_RUNNING; return HH_OK; }
bool hh_runtime_has_capability(hh_capability_t c) { return c != HH_CAP_SCREENSHOT; }
hh_result_t hh_runtime_submit_command(hh_command_type_t c, int arg, uint64_t *id)
{ last_argument = arg; last_command = c; *id = next_id++; return submit_result; }
hh_result_t hh_runtime_get_recent_count(size_t *count)
{ *count = 8; return HH_OK; }
hh_result_t hh_runtime_get_recent_game(size_t index, bool thumbnail,
      hh_runtime_recent_game_t *game)
{
   memset(game, 0, sizeof(*game));
   strcpy(game->title, index == 7 ? "拳皇97" : "合金弹头");
   game->available = index != 1;
   if (thumbnail)
   {
      strcpy(game->thumbnail, index == 7 ? "kof97.png" : "mslug.png");
      if (index != 7)
         strcpy(game->video, "mslug.mp4");
   }
   return HH_OK;
}
const char *hh_result_to_string(hh_result_t r) { return r == HH_OK ? "OK" : "ERROR"; }

static void emit(uint64_t id, hh_event_type_t type, hh_result_t result)
{
   hh_runtime_event_t event;
   memset(&event, 0, sizeof(event));
   event.request_id = id; event.type = type; event.result = result;
   if (last_command == HH_CMD_SHADER_BEGIN) shader_active_id = 0;
   if (last_command == HH_CMD_SHADER_PREVIEW && result == HH_OK)
      shader_active_id = last_argument;
   callback(&event, callback_data);
}

static void test_reset(bool pause_completed)
{
   hh_bridge_t bridge;
   uint64_t pause_id, resume_id;
   hh_bridge_init(&bridge);
   assert(hh_bridge_open(&bridge));
   pause_id = bridge.pause_request_id;
   if (pause_completed)
      emit(pause_id, HH_EVENT_PAUSED, HH_OK);
   bridge.menu.view.selected_index = HH_QUICK_MENU_ITEM_RESET;
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(bridge.menu.state == HH_QUICK_MENU_CONFIRM_DIALOG);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_RIGHT);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(last_command == HH_CMD_RESUME);
   assert(bridge.pending_action == HH_UI_ACTION_RESET);
   assert(bridge.menu.state == HH_QUICK_MENU_ACTION_PENDING);
   resume_id = bridge.pending_request_id;
   if (!pause_completed)
      emit(pause_id, HH_EVENT_PAUSED, HH_OK);
   assert(bridge.quick_menu_owns_pause);
   emit(resume_id + 100, HH_EVENT_RESUMED, HH_OK);
   assert(last_command == HH_CMD_RESUME);
   emit(resume_id, HH_EVENT_RESUMED, HH_OK);
   assert(last_command == HH_CMD_RESET);
   assert(bridge.pending_request_id != resume_id);
   assert(!bridge.quick_menu_owns_pause);
   assert(bridge.menu.state == HH_QUICK_MENU_ACTION_PENDING);
   emit(bridge.pending_request_id, HH_EVENT_RESET, HH_OK);
   assert(!hh_bridge_is_open(&bridge));
   assert(!bridge.pending_request_id);
   assert(bridge.pending_action == HH_UI_ACTION_NONE);
   hh_bridge_deinit(&bridge);
}

static void test_reset_error(bool resume_failed)
{
   hh_bridge_t bridge;
   hh_bridge_init(&bridge);
   assert(hh_bridge_open(&bridge));
   emit(bridge.pause_request_id, HH_EVENT_PAUSED, HH_OK);
   bridge.menu.view.selected_index = HH_QUICK_MENU_ITEM_RESET;
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_RIGHT);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(last_command == HH_CMD_RESUME);
   if (resume_failed)
      emit(bridge.pending_request_id, HH_EVENT_ERROR, HH_ERR_RA_COMMAND_FAILED);
   else
   {
      submit_result = HH_ERR_BUSY;
      emit(bridge.pending_request_id, HH_EVENT_RESUMED, HH_OK);
      submit_result = HH_OK;
   }
   assert(bridge.quick_menu_owns_pause == resume_failed);
   assert(bridge.menu.state == HH_QUICK_MENU_OPEN);
   assert(bridge.menu.view.feedback == HH_QUICK_MENU_ACTION_ERROR);
   assert(!bridge.pending_request_id);
   assert(bridge.pending_action == HH_UI_ACTION_NONE);
   assert(!bridge.menu.view.busy);
   hh_bridge_deinit(&bridge);
}

int main(void)
{
   test_reset(true);
   test_reset(false);
   test_reset_error(true);
   test_reset_error(false);
   return 0;
}
