#include "hh_quick_menu_internal.h"

#include <string.h>

static hh_ui_action_t hh_quick_menu_selected_action(
      const hh_quick_menu_t *menu)
{
   switch (menu->view.items[menu->view.selected_index].id)
   {
      case HH_QUICK_MENU_ITEM_CONTROLS: return HH_UI_ACTION_CONTROLS_BEGIN;
      case HH_QUICK_MENU_ITEM_CONTINUE:       return HH_UI_ACTION_CONTINUE;
      case HH_QUICK_MENU_ITEM_SAVE:           return HH_UI_ACTION_SAVE;
      case HH_QUICK_MENU_ITEM_LOAD:           return HH_UI_ACTION_LOAD;
      case HH_QUICK_MENU_ITEM_RESET:          return HH_UI_ACTION_RESET;
      case HH_QUICK_MENU_ITEM_ADVANCED_MENU:  return HH_UI_ACTION_ADVANCED_MENU;
      case HH_QUICK_MENU_ITEM_EXIT:           return HH_UI_ACTION_EXIT;
      case HH_QUICK_MENU_ITEM_SHADER: return HH_UI_ACTION_SHADER_BEGIN;
      case HH_QUICK_MENU_ITEM_RECENT:         return HH_UI_ACTION_RECENT;
      default:                                return HH_UI_ACTION_NONE;
   }
}

static bool hh_quick_menu_requires_confirm(hh_ui_action_t action)
{
   return action == HH_UI_ACTION_RESET || action == HH_UI_ACTION_EXIT;
}

static void hh_quick_menu_show_dialog(hh_quick_menu_t *menu,
      hh_ui_action_t action)
{
   menu->state = HH_QUICK_MENU_CONFIRM_DIALOG;
   menu->view.dialog.visible = true;
   menu->view.dialog.action = action;
   menu->view.dialog.confirm_selected = false;
   if (action == HH_UI_ACTION_RESET)
      strncpy(menu->view.dialog.message, "重新开始游戏？",
            HH_QUICK_MENU_MESSAGE_MAX - 1);
   else
      strncpy(menu->view.dialog.message, "退出当前游戏？",
            HH_QUICK_MENU_MESSAGE_MAX - 1);
   menu->view.dialog.message[HH_QUICK_MENU_MESSAGE_MAX - 1] = '\0';
   strcpy(menu->view.dialog.detail, "未保存的游戏进度可能丢失。");
   strcpy(menu->view.dialog.confirm_label,
         action == HH_UI_ACTION_RESET ? "重新开始" : "退出游戏");
   hh_quick_menu_clear_feedback(menu);
}

static hh_ui_action_t hh_quick_menu_begin_action(hh_quick_menu_t *menu,
      hh_ui_action_t action)
{
   if (action == HH_UI_ACTION_NONE)
      return action;
   hh_quick_menu_clear_feedback(menu);
   menu->view.pending_action = action;
   menu->view.pending_slot = action == HH_UI_ACTION_SAVE
      || action == HH_UI_ACTION_LOAD ? menu->view.state_slot : -1;
   menu->state = HH_QUICK_MENU_ACTION_PENDING;
   return action;
}

