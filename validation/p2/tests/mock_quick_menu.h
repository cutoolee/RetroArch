#ifndef MOCK_QUICK_MENU_H
#define MOCK_QUICK_MENU_H

#include "hh_quick_menu_render.h"
#include <string.h>

static void mock_quick_menu(hh_quick_menu_t *menu)
{
   hh_quick_menu_game_t game;
   hh_quick_menu_slot_t slot;
   hh_quick_menu_init(menu);
   memset(&game, 0, sizeof(game));
   strcpy(game.title, "Pokémon Emerald");
   strcpy(game.platform, "Game Boy Advance");
   strcpy(game.subtitle, "离线 UI 演示");
   hh_quick_menu_set_game(menu, &game);
   memset(&slot, 0, sizeof(slot));
   slot.index = 0;
   slot.occupied = true;
   slot.preview_available = true;
   strcpy(slot.timestamp, "2026-09-12 14:30");
   strcpy(slot.label, "绿荫镇");
   hh_quick_menu_set_slot(menu, &slot);
   slot.index = 2;
   slot.preview_available = false;
   strcpy(slot.timestamp, "昨天");
   strcpy(slot.label, "道馆之前");
   hh_quick_menu_set_slot(menu, &slot);
   menu->view.capabilities.advanced_menu_enabled = false;
   hh_quick_menu_open(menu);
   hh_quick_menu_finish_open(menu);
}

#endif
