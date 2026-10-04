#include "hh_bridge.h"
#include "../ui/hh_quick_menu_render.h"

#include <string.h>
#include <sys/stat.h>
#include <limits.h>

static hh_bridge_t *hh_active_bridge;
static volatile bool hh_test_input_pending;
static volatile hh_quick_menu_input_t hh_test_input_value;
static volatile bool hh_test_toggle_pending;

#if defined(HAVE_HANDHELD_RUNTIME) && HAVE_HANDHELD_RUNTIME
#include <time/rtime.h>

#define HH_BRIDGE_PATH_MAX 4096

bool runloop_get_savestate_path(char *s, size_t len, int state_slot);
void gfx_savestate_thumbnail_get_path(char *s, size_t len,
      const char *state_name, int state_slot);

static bool hh_bridge_file_exists(const char *path)
{
   struct stat st;
   return path && *path && stat(path, &st) == 0;
}

#endif
#if defined(HAVE_HANDHELD_RUNTIME) && HAVE_HANDHELD_RUNTIME
static void hh_bridge_refresh_slots(hh_bridge_t *bridge)
{
   int i;
   char savestate_base[HH_BRIDGE_PATH_MAX];
   char state_path[HH_BRIDGE_PATH_MAX];
   char thumbnail_path[HH_BRIDGE_PATH_MAX];
   struct stat state_stat;
   struct tm saved_time;

   if (!bridge)
      return;
   if (!runloop_get_savestate_path(savestate_base,
            sizeof(savestate_base), 0))
      savestate_base[0] = '\0';
   for (i = 0; i < HH_QUICK_MENU_SLOT_COUNT; i++)
   {
      hh_quick_menu_slot_t *slot = &bridge->menu.view.slots[i];
      memset(state_path, 0, sizeof(state_path));
      memset(thumbnail_path, 0, sizeof(thumbnail_path));
      slot->occupied = runloop_get_savestate_path(state_path,
            sizeof(state_path), i) && stat(state_path, &state_stat) == 0;
      slot->timestamp[0] = '\0';
      if (slot->occupied && rtime_localtime(&state_stat.st_mtime, &saved_time))
         strftime(slot->timestamp, sizeof(slot->timestamp),
               "%Y-%m-%d %H:%M", &saved_time);
      gfx_savestate_thumbnail_get_path(thumbnail_path,
            sizeof(thumbnail_path), savestate_base, i);
      slot->preview_available = slot->occupied
         && hh_bridge_file_exists(thumbnail_path);
   }
}
#endif

static void hh_bridge_refresh_recent(hh_bridge_t *bridge, bool refresh_rows)
{
   hh_quick_menu_view_t *view = &bridge->menu.view;
   hh_runtime_recent_game_t game;
   size_t row, first;

   view->recent_thumbnail[0] = '\0';
   view->recent_video[0] = '\0';
   if (hh_runtime_get_recent_count(&view->recent_count) != HH_OK)
      view->recent_count = 0;
   if (view->recent_selected >= view->recent_count)
      view->recent_selected = view->recent_count ? view->recent_count - 1 : 0;
   first = (size_t)hh_quick_menu_recent_scroll(view->recent_selected,
         view->recent_count);
   if (first)
      first--;
   if (bridge->menu.touch_active
         || (bridge->menu.touch_scrolled
            && bridge->menu.touch_scroll_page == HH_QUICK_MENU_PAGE_RECENT))
      first = view->recent_first;
   if (first != view->recent_first)
      refresh_rows = true;
   view->recent_first = first;
   if (refresh_rows)
      memset(view->recent, 0, sizeof(view->recent));
   for (row = 0; row < HH_QUICK_MENU_RECENT_ROWS
         && view->recent_first + row < view->recent_count; row++)
   {
      size_t index = view->recent_first + row;
      if (!refresh_rows && index != view->recent_selected)
         continue;
      view->recent[row].disabled = true;
      if (hh_runtime_get_recent_game(index, true,
               &game) != HH_OK)
         continue;
      strncpy(view->recent[row].title, game.title,
            sizeof(view->recent[row].title) - 1);
      view->recent[row].disabled = !game.available;
      strncpy(view->recent[row].thumbnail, game.thumbnail,
            sizeof(view->recent[row].thumbnail) - 1);
      strncpy(view->recent[row].video, game.video,
            sizeof(view->recent[row].video) - 1);
      if (index == view->recent_selected)
      {
         strncpy(view->recent_thumbnail, game.thumbnail,
               sizeof(view->recent_thumbnail) - 1);
         strncpy(view->recent_video, game.video,
               sizeof(view->recent_video) - 1);
      }
   }
}