hh_ui_action_t hh_quick_menu_confirm(hh_quick_menu_t *menu)
{
   hh_ui_action_t action;

   if (!menu || menu->state != HH_QUICK_MENU_OPEN || menu->view.busy
         || menu->view.dialog.visible || !menu->view.item_count
         || menu->view.selected_index >= menu->view.item_count
         || menu->view.selected_index >= HH_QUICK_MENU_ITEM_COUNT)
      return HH_UI_ACTION_NONE;
   if (hh_quick_menu_item_state(menu, menu->view.selected_index)
         == HH_QUICK_MENU_ITEM_DISABLED)
      return HH_UI_ACTION_NONE;
   if (hh_quick_menu_is_controls_page(menu))
      return hh_quick_menu_controls_input(menu, HH_QUICK_MENU_INPUT_CONFIRM);
   if (hh_quick_menu_is_shader_page(menu))
   {
      if (menu->view.shader_fullscreen)
      {
         menu->view.shader_fullscreen = false;
         return HH_UI_ACTION_NONE;
      }
      if (!menu->view.shader_valid || menu->view.shader_selected >= menu->view.shader_count)
         return HH_UI_ACTION_NONE;
      if (menu->view.page == HH_QUICK_MENU_PAGE_SHADER_SCOPE)
      {
         if (menu->view.shader_scope >= 4
               && !menu->view.shader_removable[menu->view.shader_scope - 4])
            return HH_UI_ACTION_NONE;
         return hh_quick_menu_begin_action(menu, HH_UI_ACTION_SHADER_APPLY);
      }
      if (menu->view.shaders[menu->view.shader_selected].id != menu->view.shader_active_id)
         return HH_UI_ACTION_NONE;
      menu->view.page = HH_QUICK_MENU_PAGE_SHADER_SCOPE;
      menu->view.shader_scope = 0;
      hh_quick_menu_clear_feedback(menu);
      return HH_UI_ACTION_NONE;
   }
   if (menu->view.page == HH_QUICK_MENU_PAGE_RECENT)
   {
      size_t row = menu->view.recent_selected - menu->view.recent_first;
      if (menu->view.recent_selected >= menu->view.recent_count
            || row >= HH_QUICK_MENU_RECENT_ROWS
            || menu->view.recent[row].disabled)
         return HH_UI_ACTION_NONE;
      return hh_quick_menu_begin_action(menu, HH_UI_ACTION_RECENT);
   }
   if (menu->view.page != HH_QUICK_MENU_PAGE_MAIN)
   {
      if (hh_quick_menu_slot_disabled(menu, menu->view.state_slot))
         return HH_UI_ACTION_NONE;
      return hh_quick_menu_begin_action(menu,
            menu->view.page == HH_QUICK_MENU_PAGE_SAVE
            ? HH_UI_ACTION_SAVE : HH_UI_ACTION_LOAD);
   }
   action = hh_quick_menu_selected_action(menu);
   if (action == HH_UI_ACTION_SHADER_BEGIN || action == HH_UI_ACTION_CONTROLS_BEGIN)
      return hh_quick_menu_begin_action(menu, action);
   if (action == HH_UI_ACTION_RECENT)
   {
      menu->view.page = HH_QUICK_MENU_PAGE_RECENT;
      menu->view.recent_selected = 0;
      menu->view.recent_first = 0;
      menu->slot_scroll = menu->slot_scroll_origin = menu->slot_scroll_target = 0;
      menu->slot_scroll_elapsed = 0;
      menu->animation_time_ms = 0;
      hh_quick_menu_clear_feedback(menu);
      return HH_UI_ACTION_NONE;
   }
   if (action == HH_UI_ACTION_SAVE || action == HH_UI_ACTION_LOAD)
   {
      menu->view.page = action == HH_UI_ACTION_SAVE
         ? HH_QUICK_MENU_PAGE_SAVE : HH_QUICK_MENU_PAGE_LOAD;
      menu->slot_scroll = hh_quick_menu_slot_scroll(menu->view.state_slot);
      menu->slot_scroll_origin = menu->slot_scroll;
      menu->slot_scroll_target = menu->slot_scroll;
      menu->slot_scroll_elapsed = 0;
      menu->animation_time_ms = 0;
      hh_quick_menu_clear_feedback(menu);
      return HH_UI_ACTION_NONE;
   }
   if (hh_quick_menu_requires_confirm(action))
   {
      hh_quick_menu_show_dialog(menu, action);
      return HH_UI_ACTION_NONE;
   }
   return hh_quick_menu_begin_action(menu, action);
}

