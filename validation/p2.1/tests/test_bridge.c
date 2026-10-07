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

hh_result_t hh_runtime_get_controls_draft(unsigned player, hh_runtime_controls_t *out)
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

int main(void)
{
   hh_bridge_t bridge;
   hh_quick_menu_layout_t layout;
   hh_ui_rect_t card;
   float x, y;
   int i;
   hh_bridge_init(&bridge);
   assert(hh_bridge_open(&bridge));
   emit(bridge.pause_request_id, HH_EVENT_PAUSED, HH_OK);
   assert(bridge.quick_menu_owns_pause);
   hh_bridge_tick(&bridge);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(last_command == HH_CMD_RESUME);
   emit(bridge.pending_request_id, HH_EVENT_RESUMED, HH_OK);
   hh_bridge_tick(&bridge);
   assert(!hh_bridge_is_open(&bridge));
   assert(!bridge.quick_menu_owns_pause);
   assert(!bridge.pending_request_id);
   assert(!bridge.menu.view.busy);
   hh_bridge_deinit(&bridge);

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
   for (i = 0; i < HH_QUICK_MENU_ITEM_ADVANCED_MENU; i++)
      hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_DOWN);

   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(last_command == HH_CMD_RESUME);
   assert(bridge.advanced_wait_resume);
   emit(bridge.pending_request_id, HH_EVENT_RESUMED, HH_OK);
   assert(last_command == HH_CMD_OPEN_RA_MENU);
   assert(bridge.menu.state == HH_QUICK_MENU_CLOSED);
   fake_ra_menu_open = true;
   emit(bridge.pending_request_id, HH_EVENT_RA_MENU_OPENED, HH_OK);
   hh_bridge_tick(&bridge);
   assert(bridge.menu.state == HH_QUICK_MENU_CLOSED);
   fake_ra_menu_open = false;
   hh_bridge_tick(&bridge);
   assert(bridge.menu.state == HH_QUICK_MENU_CLOSED);
   hh_bridge_deinit(&bridge);
   hh_bridge_init(&bridge);
   assert(hh_bridge_open(&bridge));
   for (i = 0; i < HH_QUICK_MENU_ITEM_EXIT; i++)
      hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_DOWN);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(bridge.menu.state == HH_QUICK_MENU_CONFIRM_DIALOG);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(bridge.menu.state == HH_QUICK_MENU_OPEN);
   assert(last_command == HH_CMD_PAUSE);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_RIGHT);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(last_command == HH_CMD_QUIT);
   assert(bridge.pending_action == HH_UI_ACTION_EXIT);
   emit(bridge.pending_request_id, HH_EVENT_QUIT, HH_ERR_RA_COMMAND_FAILED);
   assert(bridge.menu.state == HH_QUICK_MENU_OPEN);
   assert(bridge.menu.view.feedback == HH_QUICK_MENU_ACTION_ERROR);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_RIGHT);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   emit(bridge.pending_request_id, HH_EVENT_QUIT, HH_OK);
   assert(bridge.menu.state == HH_QUICK_MENU_CLOSED);
   hh_bridge_deinit(&bridge);
   hh_bridge_init(&bridge);
   assert(hh_bridge_open(&bridge));
   emit(bridge.pause_request_id, HH_EVENT_PAUSED, HH_OK);
   bridge.menu.view.selected_index = HH_QUICK_MENU_ITEM_RECENT;
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(bridge.menu.view.page == HH_QUICK_MENU_PAGE_RECENT);
   assert(bridge.menu.view.recent_count == 8);
   assert(strcmp(bridge.menu.view.recent[0].title, "合金弹头") == 0);
   assert(strcmp(bridge.menu.view.recent_thumbnail, "mslug.png") == 0);
   assert(strcmp(bridge.menu.view.recent_video, "mslug.mp4") == 0);
   for (i = 0; i < HH_QUICK_MENU_RECENT_ROWS; i++)
   {
      assert(bridge.menu.view.recent[i].thumbnail[0]);
      assert(bridge.menu.view.recent[i].video[0] || i == 7);
   }
   assert(hh_quick_menu_compute_layout(1920, 1080, &layout));
   assert(hh_quick_menu_recent_card_bounds(&layout, 0, 4, &card));
   x = card.x + card.width / 2;
   y = card.y + card.height / 2;
   assert(hh_bridge_touch(&bridge, x, y, true, 1920, 1080));
   assert(bridge.menu.view.recent_selected == 4);
   assert(hh_bridge_touch(&bridge, x - 960, y, true, 1920, 1080));
   assert(bridge.menu.slot_scroll == 3 && bridge.menu.view.recent_first == 2);
   assert(hh_bridge_touch(&bridge, x - 960, y, false, 1920, 1080));
   assert(bridge.menu.state == HH_QUICK_MENU_OPEN);
   assert(strcmp(bridge.menu.view.recent[5].thumbnail, "kof97.png") == 0);
   assert(bridge.menu.view.recent[5].video[0] == '\0');
   for (i = 0; i < 4; i++)
      hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_UP);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_DOWN);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(bridge.menu.state == HH_QUICK_MENU_OPEN);
   assert(last_command == HH_CMD_PAUSE);
   for (i = 0; i < 6; i++)
      hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_DOWN);
   assert(bridge.menu.view.recent_selected == 7);
   assert(bridge.menu.view.recent_first == 2);
   assert(strcmp(bridge.menu.view.recent[5].title, "拳皇97") == 0);
   assert(strcmp(bridge.menu.view.recent[5].thumbnail, "kof97.png") == 0);
   assert(bridge.menu.view.recent[5].video[0] == '\0');
   assert(strcmp(bridge.menu.view.recent_thumbnail, "kof97.png") == 0);
   assert(bridge.menu.view.recent_video[0] == '\0');
   submit_result = HH_ERR_BUSY;
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(bridge.menu.state == HH_QUICK_MENU_OPEN);
   assert(bridge.menu.view.feedback == HH_QUICK_MENU_ACTION_ERROR);
   submit_result = HH_OK;
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(last_command == HH_CMD_LOAD_RECENT && last_argument == 7);
   emit(bridge.pending_request_id + 100, HH_EVENT_CONTENT_LOADED, HH_OK);
   assert(bridge.menu.state == HH_QUICK_MENU_ACTION_PENDING);
   emit(bridge.pending_request_id, HH_EVENT_ERROR, HH_ERR_LOAD_FAILED);
   assert(bridge.menu.state == HH_QUICK_MENU_OPEN);
   assert(bridge.menu.view.feedback == HH_QUICK_MENU_ACTION_ERROR);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   emit(bridge.pending_request_id, HH_EVENT_CONTENT_LOADED, HH_OK);
   assert(bridge.menu.state == HH_QUICK_MENU_CLOSED);
   assert(!bridge.quick_menu_owns_pause);
   hh_bridge_deinit(&bridge);
   hh_bridge_init(&bridge);
   assert(hh_bridge_open(&bridge));
   emit(bridge.pause_request_id, HH_EVENT_PAUSED, HH_OK);
   bridge.menu.view.selected_index = HH_QUICK_MENU_ITEM_SHADER;
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(last_command == HH_CMD_SHADER_BEGIN);
   emit(bridge.pending_request_id, HH_EVENT_SHADER_CHANGED, HH_OK);
   assert(bridge.shader_session_active && bridge.menu.view.page == HH_QUICK_MENU_PAGE_SHADER);
   bridge.menu.animation_time_ms = 1000;
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_DOWN);
   hh_bridge_tick(&bridge);
   bridge.menu.animation_time_ms = 1249;
   hh_bridge_tick(&bridge);
   assert(last_command == HH_CMD_SHADER_BEGIN);
   bridge.menu.animation_time_ms = 1250;
   hh_bridge_tick(&bridge);
   assert(last_command == HH_CMD_SHADER_PREVIEW && last_argument == 1);
   assert(bridge.menu.view.busy);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(bridge.menu.view.page == HH_QUICK_MENU_PAGE_SHADER);
   emit(bridge.pending_request_id, HH_EVENT_ERROR, HH_ERR_SHADER_LOAD_FAILED);
   assert(bridge.menu.view.shader_selected == 0 && bridge.menu.view.shader_valid);
   assert(bridge.menu.view.feedback == HH_QUICK_MENU_ACTION_ERROR);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(bridge.menu.view.page == HH_QUICK_MENU_PAGE_SHADER_SCOPE);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_DOWN);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(last_command == HH_CMD_SHADER_APPLY && last_argument == 1);
   emit(bridge.pending_request_id, HH_EVENT_SHADER_CHANGED, HH_OK);
   assert(!bridge.shader_session_active && bridge.menu.view.page == HH_QUICK_MENU_PAGE_MAIN);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   emit(bridge.pending_request_id, HH_EVENT_SHADER_CHANGED, HH_OK);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_RIGHT);
   assert(bridge.menu.view.shader_fullscreen);
   hh_bridge_close(&bridge);
   assert(last_command == HH_CMD_SHADER_CANCEL && bridge.shader_close_after_cancel);
   emit(bridge.pending_request_id, HH_EVENT_ERROR, HH_ERR_SHADER_RESTORE_FAILED);
   assert(bridge.shader_session_active && hh_bridge_is_open(&bridge));
   assert(!bridge.shader_close_after_cancel);
   hh_bridge_close(&bridge);
   emit(bridge.pending_request_id, HH_EVENT_SHADER_CHANGED, HH_OK);
   assert(last_command == HH_CMD_RESUME);
   assert(!bridge.shader_session_active);
   emit(bridge.pending_request_id, HH_EVENT_RESUMED, HH_OK);
   assert(!hh_bridge_is_open(&bridge));
   assert(hh_bridge_open(&bridge));
   emit(bridge.pause_request_id, HH_EVENT_PAUSED, HH_OK);
   bridge.menu.view.selected_index = HH_QUICK_MENU_ITEM_CONTROLS;
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(bridge.menu.view.page == HH_QUICK_MENU_PAGE_CONTROLS);
   assert(bridge.menu.view.control_selected == 2);
   bridge.menu.view.control_selected = 0;
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_RIGHT);
   assert(bridge.menu.view.control_player == 1);
   assert(bridge.menu.view.controls.device_index == 1);
   bridge.menu.view.control_selected = 2;
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_RIGHT);
   assert(last_command == HH_CMD_CONTROLS_SET);
   assert((unsigned)last_argument == ((1U << 20) | 1));
   emit(bridge.pending_request_id, HH_EVENT_CONTROLS_CHANGED, HH_OK);
   assert(last_command == HH_CMD_CONTROLS_SET);
   assert(bridge.pending_request_id == 0);
   assert(bridge.menu.view.page == HH_QUICK_MENU_PAGE_CONTROL_EDIT);
   assert(bridge.menu.view.control_selected == 0 && !bridge.menu.view.busy);
   assert(bridge.menu.view.controls_dirty);
   assert(strstr(bridge.menu.view.message, "草稿"));
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_RIGHT);
   assert(last_command == HH_CMD_CONTROLS_SET);
   assert((unsigned)last_argument == ((6U << 24) | (1U << 20) | 1));
   assert(bridge.menu.view.busy);
   emit(bridge.pending_request_id, HH_EVENT_CONTROLS_CHANGED, HH_OK);
   assert(last_command == HH_CMD_CONTROLS_SET);
   assert(bridge.pending_request_id == 0 && !bridge.menu.view.busy);
   assert(bridge.menu.view.controls_dirty);
   assert(bridge.menu.view.page == HH_QUICK_MENU_PAGE_CONTROL_EDIT);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_BACK);
   bridge.menu.view.control_selected = 18;
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(last_command == HH_CMD_CONTROLS_SAVE && last_argument == 0);
   emit(bridge.pending_request_id, HH_EVENT_ERROR, HH_ERR_SAVE_FAILED);
   assert(bridge.menu.view.controls_dirty);
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   emit(bridge.pending_request_id, HH_EVENT_CONTROLS_CHANGED, HH_OK);
   assert(!bridge.menu.view.controls_dirty);
   bridge.menu.view.control_selected = 19;
   hh_bridge_input(&bridge, HH_QUICK_MENU_INPUT_CONFIRM);
   assert(last_command == HH_CMD_CONTROLS_SAVE && last_argument == 1);
   emit(bridge.pending_request_id, HH_EVENT_CONTROLS_CHANGED, HH_OK);
   assert(!bridge.menu.view.controls_dirty);
   bridge.menu.view.controls_dirty = true;
   emit(0, HH_EVENT_CONTENT_CLOSED, HH_OK);
   assert(!bridge.menu.view.controls_dirty && !bridge.menu.view.controls_valid);
   hh_bridge_deinit(&bridge);
   return 0;
}