static bool hh_bridge_refresh_controls(hh_bridge_t *bridge)
{
   hh_runtime_controls_t controls;
   hh_quick_menu_controls_t *out = &bridge->menu.view.controls;
   if (hh_runtime_get_controls(bridge->menu.view.control_player, &controls) != HH_OK)
   {
      bridge->menu.view.controls_valid = false;
      return false;
   }
   memcpy(out->masks, controls.masks, sizeof(out->masks));
   memcpy(out->periods, controls.periods, sizeof(out->periods));
   memcpy(out->custom, controls.custom, sizeof(out->custom));
   memcpy(out->available, controls.available, sizeof(out->available));
   memcpy(out->sources, controls.sources, sizeof(out->sources));
   memcpy(out->targets, controls.targets, sizeof(out->targets));
   memcpy(out->devices, controls.devices, sizeof(out->devices));
   memcpy(out->device_ids, controls.device_ids, sizeof(out->device_ids));
   out->device_count = controls.device_count;
   out->device_index = controls.device_index;
   out->player_count = controls.player_count;
   bridge->menu.view.controls_valid = true;
   return true;
}

static bool hh_bridge_refresh_shaders(hh_bridge_t *bridge, bool select_active)
{
   hh_runtime_shader_list_t list;
   hh_quick_menu_view_t *view = &bridge->menu.view;
   size_t i;
   if (hh_runtime_get_shader_list(view->shader_recommended, &list) != HH_OK)
      return false;
   view->shader_count = list.count;
   view->shader_active_id = list.active_id;
   view->shader_valid = list.valid;
   strncpy(view->shader_source, list.source, sizeof(view->shader_source) - 1);
   for (i = 0; i < list.count && i < HH_QUICK_MENU_SHADER_COUNT; i++)
   {
      view->shaders[i].id = list.entries[i].id;
      memcpy(view->shaders[i].name, list.entries[i].name, sizeof(view->shaders[i].name));
      memcpy(view->shaders[i].description, list.entries[i].description, sizeof(view->shaders[i].description));
      memcpy(view->shaders[i].detail, list.entries[i].detail, sizeof(view->shaders[i].detail));
      if (select_active && list.entries[i].id == list.active_id)
         view->shader_selected = i;
   }
   if (view->shader_selected >= view->shader_count)
      view->shader_selected = 0;
   if (select_active)
      view->shader_first = view->shader_selected >= HH_QUICK_MENU_SHADER_ROWS
         ? view->shader_selected - HH_QUICK_MENU_SHADER_ROWS + 1 : 0;
   for (i = 0; i < 3; i++)
      view->shader_removable[i] = list.removable[i];
   bridge->shader_observed_id = view->shader_count
      ? view->shaders[view->shader_selected].id : -1;
   return true;
}

static void hh_bridge_apply_snapshot(hh_bridge_t *bridge,
      const hh_runtime_snapshot_t *snapshot)
{
   hh_quick_menu_game_t game;
   hh_quick_menu_capabilities_t caps;
   if (!bridge || !snapshot)
      return;
   memset(&game, 0, sizeof(game));
   strncpy(game.title, snapshot->content_name, HH_QUICK_MENU_TEXT_MAX - 1);
   strncpy(game.platform, snapshot->core_name, HH_QUICK_MENU_TEXT_MAX - 1);
   strncpy(game.subtitle, snapshot->core_version, HH_QUICK_MENU_TEXT_MAX - 1);
   hh_quick_menu_set_game(&bridge->menu, &game);
   memset(&caps, 0, sizeof(caps));
   caps.save_enabled = hh_runtime_has_capability(HH_CAP_SAVE_STATE);
   caps.load_enabled = hh_runtime_has_capability(HH_CAP_LOAD_STATE);
   caps.screenshot_enabled = hh_runtime_has_capability(HH_CAP_SCREENSHOT);
   caps.advanced_menu_enabled = hh_runtime_has_capability(HH_CAP_RA_MENU);
   caps.reset_enabled = hh_runtime_has_capability(HH_CAP_RESET);
   caps.shader_enabled = hh_runtime_has_capability(HH_CAP_SHADER);
   hh_quick_menu_set_capabilities(&bridge->menu, &caps);
#if defined(HAVE_HANDHELD_RUNTIME) && HAVE_HANDHELD_RUNTIME
   hh_bridge_refresh_slots(bridge);
#endif
   if (bridge->menu.state == HH_QUICK_MENU_CLOSED
         || bridge->menu.view.page == HH_QUICK_MENU_PAGE_MAIN)
      bridge->menu.view.state_slot = snapshot->state_slot < 0
         ? 0 : snapshot->state_slot % HH_QUICK_MENU_SLOT_COUNT;
}

static const char *hh_bridge_error_message(hh_result_t result)
{
   switch (result)
   {
      case HH_ERR_SHADER_LOAD_FAILED: return "加载失败，已恢复上一效果";
      case HH_ERR_SHADER_RESTORE_FAILED: return "恢复失败，请重试或选择其他效果";
      case HH_ERR_BUSY: return "操作正在进行";
      case HH_ERR_NO_CONTENT: return "当前没有运行中的游戏";
      case HH_ERR_SAVE_FAILED: return "保存失败";
      case HH_ERR_LOAD_FAILED: return "读取失败";
      case HH_ERR_UNSUPPORTED: return "游戏核心不可用";
      case HH_ERR_INVALID_ARGUMENT: return "最近游戏记录已失效";
      default: return "操作失败";
   }
}

static const char *hh_bridge_shader_error_message(hh_result_t result)
{
   if (result == HH_ERR_UNSUPPORTED)
      return "效果文件缺失或设备不支持";
   if (result == HH_ERR_SAVE_FAILED)
      return "着色器设置保存失败";
   return hh_bridge_error_message(result);
}

