#include "hh_quick_menu.h"

static bool hh_quick_menu_is_navigable(const hh_quick_menu_t *menu)
{
   return menu && menu->state == HH_QUICK_MENU_OPEN
      && !menu->view.dialog.visible && !menu->view.busy;
}

static void hh_quick_menu_move(hh_quick_menu_t *menu, int direction)
{
   size_t index;
   size_t count;

   if (!hh_quick_menu_is_navigable(menu))
      return;
   if (menu->view.page != HH_QUICK_MENU_PAGE_MAIN)
   {
      if (direction < 0)
         hh_quick_menu_slot_prev(menu);
      else
         hh_quick_menu_slot_next(menu);
      return;
   }
   count = menu->view.item_count;
   if (!count)
      return;
   index = menu->view.selected_index;
   if (direction < 0)
      index = index ? index - 1 : count - 1;
   else
      index = (index + 1) % count;
   menu->view.selected_index = index;
}

void hh_quick_menu_move_up(hh_quick_menu_t *menu)
{
   hh_quick_menu_move(menu, -1);
}

void hh_quick_menu_move_down(hh_quick_menu_t *menu)
{
   hh_quick_menu_move(menu, 1);
}

void hh_quick_menu_slot_prev(hh_quick_menu_t *menu)
{
   if (!menu || menu->state != HH_QUICK_MENU_OPEN || menu->view.busy)
      return;
   if (menu->view.state_slot > 0)
      menu->view.state_slot--;
}

void hh_quick_menu_slot_next(hh_quick_menu_t *menu)
{
   if (!menu || menu->state != HH_QUICK_MENU_OPEN || menu->view.busy)
      return;
   if (menu->view.state_slot < HH_QUICK_MENU_SLOT_COUNT - 1)
      menu->view.state_slot++;
}

hh_quick_menu_item_state_t hh_quick_menu_item_state(
      const hh_quick_menu_t *menu, size_t index)
{
   bool enabled = true;
   const hh_quick_menu_capabilities_t *caps;
   if (!menu || index >= menu->view.item_count
         || index >= HH_QUICK_MENU_ITEM_COUNT)
      return HH_QUICK_MENU_ITEM_DISABLED;
   caps = &menu->view.capabilities;
   switch (menu->view.items[index].id)
   {
      case HH_QUICK_MENU_ITEM_SAVE: enabled = caps->save_enabled; break;
      case HH_QUICK_MENU_ITEM_LOAD: enabled = caps->load_enabled; break;
      case HH_QUICK_MENU_ITEM_RESET: enabled = caps->reset_enabled; break;
      case HH_QUICK_MENU_ITEM_ADVANCED_MENU:
         enabled = caps->advanced_menu_enabled;
         break;
      default: break;
   }
   if (index == menu->view.selected_index
         && (menu->state == HH_QUICK_MENU_ACTION_PENDING || menu->view.busy))
      return HH_QUICK_MENU_ITEM_PENDING;
   if (!enabled || menu->view.items[index].disabled)
      return HH_QUICK_MENU_ITEM_DISABLED;
   return index == menu->view.selected_index
      ? HH_QUICK_MENU_ITEM_FOCUSED : HH_QUICK_MENU_ITEM_NORMAL;
}

bool hh_quick_menu_slot_disabled(const hh_quick_menu_t *menu, int slot)
{
   if (!menu || slot < 0 || slot >= HH_QUICK_MENU_SLOT_COUNT)
      return true;
   if (menu->view.slots[slot].disabled)
      return true;
   if (menu->view.page == HH_QUICK_MENU_PAGE_LOAD)
      return !menu->view.capabilities.load_enabled
         || !menu->view.slots[slot].occupied;
   return !menu->view.capabilities.save_enabled;
}

hh_ui_action_t hh_quick_menu_input(hh_quick_menu_t *menu,
      hh_quick_menu_input_t input)
{
   if (!menu)
      return HH_UI_ACTION_NONE;
   switch (input)
   {
      case HH_QUICK_MENU_INPUT_UP: hh_quick_menu_move_up(menu); break;
      case HH_QUICK_MENU_INPUT_DOWN: hh_quick_menu_move_down(menu); break;
      case HH_QUICK_MENU_INPUT_LEFT:
         if (menu->state == HH_QUICK_MENU_CONFIRM_DIALOG)
            hh_quick_menu_dialog_move_left(menu);
         else if (menu->view.page != HH_QUICK_MENU_PAGE_MAIN)
            hh_quick_menu_slot_prev(menu);
         break;
      case HH_QUICK_MENU_INPUT_RIGHT:
         if (menu->state == HH_QUICK_MENU_CONFIRM_DIALOG)
            hh_quick_menu_dialog_move_right(menu);
         else if (menu->view.page != HH_QUICK_MENU_PAGE_MAIN)
            hh_quick_menu_slot_next(menu);
         break;
      case HH_QUICK_MENU_INPUT_CONFIRM:
         if (menu->state == HH_QUICK_MENU_CONFIRM_DIALOG)
            return hh_quick_menu_dialog_confirm(menu);
         return hh_quick_menu_confirm(menu);
      case HH_QUICK_MENU_INPUT_BACK: hh_quick_menu_back(menu); break;
      default: break;
   }
   return HH_UI_ACTION_NONE;
}

hh_quick_menu_state_t hh_quick_menu_get_state(
      const hh_quick_menu_t *menu)
{
   return menu ? menu->state : HH_QUICK_MENU_CLOSED;
}

const hh_quick_menu_view_t *hh_quick_menu_get_view(
      const hh_quick_menu_t *menu)
{
   return menu ? &menu->view : (const hh_quick_menu_view_t *)0;
}
