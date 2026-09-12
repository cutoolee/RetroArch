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
   if (menu->view.state_slot < 9)
      menu->view.state_slot++;
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