static void hh_bridge_finish_pending(hh_bridge_t *bridge,
      hh_quick_menu_feedback_t feedback, hh_result_t result)
{
   if (!bridge)
      return;
   if (bridge->menu.state == HH_QUICK_MENU_ACTION_PENDING)
      hh_quick_menu_action_result(&bridge->menu, feedback,
            feedback == HH_QUICK_MENU_ACTION_ERROR
            ? hh_bridge_error_message(result) : NULL);
   hh_quick_menu_set_busy(&bridge->menu, false);
}

static void hh_bridge_event(const hh_runtime_event_t *event, void *userdata)
{
   hh_bridge_t *bridge = (hh_bridge_t*)userdata;
   hh_ui_action_t action;
   if (!bridge || !event)
      return;
   if (event->type == HH_EVENT_PAUSED && event->request_id == bridge->pause_request_id)
   {
      if (event->result != HH_OK)
      {
         bridge->pause_request_id = 0;
         bridge->pause_cancel_requested = false;
         if (bridge->pending_request_id == event->request_id)
         {
            bridge->pending_request_id = 0;
            bridge->pending_action = HH_UI_ACTION_NONE;
            hh_quick_menu_set_busy(&bridge->menu, false);
         }
         return;
      }
      bridge->quick_menu_owns_pause = true;
      bridge->pause_request_id = 0;
      if (bridge->pause_cancel_requested)
      {
         uint64_t request_id = 0;
         if (hh_runtime_submit_command(HH_CMD_RESUME, 0, &request_id) == HH_OK)
         {
            bridge->pending_request_id = request_id;
            bridge->pending_action = HH_UI_ACTION_CONTINUE;
         }
         else
         {
            bridge->pending_request_id = 0;
            bridge->pending_action = HH_UI_ACTION_NONE;
            hh_quick_menu_set_busy(&bridge->menu, false);
         }
         bridge->pause_cancel_requested = false;
      }
   }
   if (event->type == HH_EVENT_RESUMED && event->request_id == bridge->pending_request_id
         && event->result == HH_OK)
   {
      bridge->quick_menu_owns_pause = false;
      if (bridge->pending_action == HH_UI_ACTION_RESET)
      {
         if (hh_runtime_submit_command(HH_CMD_RESET, 0,
                  &bridge->pending_request_id) == HH_OK)
            return;
         hh_bridge_finish_pending(bridge, HH_QUICK_MENU_ACTION_ERROR,
               HH_ERR_RA_COMMAND_FAILED);
         bridge->pending_request_id = 0;
         bridge->pending_action = HH_UI_ACTION_NONE;
         return;
      }
      if (bridge->advanced_wait_resume)
      {
         bridge->advanced_wait_resume = false;
         if (hh_runtime_submit_command(HH_CMD_OPEN_RA_MENU, 0,
                  &bridge->pending_request_id) == HH_OK)
         {
            hh_quick_menu_close(&bridge->menu);
            hh_quick_menu_finish_close(&bridge->menu);
            return;
         }
         hh_bridge_finish_pending(bridge, HH_QUICK_MENU_ACTION_ERROR,
               HH_ERR_RA_COMMAND_FAILED);
         bridge->pending_request_id = 0;
         bridge->pending_action = HH_UI_ACTION_NONE;
         return;
      }
      bridge->pending_request_id = 0;
      bridge->pending_action = HH_UI_ACTION_NONE;
      hh_quick_menu_set_busy(&bridge->menu, false);
      hh_quick_menu_close(&bridge->menu);
      hh_quick_menu_finish_close(&bridge->menu);
   }
   if (event->type == HH_EVENT_CONTENT_CLOSED)
   {
      bridge->shader_session_active = false;
      bridge->menu.view.controls_dirty = false;
      bridge->menu.view.controls_valid = false;
      bridge->shader_close_after_cancel = false;
      if (bridge->pending_action == HH_UI_ACTION_EXIT)
         hh_bridge_finish_pending(bridge, HH_QUICK_MENU_ACTION_SUCCESS, HH_OK);
      else if (bridge->pending_action != HH_UI_ACTION_NONE)
         hh_bridge_finish_pending(bridge, HH_QUICK_MENU_ACTION_ERROR,
               HH_ERR_NO_CONTENT);
      bridge->pending_request_id = 0;
      bridge->pending_action = HH_UI_ACTION_NONE;
      hh_quick_menu_close(&bridge->menu);
      hh_quick_menu_finish_close(&bridge->menu);
      return;
   }
   if (!bridge->pending_request_id || event->request_id != bridge->pending_request_id)
      return;
   if (bridge->pending_action == HH_UI_ACTION_CONTROLS_SET
         || bridge->pending_action == HH_UI_ACTION_CONTROLS_SAVE
         || bridge->pending_action == HH_UI_ACTION_CONTROLS_DEVICE)
   {
      action = bridge->pending_action;
      bridge->pending_request_id = 0;
      bridge->pending_action = HH_UI_ACTION_NONE;
      hh_quick_menu_set_busy(&bridge->menu, false);
      if (event->result == HH_OK)
      {
         if (action == HH_UI_ACTION_CONTROLS_SET)
         {
            bridge->menu.view.page = HH_QUICK_MENU_PAGE_CONTROLS;
            bridge->menu.view.control_selected = bridge->menu.view.control_source + 2;
            bridge->menu.view.control_first = bridge->menu.view.control_selected >= HH_QUICK_MENU_CONTROL_ROWS
               ? bridge->menu.view.control_selected - HH_QUICK_MENU_CONTROL_ROWS + 1 : 0;
            bridge->menu.view.controls_dirty = true;
         }
         else if (action == HH_UI_ACTION_CONTROLS_SAVE)
            bridge->menu.view.controls_dirty = false;
         else
            bridge->menu.view.controls_dirty = true;
         hh_bridge_refresh_controls(bridge);
      }
      hh_quick_menu_action_result(&bridge->menu,
            event->result == HH_OK ? HH_QUICK_MENU_ACTION_SUCCESS : HH_QUICK_MENU_ACTION_ERROR,
            event->result != HH_OK ? "按键配置操作失败" :
            action == HH_UI_ACTION_CONTROLS_SAVE ? "已保存；手柄分配沿用全局设置" :
            action == HH_UI_ACTION_CONTROLS_DEVICE ? "手柄已分配；原玩家交换设备" : "按键已应用，可保存到游戏");
      return;
   }
   if (bridge->pending_action == HH_UI_ACTION_SHADER_BEGIN
         || bridge->pending_action == HH_UI_ACTION_SHADER_APPLY
         || bridge->pending_action == HH_UI_ACTION_SHADER_CANCEL
         || (bridge->shader_session_active && bridge->pending_action == HH_UI_ACTION_NONE))
   {
      action = bridge->pending_action;
      bridge->pending_request_id = 0;
      bridge->pending_action = HH_UI_ACTION_NONE;
      hh_quick_menu_set_busy(&bridge->menu, false);
      if (event->result != HH_OK)
      {
         hh_quick_menu_action_result(&bridge->menu, HH_QUICK_MENU_ACTION_ERROR,
               hh_bridge_shader_error_message(event->result));
         if (bridge->shader_session_active)
         {
            hh_bridge_refresh_shaders(bridge, true);
            bridge->menu.view.feedback = HH_QUICK_MENU_ACTION_ERROR;
            strncpy(bridge->menu.view.message, hh_bridge_shader_error_message(event->result),
                  sizeof(bridge->menu.view.message) - 1);
         }
         bridge->shader_close_after_cancel = false;
         return;
      }
      if (action == HH_UI_ACTION_SHADER_BEGIN)
      {
         bridge->shader_session_active = true;
         bridge->menu.view.shader_recommended = true;
         bridge->menu.view.shader_selected = 0;
         bridge->menu.view.shader_first = 0;
         bridge->menu.view.shader_fullscreen = false;
         bridge->menu.view.page = HH_QUICK_MENU_PAGE_SHADER;
         hh_bridge_refresh_shaders(bridge, true);
      }
      else if (action == HH_UI_ACTION_SHADER_CANCEL || action == HH_UI_ACTION_SHADER_APPLY)
      {
         bridge->shader_session_active = false;
         bridge->menu.view.page = HH_QUICK_MENU_PAGE_MAIN;
         bridge->menu.view.shader_fullscreen = false;
         bridge->menu.open_generation++;
      }
      else
         hh_bridge_refresh_shaders(bridge, false);
      hh_quick_menu_action_result(&bridge->menu, HH_QUICK_MENU_ACTION_SUCCESS,
            action == HH_UI_ACTION_SHADER_APPLY ? "着色器设置已应用" : NULL);
   if (action == HH_UI_ACTION_SHADER_BEGIN || action == HH_UI_ACTION_SHADER_CANCEL)
         hh_quick_menu_clear_feedback(&bridge->menu);
      if (bridge->shader_close_after_cancel)
      {
         bridge->shader_close_after_cancel = false;
         hh_bridge_close(bridge);
      }
      return;
   }
   if (bridge->pending_action == HH_UI_ACTION_RECENT
         && event->type == HH_EVENT_CONTENT_LOADED && event->result == HH_OK)
   {
      hh_bridge_finish_pending(bridge, HH_QUICK_MENU_ACTION_SUCCESS, HH_OK);
      bridge->pending_request_id = 0;
      bridge->pending_action = HH_UI_ACTION_NONE;
      bridge->quick_menu_owns_pause = false;
      bridge->pause_request_id = 0;
      hh_quick_menu_close(&bridge->menu);
      hh_quick_menu_finish_close(&bridge->menu);
      return;
   }
   if ((bridge->pending_action == HH_UI_ACTION_SAVE
            && event->type == HH_EVENT_STATE_SAVE_COMPLETED)
         || (bridge->pending_action == HH_UI_ACTION_LOAD
            && event->type == HH_EVENT_STATE_LOAD_COMPLETED))
   {
      if (bridge->pending_action == HH_UI_ACTION_SAVE
            && event->result == HH_OK
            && bridge->pending_slot >= 0
            && bridge->pending_slot < HH_QUICK_MENU_SLOT_COUNT)
      {
         hh_quick_menu_slot_t slot = bridge->menu.view.slots[bridge->pending_slot];
         slot.index = bridge->pending_slot;
         slot.occupied = true;
         slot.disabled = false;
         hh_quick_menu_set_slot(&bridge->menu, &slot);
#if defined(HAVE_HANDHELD_RUNTIME) && HAVE_HANDHELD_RUNTIME
         hh_bridge_refresh_slots(bridge);
#endif
      }
      hh_quick_menu_action_result(&bridge->menu,
            event->result == HH_OK
            ? (bridge->pending_action == HH_UI_ACTION_SAVE
               ? HH_QUICK_MENU_SAVE_SUCCESS : HH_QUICK_MENU_LOAD_SUCCESS)
            : HH_QUICK_MENU_ACTION_ERROR,
            event->result == HH_OK ? NULL : hh_bridge_error_message(event->result));
      if (bridge->pending_action == HH_UI_ACTION_LOAD
            && event->result == HH_OK)
      {
         bridge->pending_request_id = 0;
         bridge->pending_action = HH_UI_ACTION_NONE;
         hh_bridge_close(bridge);
         return;
      }
      bridge->pending_request_id = 0;
      bridge->pending_action = HH_UI_ACTION_NONE;
      return;
   }
   if (event->type == HH_EVENT_ERROR || event->result != HH_OK)
   {
      hh_bridge_finish_pending(bridge, HH_QUICK_MENU_ACTION_ERROR,
            event->result);
      bridge->pending_request_id = 0;
      bridge->pending_action = HH_UI_ACTION_NONE;
      bridge->advanced_wait_resume = false;
      return;
   }
   action = bridge->pending_action;
   if (action == HH_UI_ACTION_CONTINUE || action == HH_UI_ACTION_RESET
         || action == HH_UI_ACTION_ADVANCED_MENU || action == HH_UI_ACTION_EXIT)
   {
      hh_quick_menu_action_result(&bridge->menu,
            HH_QUICK_MENU_ACTION_SUCCESS, NULL);
      hh_quick_menu_close(&bridge->menu);
      hh_quick_menu_finish_close(&bridge->menu);
      bridge->pending_request_id = 0;
      bridge->pending_action = HH_UI_ACTION_NONE;
   }
}