void hh_quick_menu_action_complete(hh_quick_menu_t *menu)
{
   hh_quick_menu_action_result(menu, HH_QUICK_MENU_ACTION_SUCCESS, NULL);
}

void hh_quick_menu_clear_feedback(hh_quick_menu_t *menu)
{
   if (!menu)
      return;
   menu->view.feedback = HH_QUICK_MENU_FEEDBACK_NONE;
   menu->view.message[0] = '\0';
}

void hh_quick_menu_action_result(hh_quick_menu_t *menu,
      hh_quick_menu_feedback_t feedback, const char *message)
{
   if (!menu || menu->state != HH_QUICK_MENU_ACTION_PENDING
         || feedback < HH_QUICK_MENU_SAVE_SUCCESS
         || feedback > HH_QUICK_MENU_ACTION_ERROR)
      return;
   if (!message || !*message)
   {
      switch (feedback)
      {
         case HH_QUICK_MENU_SAVE_SUCCESS: message = "已保存"; break;
         case HH_QUICK_MENU_LOAD_SUCCESS: message = "读取成功"; break;
         case HH_QUICK_MENU_ACTION_ERROR: message = "操作失败"; break;
         default: message = "操作成功"; break;
      }
   }
   menu->view.feedback = feedback;
   hh_quick_menu_copy_text(menu->view.message, HH_QUICK_MENU_MESSAGE_MAX, message);
   menu->view.pending_action = HH_UI_ACTION_NONE;
   menu->view.pending_slot = -1;
   menu->state = HH_QUICK_MENU_OPEN;
}

void hh_quick_menu_dialog_move_left(hh_quick_menu_t *menu)
{
   if (menu && menu->state == HH_QUICK_MENU_CONFIRM_DIALOG && !menu->view.busy)
      menu->view.dialog.confirm_selected = false;
}

void hh_quick_menu_dialog_move_right(hh_quick_menu_t *menu)
{
   if (menu && menu->state == HH_QUICK_MENU_CONFIRM_DIALOG && !menu->view.busy)
      menu->view.dialog.confirm_selected = true;
}

hh_ui_action_t hh_quick_menu_dialog_confirm(hh_quick_menu_t *menu)
{
   hh_ui_action_t action;

   if (!menu || menu->state != HH_QUICK_MENU_CONFIRM_DIALOG
         || !menu->view.dialog.visible || menu->view.busy)
      return HH_UI_ACTION_NONE;
   if (!menu->view.dialog.confirm_selected)
   {
      hh_quick_menu_back(menu);
      return HH_UI_ACTION_NONE;
   }
   action = menu->view.dialog.action;
   if (hh_quick_menu_item_state(menu, menu->view.selected_index)
         == HH_QUICK_MENU_ITEM_DISABLED)
   {
      hh_quick_menu_back(menu);
      return HH_UI_ACTION_NONE;
   }
   menu->view.dialog.visible = false;
   return hh_quick_menu_begin_action(menu, action);
}

void hh_quick_menu_back(hh_quick_menu_t *menu)
{
   if (!menu)
      return;
   if (menu->state == HH_QUICK_MENU_CONFIRM_DIALOG)
   {
      menu->view.dialog.visible = false;
      menu->state = HH_QUICK_MENU_OPEN;
      return;
   }
   if (menu->state == HH_QUICK_MENU_OPEN)
   {
      if (hh_quick_menu_is_shader_page(menu))
      {
         if (menu->view.shader_fullscreen)
            menu->view.shader_fullscreen = false;
         else if (menu->view.page == HH_QUICK_MENU_PAGE_SHADER_SCOPE)
            menu->view.page = HH_QUICK_MENU_PAGE_SHADER;
         return;
      }
      if (menu->view.page != HH_QUICK_MENU_PAGE_MAIN)
      {
         menu->view.page = HH_QUICK_MENU_PAGE_MAIN;
         hh_quick_menu_clear_feedback(menu);
         return;
      }
      hh_quick_menu_close(menu);
   }
}
