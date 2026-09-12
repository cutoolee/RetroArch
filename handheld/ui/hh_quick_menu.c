#include "hh_quick_menu_internal.h"

#include <string.h>

void hh_quick_menu_copy_text(char *target, size_t capacity, const char *source)
{
   size_t length = 0;
   if (!capacity)
      return;
   while (length < capacity - 1 && source[length])
      length++;
   /* Do not split a UTF-8 character at the buffer boundary. */
   if (length == capacity - 1)
      while (length && ((unsigned char)source[length] & 0xc0) == 0x80)
         length--;
   memmove(target, source, length);
   target[length] = '\0';
}

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
   menu->view.page = HH_QUICK_MENU_PAGE_MAIN;
   hh_quick_menu_clear_feedback(menu);
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
         || menu->state == HH_QUICK_MENU_CLOSING
         || menu->state == HH_QUICK_MENU_ACTION_PENDING)
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

void hh_quick_menu_set_game(hh_quick_menu_t *menu,
      const hh_quick_menu_game_t *game)
{
   if (!menu || !game)
      return;
   menu->view.game = *game;
   hh_quick_menu_copy_text(menu->view.game.title,
         HH_QUICK_MENU_TEXT_MAX, game->title);
   hh_quick_menu_copy_text(menu->view.game.platform,
         HH_QUICK_MENU_TEXT_MAX, game->platform);
   hh_quick_menu_copy_text(menu->view.game.subtitle,
         HH_QUICK_MENU_TEXT_MAX, game->subtitle);
}

void hh_quick_menu_set_slot(hh_quick_menu_t *menu,
      const hh_quick_menu_slot_t *slot)
{
   hh_quick_menu_slot_t *target;
   if (!menu || !slot || slot->index < 0
         || slot->index >= HH_QUICK_MENU_SLOT_COUNT)
      return;
   target = &menu->view.slots[slot->index];
   *target = *slot;
   hh_quick_menu_copy_text(target->timestamp,
         HH_QUICK_MENU_MESSAGE_MAX, slot->timestamp);
   hh_quick_menu_copy_text(target->label, HH_QUICK_MENU_TEXT_MAX, slot->label);
}

void hh_quick_menu_set_capabilities(hh_quick_menu_t *menu,
      const hh_quick_menu_capabilities_t *capabilities)
{
   if (menu && capabilities)
      menu->view.capabilities = *capabilities;
}