void hh_bridge_init(hh_bridge_t *bridge)
{
   if (!bridge)
      return;
   memset(bridge, 0, sizeof(*bridge));
   hh_quick_menu_init(&bridge->menu);
   bridge->pending_action = HH_UI_ACTION_NONE;
   bridge->pending_slot = -1;
   bridge->initialized = hh_runtime_init() == HH_OK;
   if (bridge->initialized)
      hh_runtime_set_event_callback(hh_bridge_event, bridge);
   if (bridge->initialized)
      hh_active_bridge = bridge;
}

void hh_bridge_deinit(hh_bridge_t *bridge)
{
   if (!bridge || !bridge->initialized)
      return;
   hh_runtime_set_event_callback(NULL, NULL);
   hh_runtime_deinit();
   bridge->initialized = false;
   if (hh_active_bridge == bridge)
      hh_active_bridge = NULL;
}

bool hh_bridge_open(hh_bridge_t *bridge)
{
   hh_runtime_snapshot_t snapshot;
   uint64_t request_id = 0;
   if (!bridge || !bridge->initialized
         || hh_runtime_get_snapshot(&snapshot) != HH_OK
         || !snapshot.content_loaded)
      return false;
   hh_bridge_apply_snapshot(bridge, &snapshot);
   hh_quick_menu_open(&bridge->menu);
   hh_quick_menu_finish_open(&bridge->menu);
   bridge->quick_menu_owns_pause = false;
   bridge->pause_request_id = 0;
   bridge->pending_request_id = 0;
   bridge->pending_action = HH_UI_ACTION_NONE;
   bridge->pending_slot = -1;
   bridge->advanced_wait_resume = false;
   bridge->pause_cancel_requested = false;
   if (!snapshot.paused && hh_runtime_submit_command(HH_CMD_PAUSE, 0, &request_id) == HH_OK)
      bridge->pause_request_id = request_id;
   return true;
}

