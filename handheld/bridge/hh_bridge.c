#include "hh_bridge.h"

#include <string.h>

static hh_bridge_t *hh_active_bridge;
static volatile bool hh_test_input_pending;
static volatile hh_quick_menu_input_t hh_test_input_value;
static volatile bool hh_test_toggle_pending;

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
   hh_quick_menu_set_capabilities(&bridge->menu, &caps);
   if (bridge->menu.state == HH_QUICK_MENU_CLOSED
         || bridge->menu.view.page == HH_QUICK_MENU_PAGE_MAIN)
      bridge->menu.view.state_slot = snapshot->state_slot < 0
         ? 0 : snapshot->state_slot % HH_QUICK_MENU_SLOT_COUNT;
}

static const char *hh_bridge_error_message(hh_result_t result)
{
   switch (result)
   {
      case HH_ERR_BUSY: return "操作正在进行";
      case HH_ERR_NO_CONTENT: return "当前没有运行中的游戏";
      case HH_ERR_SAVE_FAILED: return "保存失败";
      case HH_ERR_LOAD_FAILED: return "读取失败";
      default: return "操作失败";
   }
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
   if (!bridge || !hh_bridge_is_open(bridge))
      return false;
   if (input == HH_QUICK_MENU_INPUT_BACK
         && bridge->menu.state == HH_QUICK_MENU_OPEN
         && bridge->menu.view.page == HH_QUICK_MENU_PAGE_MAIN)
   {
      hh_bridge_close(bridge);
      return true;
   }
   action = hh_quick_menu_input(&bridge->menu, input);
   if (action == HH_UI_ACTION_NONE)
      return true;
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
            ? HH_CMD_RESET : HH_CMD_CLOSE_CONTENT, 0, &request_id);
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
