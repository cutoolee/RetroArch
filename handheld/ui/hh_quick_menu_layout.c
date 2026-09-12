#include "hh_quick_menu_internal.h"

#include <string.h>

static void hh_quick_menu_set_item(hh_quick_menu_item_t *item,
      hh_quick_menu_item_id_t id, const char *label)
{
   memset(item, 0, sizeof(*item));
   item->id = id;
   strncpy(item->label, label, HH_QUICK_MENU_LABEL_MAX - 1);
   item->label[HH_QUICK_MENU_LABEL_MAX - 1] = '\0';
}

void hh_quick_menu_layout_init(hh_quick_menu_t *menu)
{
   int i;
   if (!menu)
      return;
   menu->view.capabilities.save_enabled = true;
   menu->view.capabilities.load_enabled = true;
   menu->view.capabilities.screenshot_enabled = true;
   menu->view.capabilities.advanced_menu_enabled = true;
   menu->view.capabilities.reset_enabled = true;
   menu->view.pending_slot = -1;
   for (i = 0; i < HH_QUICK_MENU_SLOT_COUNT; i++)
      menu->view.slots[i].index = i;
   strncpy(menu->view.title, "快捷菜单", HH_QUICK_MENU_LABEL_MAX - 1);
   menu->view.title[HH_QUICK_MENU_LABEL_MAX - 1] = '\0';
   menu->view.item_count = HH_QUICK_MENU_ITEM_COUNT;
   hh_quick_menu_set_item(&menu->view.items[0],
         HH_QUICK_MENU_ITEM_CONTINUE, "继续游戏");
   hh_quick_menu_set_item(&menu->view.items[1],
         HH_QUICK_MENU_ITEM_SAVE, "保存进度");
   hh_quick_menu_set_item(&menu->view.items[2],
         HH_QUICK_MENU_ITEM_LOAD, "读取进度");
   hh_quick_menu_set_item(&menu->view.items[3],
         HH_QUICK_MENU_ITEM_RESET, "重新开始");
   hh_quick_menu_set_item(&menu->view.items[4],
         HH_QUICK_MENU_ITEM_ADVANCED_MENU, "高级菜单");
   hh_quick_menu_set_item(&menu->view.items[5],
         HH_QUICK_MENU_ITEM_EXIT, "退出游戏");
}

bool hh_quick_menu_compute_layout(float width, float height,
      hh_quick_menu_layout_t *layout)
{
   float scale, panel_width, x, y;
   int i;
   if (!layout)
      return false;
   memset(layout, 0, sizeof(*layout));
   if (!(width > 0.0f) || !(height > 0.0f)
         || width > 16384.0f || height > 16384.0f)
      return false;
   scale = width / 640.0f;
   if (height / 480.0f < scale)
      scale = height / 480.0f;
   panel_width = width / scale - 64.0f;
   if (panel_width > 736.0f)
      panel_width = 736.0f;
   x = (width - panel_width * scale) / 2.0f;
   y = (height - 432.0f * scale) / 2.0f;
   layout->scale = scale;
   layout->viewport = hh_quick_menu_rect(0, 0, width, height);
   layout->panel = hh_quick_menu_rect(x, y, panel_width * scale, 432 * scale);
   layout->header = hh_quick_menu_rect(x + 20 * scale, y + 12 * scale,
         (panel_width - 40) * scale, 84 * scale);
   for (i = 0; i < HH_QUICK_MENU_ITEM_COUNT; i++)
      layout->items[i] = hh_quick_menu_rect(x + 20 * scale,
            y + (108 + i * 40) * scale,
            (panel_width - 40) * scale, 36 * scale);
   layout->slot_card = hh_quick_menu_rect(x + 20 * scale, y + 148 * scale,
         (panel_width - 40) * scale, 146 * scale);
   layout->slot_preview = hh_quick_menu_rect(x + 32 * scale, y + 160 * scale,
         180 * scale, 122 * scale);
   layout->slot_info = hh_quick_menu_rect(x + 228 * scale, y + 160 * scale,
         (panel_width - 260) * scale, 122 * scale);
   for (i = 0; i < HH_QUICK_MENU_SLOT_COUNT; i++)
      layout->slot_numbers[i] = hh_quick_menu_rect(
            x + (20 + i * (panel_width - 40) / 10) * scale,
            y + 310 * scale, ((panel_width - 40) / 10 - 4) * scale, 34 * scale);
   layout->feedback = hh_quick_menu_rect(x + 20 * scale, y + 352 * scale,
         (panel_width - 40) * scale, 28 * scale);
   layout->footer = hh_quick_menu_rect(x + 20 * scale, y + 394 * scale,
         (panel_width - 40) * scale, 26 * scale);
   layout->dialog = hh_quick_menu_rect(x + 32 * scale, y + 120 * scale,
         (panel_width - 64) * scale, 192 * scale);
   for (i = 0; i < 2; i++)
      layout->dialog_buttons[i] = hh_quick_menu_rect(
            x + (52 + i * (panel_width - 104) / 2) * scale,
            y + 248 * scale, ((panel_width - 104) / 2 - 8) * scale, 44 * scale);
   return true;
}