void hh_bridge_close(hh_bridge_t *bridge)
{
   uint64_t request_id = 0;
   if (!bridge)
      return;
   if (bridge->menu.state == HH_QUICK_MENU_ACTION_PENDING)
      return;
   if (bridge->shader_session_active)
   {
      if (hh_runtime_submit_command(HH_CMD_SHADER_CANCEL, 0, &request_id) == HH_OK)
      {
         bridge->shader_close_after_cancel = true;
         bridge->pending_request_id = request_id;
         bridge->pending_action = HH_UI_ACTION_SHADER_CANCEL;
         bridge->menu.state = HH_QUICK_MENU_ACTION_PENDING;
         bridge->menu.view.pending_action = HH_UI_ACTION_SHADER_CANCEL;
         hh_quick_menu_set_busy(&bridge->menu, true);
      }
      return;
   }
   if (bridge->quick_menu_owns_pause)
   {
      if (hh_runtime_submit_command(HH_CMD_RESUME, 0, &request_id) == HH_OK)
      {
         hh_quick_menu_set_busy(&bridge->menu, true);
         bridge->pending_request_id = request_id;
         bridge->pending_action = HH_UI_ACTION_CONTINUE;
         return;
      }
      return;
   }
   else if (bridge->pause_request_id)
   {
      bridge->pause_cancel_requested = true;
      bridge->pending_request_id = bridge->pause_request_id;
      bridge->pending_action = HH_UI_ACTION_CONTINUE;
      hh_quick_menu_set_busy(&bridge->menu, true);
      return;
   }
   hh_quick_menu_close(&bridge->menu);
   hh_quick_menu_finish_close(&bridge->menu);
}

