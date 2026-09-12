#include "hh_quick_menu_internal.h"

#include <string.h>

void hh_quick_menu_init(hh_quick_menu_t *menu)
{
   if (!menu)
      return;
   memset(menu, 0, sizeof(*menu));
   menu->state = HH_QUICK_MENU_CLOSED;
   hh_quick_menu_layout_init(menu);
}

void hh_quick_menu_open(hh_quick_menu_t *menu)
{
   if (!menu || menu->state != HH_QUICK_MENU_CLOSED)
      return;
   menu->state = HH_QUICK_MENU_OPENING;
}

void hh_quick_menu_finish_open(hh_quick_menu_t *menu)
{
   if (!menu || menu->state != HH_QUICK_MENU_OPENING)
      return;
   menu->state = HH_QUICK_MENU_OPEN;
}

void hh_quick_menu_close(hh_quick_menu_t *menu)
{
   if (!menu || menu->state == HH_QUICK_MENU_CLOSED
         || menu->state == HH_QUICK_MENU_CLOSING)
      return;
   menu->state = HH_QUICK_MENU_CLOSING;
   menu->view.dialog.visible = false;
}

void hh_quick_menu_finish_close(hh_quick_menu_t *menu)
{
   if (!menu || menu->state != HH_QUICK_MENU_CLOSING)
      return;
   menu->state = HH_QUICK_MENU_CLOSED;
}

void hh_quick_menu_set_busy(hh_quick_menu_t *menu, bool busy)
{
   if (menu)
      menu->view.busy = busy;
}
