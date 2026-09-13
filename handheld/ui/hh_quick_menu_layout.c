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
   const float design_width = 1920.0f;
   const float design_height = 1080.0f;
   const float panel_width = 1180.0f;
   const float panel_height = 864.0f;
   float scale, x, y, content_width;
   int i;
   if (!layout)
      return false;
   memset(layout, 0, sizeof(*layout));
   if (!(width > 0.0f) || !(height > 0.0f)
         || width > 16384.0f || height > 16384.0f)
      return false;
   scale = width / design_width;
   if (height / design_height < scale)
      scale = height / design_height;
   x = (width - panel_width * scale) / 2.0f;
   y = (height - panel_height * scale) / 2.0f;
   content_width = panel_width - 72.0f;
   layout->scale = scale;
   layout->viewport = hh_quick_menu_rect(0, 0, width, height);
   layout->panel = hh_quick_menu_rect(x, y, panel_width * scale,
         panel_height * scale);
   layout->header = hh_quick_menu_rect(x + 36 * scale, y + 32 * scale,
         content_width * scale, 158 * scale);
   for (i = 0; i < HH_QUICK_MENU_ITEM_COUNT; i++)
      layout->items[i] = hh_quick_menu_rect(x + 36 * scale,
            y + (210 + i * 92) * scale, content_width * scale, 76 * scale);
   layout->slot_card = hh_quick_menu_rect(x + 36 * scale, y + 210 * scale,
         content_width * scale, 300 * scale);
   layout->slot_preview = hh_quick_menu_rect(x + 52 * scale, y + 226 * scale,
         180 * scale, 122 * scale);
   layout->slot_info = hh_quick_menu_rect(x + 248 * scale, y + 226 * scale,
         (content_width - 212) * scale, 122 * scale);
   for (i = 0; i < HH_QUICK_MENU_SLOT_COUNT; i++)
      layout->slot_numbers[i] = hh_quick_menu_rect(
            x + (36 + i * content_width / 10) * scale,
            y + 550 * scale, (content_width / 10 - 4) * scale, 34 * scale);
   layout->feedback = hh_quick_menu_rect(x + 36 * scale, y + 754 * scale,
         content_width * scale, 24 * scale);
   layout->footer = hh_quick_menu_rect(x + 36 * scale, y + 790 * scale,
         content_width * scale, 48 * scale);
   layout->dialog = hh_quick_menu_rect(x + 72 * scale, y + 190 * scale,
         (panel_width - 144) * scale, 420 * scale);
   for (i = 0; i < 2; i++)
      layout->dialog_buttons[i] = hh_quick_menu_rect(
            x + (104 + i * (panel_width - 208) / 2) * scale,
            y + 510 * scale, ((panel_width - 208) / 2 - 12) * scale, 64 * scale);
   return true;
}