bool hh_bridge_is_open(const hh_bridge_t *bridge)
{
   return bridge && bridge->menu.state != HH_QUICK_MENU_CLOSED;
}

bool hh_bridge_input(hh_bridge_t *bridge, hh_quick_menu_input_t input)
{
   hh_ui_action_t action;
   hh_result_t result;
   uint64_t request_id = 0;
   hh_quick_menu_page_t previous_page;
   size_t previous_first, previous_selected;
   bool previous_recommended;
   if (!bridge || !hh_bridge_is_open(bridge))
      return false;
   if (input == HH_QUICK_MENU_INPUT_BACK
         && bridge->menu.state == HH_QUICK_MENU_OPEN
         && bridge->menu.view.page == HH_QUICK_MENU_PAGE_MAIN)
   {
      hh_bridge_close(bridge);
      return true;
   }
   previous_recommended = bridge->menu.view.shader_recommended;
   previous_page = bridge->menu.view.page;
   previous_first = bridge->menu.view.recent_first;
   previous_selected = bridge->menu.view.recent_selected;
   action = hh_quick_menu_input(&bridge->menu, input);
   if (action == HH_UI_ACTION_NONE)
   {
      if (bridge->menu.view.page == HH_QUICK_MENU_PAGE_CONTROLS)
         hh_bridge_refresh_controls(bridge);
      if (bridge->shader_session_active
            && previous_recommended != bridge->menu.view.shader_recommended)
         hh_bridge_refresh_shaders(bridge, true);
      if (bridge->menu.view.page == HH_QUICK_MENU_PAGE_RECENT
            && bridge->menu.state == HH_QUICK_MENU_OPEN
            && (previous_page != HH_QUICK_MENU_PAGE_RECENT
               || previous_selected != bridge->menu.view.recent_selected))
         hh_bridge_refresh_recent(bridge,
               previous_page != HH_QUICK_MENU_PAGE_RECENT
               || previous_first != bridge->menu.view.recent_first);
      return true;
   }
   if (action == HH_UI_ACTION_CONTROLS_BEGIN)
   {
      bridge->menu.view.control_player = 0;
      if (hh_bridge_refresh_controls(bridge))
      {
         bridge->menu.view.page = HH_QUICK_MENU_PAGE_CONTROLS;
         bridge->menu.view.control_selected = bridge->menu.view.control_first = 0;
         hh_quick_menu_action_result(&bridge->menu, HH_QUICK_MENU_ACTION_SUCCESS, NULL);
         hh_quick_menu_clear_feedback(&bridge->menu);
      }
      else
         hh_quick_menu_action_result(&bridge->menu, HH_QUICK_MENU_ACTION_ERROR,
               "当前核心不支持手柄按键配置");
      return true;
   }
   if (action == HH_UI_ACTION_CONTROLS_SET || action == HH_UI_ACTION_CONTROLS_SAVE
         || action == HH_UI_ACTION_CONTROLS_DEVICE)
   {
      hh_quick_menu_view_t *view = &bridge->menu.view;
      unsigned argument = view->control_player << 20;
      hh_command_type_t command;
      if (action == HH_UI_ACTION_CONTROLS_SET)
      {
         argument |= view->control_source << 16;
         if (view->control_mode)
            argument |= view->control_mask;
         if (view->control_mode == 2)
            argument |= view->control_period << 24;
         command = HH_CMD_CONTROLS_SET;
      }
      else if (action == HH_UI_ACTION_CONTROLS_DEVICE)
      {
         argument |= view->control_device;
         command = HH_CMD_CONTROLS_DEVICE;
      }
      else
      {
         argument = view->control_save_scope;
         command = HH_CMD_CONTROLS_SAVE;
      }
      result = hh_runtime_submit_command(command, (int)argument, &request_id);
      if (result == HH_OK)
      {
         bridge->pending_request_id = request_id;
         bridge->pending_action = action;
         hh_quick_menu_set_busy(&bridge->menu, true);
      }
      else
         hh_quick_menu_action_result(&bridge->menu, HH_QUICK_MENU_ACTION_ERROR,
               "按键配置操作失败");
      return true;
   }
   if (action == HH_UI_ACTION_SHADER_BEGIN || action == HH_UI_ACTION_SHADER_CANCEL
         || action == HH_UI_ACTION_SHADER_APPLY)
   {
      hh_command_type_t command = action == HH_UI_ACTION_SHADER_BEGIN ? HH_CMD_SHADER_BEGIN :
         action == HH_UI_ACTION_SHADER_CANCEL ? HH_CMD_SHADER_CANCEL :
         bridge->menu.view.shader_scope >= 4 ? HH_CMD_SHADER_REMOVE : HH_CMD_SHADER_APPLY;
      int scope = (int)bridge->menu.view.shader_scope;
      result = hh_runtime_submit_command(command, scope >= 4 ? scope - 3 : scope, &request_id);
      if (result == HH_OK)
      {
         bridge->pending_request_id = request_id;
         bridge->pending_action = action;
         hh_quick_menu_set_busy(&bridge->menu, true);
      }
      else
         hh_quick_menu_action_result(&bridge->menu, HH_QUICK_MENU_ACTION_ERROR,
               hh_bridge_shader_error_message(result));
      return true;
   }
   if (action == HH_UI_ACTION_RECENT)
   {
      result = bridge->menu.view.recent_selected > INT_MAX
         ? HH_ERR_INVALID_ARGUMENT
         : hh_runtime_submit_command(HH_CMD_LOAD_RECENT,
               (int)bridge->menu.view.recent_selected, &request_id);
      if (result == HH_OK)
      {
         bridge->pending_request_id = request_id;
         bridge->pending_action = action;
      }
      else
         hh_quick_menu_action_result(&bridge->menu,
               HH_QUICK_MENU_ACTION_ERROR, hh_bridge_error_message(result));
      return true;
   }
   if (action == HH_UI_ACTION_CONTINUE)
   {
      if (bridge->quick_menu_owns_pause || bridge->pause_request_id
            || bridge->pause_cancel_requested)
      {
         if (hh_runtime_submit_command(HH_CMD_RESUME, 0, &request_id) == HH_OK)
         {
            bridge->pause_request_id = 0;
            bridge->pause_cancel_requested = false;
            bridge->pending_request_id = request_id;
            bridge->pending_action = HH_UI_ACTION_CONTINUE;
            hh_quick_menu_set_busy(&bridge->menu, true);
         }
      }
      else
         hh_bridge_close(bridge);
      return true;
   }
   if (action == HH_UI_ACTION_SAVE || action == HH_UI_ACTION_LOAD)
   {
      result = hh_runtime_submit_command(HH_CMD_SET_STATE_SLOT,
            bridge->menu.view.state_slot, &request_id);
      if (result != HH_OK)
      {
         hh_quick_menu_action_result(&bridge->menu,
               HH_QUICK_MENU_ACTION_ERROR, hh_bridge_error_message(result));
         return true;
      }
      bridge->pending_slot = bridge->menu.view.state_slot;
      result = hh_runtime_submit_command(action == HH_UI_ACTION_SAVE
            ? HH_CMD_SAVE_STATE : HH_CMD_LOAD_STATE, 0, &request_id);
      if (result != HH_OK)
      {
         hh_quick_menu_action_result(&bridge->menu,
               HH_QUICK_MENU_ACTION_ERROR, hh_bridge_error_message(result));
         return true;
      }
      bridge->pending_request_id = request_id;
      bridge->pending_action = action;
      return true;
   }
   if (action == HH_UI_ACTION_RESET || action == HH_UI_ACTION_EXIT)
   {
      result = hh_runtime_submit_command(action == HH_UI_ACTION_RESET
            ? ((bridge->quick_menu_owns_pause || bridge->pause_request_id)
               ? HH_CMD_RESUME : HH_CMD_RESET)
            : HH_CMD_QUIT, 0, &request_id);
      if (result == HH_OK)
      {
         bridge->pending_request_id = request_id;
         bridge->pending_action = action;
      }
      else
         hh_quick_menu_action_result(&bridge->menu,
               HH_QUICK_MENU_ACTION_ERROR, hh_bridge_error_message(result));
      return true;
   }
   if (action == HH_UI_ACTION_ADVANCED_MENU)
   {
      if (bridge->quick_menu_owns_pause)
      {
         if (hh_runtime_submit_command(HH_CMD_RESUME, 0, &request_id) == HH_OK)
         {
            hh_quick_menu_set_busy(&bridge->menu, true);
            bridge->pending_request_id = request_id;
            bridge->pending_action = action;
            bridge->advanced_wait_resume = true;
         }
         return true;
      }
      if (hh_runtime_submit_command(HH_CMD_OPEN_RA_MENU, 0, &request_id) == HH_OK)
      {
         bridge->pending_request_id = request_id;
         bridge->pending_action = action;
         hh_quick_menu_close(&bridge->menu);
         hh_quick_menu_finish_close(&bridge->menu);
      }
      else
         hh_quick_menu_action_result(&bridge->menu,
               HH_QUICK_MENU_ACTION_ERROR, "高级菜单打开失败");
      return true;
   }
   return true;
}

