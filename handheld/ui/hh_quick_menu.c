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

bool hh_quick_menu_is_shader_page(const hh_quick_menu_t *menu)
{
   return menu && (menu->view.page == HH_QUICK_MENU_PAGE_SHADER
         || menu->view.page == HH_QUICK_MENU_PAGE_SHADER_SCOPE);
}

const char *hh_quick_menu_shader_scope_label(size_t scope)
{
   static const char *labels[] = {"仅本次使用", "应用到当前游戏",
      "应用到当前模拟器", "设为全局默认", "游戏：恢复继承",
      "模拟器：恢复继承", "全局：恢复默认"};
   return scope < HH_QUICK_MENU_SHADER_SCOPES ? labels[scope] : "";
}

const char *hh_quick_menu_shader_scope_detail(size_t scope)
{
   static const char *details[] = {"当前运行有效，不保存",
      "此游戏在当前核心下使用", "此核心的默认效果",
      "已有游戏、文件夹、核心设置优先",
      "移除游戏预设，使用上一级设置",
      "移除核心预设，保留游戏与文件夹设置",
      "移除全局预设，保留更具体的设置"};
   return scope < HH_QUICK_MENU_SHADER_SCOPES ? details[scope] : "";
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
   menu->touch_active = false;
   menu->touch_dragged = false;
   menu->touch_scrolled = false;
   menu->list_scroll_fraction = 0;
   menu->buttons_previous = 0;
   menu->buttons_repeating = 0;
   menu->state = HH_QUICK_MENU_OPENING;
   menu->open_generation++;
   menu->view.page = HH_QUICK_MENU_PAGE_MAIN;
   menu->view.selected_index = 0;
   menu->main_scroll = hh_quick_menu_main_scroll(menu->view.selected_index);
   menu->main_scroll_origin = menu->main_scroll_target = menu->main_scroll;
   menu->main_scroll_elapsed = 0;
   menu->animation_time_ms = 0;
   menu->view.busy = false;
   menu->view.pending_action = HH_UI_ACTION_NONE;
   menu->view.pending_slot = -1;
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
         || (menu->state == HH_QUICK_MENU_ACTION_PENDING
         && menu->view.pending_action != HH_UI_ACTION_ADVANCED_MENU))
      return;
   menu->touch_active = false;
   menu->touch_dragged = false;
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

unsigned hh_quick_menu_poll_buttons(hh_quick_menu_t *menu,
      unsigned buttons, unsigned long time_ms)
{
   unsigned triggers, i;
   if (!menu)
      return 0;
   if (menu->state == HH_QUICK_MENU_CLOSED
         || menu->state == HH_QUICK_MENU_CLOSING)
      buttons = 0;
   triggers = buttons & ~menu->buttons_previous;
   for (i = 0; i < 4; i++)
   {
      unsigned bit = 1U << i;
      if (!(buttons & bit) || (triggers & bit) || menu->view.busy
            || menu->state == HH_QUICK_MENU_ACTION_PENDING)
      {
         menu->buttons_repeating &= ~bit;
         menu->button_time_ms[i] = time_ms;
      }
      else if (time_ms - menu->button_time_ms[i]
            >= ((menu->buttons_repeating & bit) ? 120UL : 350UL))
      {
         triggers |= bit;
         menu->buttons_repeating |= bit;
         menu->button_time_ms[i] = time_ms;
      }
   }
   menu->buttons_previous = buttons;
   return triggers;
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

void hh_quick_menu_update_animation(hh_quick_menu_t *menu,
      unsigned long time_ms)
{
   unsigned long delta;
   float target, progress;
   float *scroll, *origin, *scroll_target, *elapsed;
   if (!menu)
      return;
   delta = menu->animation_time_ms ? time_ms - menu->animation_time_ms : 0;
   menu->animation_time_ms = time_ms;
   if (menu->touch_active
         || (menu->touch_scrolled && menu->touch_scroll_page == menu->view.page))
      return;
   scroll = &menu->slot_scroll;
   origin = &menu->slot_scroll_origin;
   scroll_target = &menu->slot_scroll_target;
   elapsed = &menu->slot_scroll_elapsed;
   if (menu->view.page == HH_QUICK_MENU_PAGE_MAIN)
   {
      target = hh_quick_menu_main_scroll(menu->view.selected_index);
      scroll = &menu->main_scroll;
      origin = &menu->main_scroll_origin;
      scroll_target = &menu->main_scroll_target;
      elapsed = &menu->main_scroll_elapsed;
   }
   else if (menu->view.page == HH_QUICK_MENU_PAGE_RECENT)
      target = hh_quick_menu_recent_scroll(menu->view.recent_selected,
            menu->view.recent_count);
   else if (menu->view.page == HH_QUICK_MENU_PAGE_SAVE
         || menu->view.page == HH_QUICK_MENU_PAGE_LOAD)
      target = hh_quick_menu_slot_scroll(menu->view.state_slot);
   else
      return;
   if (target != *scroll_target)
   {
      *origin = *scroll;
      *scroll_target = target;
      *elapsed = 0;
      delta = 0;
   }
   if (*scroll == target)
      return;
   *elapsed += (float)delta;
   if (*elapsed >= 220.0f)
   {
      *scroll = target;
      return;
   }
   progress = *elapsed / 220.0f;
   progress = 1.0f - (1.0f - progress) * (1.0f - progress) * (1.0f - progress);
   *scroll = *origin
      + (target - *origin) * progress;
}
