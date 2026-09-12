#include "hh_quick_menu.h"

#include <string.h>

static hh_ui_action_t hh_quick_menu_selected_action(
      const hh_quick_menu_t *menu)
{
   switch (menu->view.items[menu->view.selected_index].id)
   {
      case HH_QUICK_MENU_ITEM_CONTINUE:       return HH_UI_ACTION_CONTINUE;
      case HH_QUICK_MENU_ITEM_SAVE:           return HH_UI_ACTION_SAVE;
      case HH_QUICK_MENU_ITEM_LOAD:           return HH_UI_ACTION_LOAD;
      case HH_QUICK_MENU_ITEM_RESET:          return HH_UI_ACTION_RESET;
      case HH_QUICK_MENU_ITEM_ADVANCED_MENU:  return HH_UI_ACTION_ADVANCED_MENU;
      case HH_QUICK_MENU_ITEM_EXIT:           return HH_UI_ACTION_EXIT;
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
      strncpy(menu->view.dialog.message, "退出游戏？",
            HH_QUICK_MENU_MESSAGE_MAX - 1);
   menu->view.dialog.message[HH_QUICK_MENU_MESSAGE_MAX - 1] = '\0';
}

hh_ui_action_t hh_quick_menu_confirm(hh_quick_menu_t *menu)
{
   hh_ui_action_t action;

   if (!menu || menu->state != HH_QUICK_MENU_OPEN || menu->view.busy
         || menu->view.dialog.visible || !menu->view.item_count)
      return HH_UI_ACTION_NONE;
   if (menu->view.items[menu->view.selected_index].disabled)
      return HH_UI_ACTION_NONE;
   action = hh_quick_menu_selected_action(menu);
   if (hh_quick_menu_requires_confirm(action))
   {
      hh_quick_menu_show_dialog(menu, action);
      return HH_UI_ACTION_NONE;
   }
   menu->state = HH_QUICK_MENU_ACTION_PENDING;
   return action;
}

void hh_quick_menu_action_complete(hh_quick_menu_t *menu)
{
   if (menu && menu->state == HH_QUICK_MENU_ACTION_PENDING)
      menu->state = HH_QUICK_MENU_OPEN;
}

void hh_quick_menu_dialog_move_left(hh_quick_menu_t *menu)
{
   if (menu && menu->state == HH_QUICK_MENU_CONFIRM_DIALOG)
      menu->view.dialog.confirm_selected = false;
}

void hh_quick_menu_dialog_move_right(hh_quick_menu_t *menu)
{
   if (menu && menu->state == HH_QUICK_MENU_CONFIRM_DIALOG)
      menu->view.dialog.confirm_selected = true;
}

hh_ui_action_t hh_quick_menu_dialog_confirm(hh_quick_menu_t *menu)
{
   hh_ui_action_t action;

   if (!menu || menu->state != HH_QUICK_MENU_CONFIRM_DIALOG
         || !menu->view.dialog.visible)
      return HH_UI_ACTION_NONE;
   if (!menu->view.dialog.confirm_selected)
      return HH_UI_ACTION_NONE;
   action = menu->view.dialog.action;
   menu->view.dialog.visible = false;
   menu->state = HH_QUICK_MENU_ACTION_PENDING;
   return action;
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
      hh_quick_menu_close(menu);
}