bool hh_bridge_touch(hh_bridge_t *bridge, float x, float y, bool pressed,
      float width, float height)
{
   hh_quick_menu_input_t input;
   size_t previous_first, previous_selected;
   if (!bridge || !hh_bridge_is_open(bridge))
      return false;
   previous_first = bridge->menu.view.recent_first;
   previous_selected = bridge->menu.view.recent_selected;
   if (!hh_quick_menu_touch(&bridge->menu, x, y, pressed,
            width, height, &input))
      return false;
   if (!pressed && input != HH_QUICK_MENU_INPUT_NONE)
      hh_bridge_input(bridge, input);
   else if (bridge->menu.view.page == HH_QUICK_MENU_PAGE_RECENT
         && (previous_first != bridge->menu.view.recent_first
            || previous_selected != bridge->menu.view.recent_selected))
      hh_bridge_refresh_recent(bridge, previous_first != bridge->menu.view.recent_first);
   return true;
}

void hh_bridge_tick(hh_bridge_t *bridge)
{
   hh_runtime_snapshot_t snapshot;
   uint64_t request_id = 0;
   if (!bridge || !bridge->initialized)
      return;
   if (hh_test_input_pending && hh_active_bridge == bridge)
   {
      hh_quick_menu_input_t input = hh_test_input_value;
      hh_test_input_pending = false;
      hh_bridge_input(bridge, input);
   }
   if (hh_test_toggle_pending && hh_active_bridge == bridge)
   {
      hh_test_toggle_pending = false;
      if (hh_bridge_is_open(bridge))
         hh_bridge_close(bridge);
      else
         hh_bridge_open(bridge);
   }
   if (hh_runtime_get_snapshot(&snapshot) == HH_OK)
   {
      if (snapshot.paused && bridge->pause_request_id)
      {
         bridge->pause_request_id = 0;
         bridge->quick_menu_owns_pause = true;
         if (bridge->pause_cancel_requested
               && hh_runtime_submit_command(HH_CMD_RESUME, 0, &request_id) == HH_OK)
         {
            bridge->pause_cancel_requested = false;
            bridge->pending_request_id = request_id;
            bridge->pending_action = HH_UI_ACTION_CONTINUE;
            hh_quick_menu_set_busy(&bridge->menu, true);
         }
      }
      if (snapshot.paused
            && bridge->pending_action == HH_UI_ACTION_CONTINUE
            && !bridge->pending_request_id
            && hh_runtime_submit_command(HH_CMD_RESUME, 0, &request_id) == HH_OK)
      {
         bridge->pending_request_id = request_id;
         hh_quick_menu_set_busy(&bridge->menu, true);
      }
      if (!snapshot.paused
            && bridge->menu.view.pending_action == HH_UI_ACTION_CONTINUE
            && !bridge->pending_request_id)
      {
         hh_quick_menu_action_result(&bridge->menu,
               HH_QUICK_MENU_ACTION_SUCCESS, NULL);
         bridge->pending_action = HH_UI_ACTION_NONE;
         hh_quick_menu_set_busy(&bridge->menu, false);
         hh_quick_menu_close(&bridge->menu);
         hh_quick_menu_finish_close(&bridge->menu);
      }
      hh_bridge_apply_snapshot(bridge, &snapshot);
      if (bridge->shader_session_active && snapshot.content_loaded
            && bridge->menu.view.page == HH_QUICK_MENU_PAGE_SHADER
            && bridge->menu.state == HH_QUICK_MENU_OPEN && !bridge->pending_request_id
            && !bridge->menu.touch_active
            && bridge->menu.view.shader_selected < bridge->menu.view.shader_count)
      {
         int id = bridge->menu.view.shaders[bridge->menu.view.shader_selected].id;
         unsigned long now = bridge->menu.animation_time_ms;
         if (id != bridge->shader_observed_id)
         {
            bridge->shader_observed_id = id;
            bridge->shader_preview_time_ms = now;
         }
         if (id != bridge->menu.view.shader_active_id
               && now - bridge->shader_preview_time_ms >= 250)
         {
            hh_result_t result = hh_runtime_submit_command(HH_CMD_SHADER_PREVIEW, id, &request_id);
            if (result == HH_OK)
            {
               bridge->pending_request_id = request_id;
               bridge->pending_action = HH_UI_ACTION_NONE;
               hh_quick_menu_set_busy(&bridge->menu, true);
            }
            else
            {
               bridge->menu.view.feedback = HH_QUICK_MENU_ACTION_ERROR;
               strncpy(bridge->menu.view.message, hh_bridge_error_message(result),
                     sizeof(bridge->menu.view.message) - 1);
               bridge->shader_preview_time_ms = now;
            }
         }
      }
      if (hh_bridge_is_open(bridge) && !snapshot.content_loaded)
         hh_bridge_close(bridge);
   }
}

bool hh_bridge_test_input(hh_bridge_t *bridge, hh_quick_menu_input_t input)
{
   if (!bridge || bridge != hh_active_bridge || hh_test_input_pending)
      return false;
   hh_test_input_value = input;
   hh_test_input_pending = true;
   return true;
}

bool hh_bridge_test_toggle(hh_bridge_t *bridge)
{
   if (!bridge || bridge != hh_active_bridge || hh_test_toggle_pending)
      return false;
   hh_test_toggle_pending = true;
   return true;
}

const hh_quick_menu_t *hh_bridge_menu(const hh_bridge_t *bridge)
{
   return bridge ? &bridge->menu : NULL;
}

hh_bridge_t *hh_bridge_active(void)
{
   return hh_active_bridge;
}
