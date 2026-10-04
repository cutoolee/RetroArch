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
   menu->view.capabilities.shader_enabled = true;
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
         HH_QUICK_MENU_ITEM_SHADER, "着色器");
   hh_quick_menu_set_item(&menu->view.items[4],
         HH_QUICK_MENU_ITEM_CONTROLS, "按键配置");
   hh_quick_menu_set_item(&menu->view.items[5],
         HH_QUICK_MENU_ITEM_RESET, "重新开始");
   hh_quick_menu_set_item(&menu->view.items[6],
         HH_QUICK_MENU_ITEM_ADVANCED_MENU, "高级菜单");
   hh_quick_menu_set_item(&menu->view.items[7],
         HH_QUICK_MENU_ITEM_RECENT, "最近游戏");
   hh_quick_menu_set_item(&menu->view.items[8],
         HH_QUICK_MENU_ITEM_EXIT, "退出游戏");
}

bool hh_quick_menu_compute_layout(float width, float height,
      hh_quick_menu_layout_t *layout)
{
   const float design_width = 1920.0f;
   const float design_height = 1080.0f;
   const float panel_width = 1120.0f;
   const float panel_height = 896.0f;
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
   content_width = panel_width - 80.0f;
   layout->scale = scale;
   layout->viewport = hh_quick_menu_rect(0, 0, width, height);
   layout->panel = hh_quick_menu_rect(x, y, panel_width * scale,
         panel_height * scale);
   layout->header = hh_quick_menu_rect(x + 40 * scale, y + 36 * scale,
         content_width * scale, 112 * scale);
   for (i = 0; i < HH_QUICK_MENU_ITEM_COUNT; i++)
      layout->items[i] = hh_quick_menu_rect(
            (width - design_width * scale) / 2 + (263 + i * 240) * scale,
            (height - design_height * scale) / 2 + 450 * scale,
            240 * scale, 216 * scale);
   layout->main_header = hh_quick_menu_rect(
         (width - design_width * scale) / 2 + 240 * scale,
         (height - design_height * scale) / 2 + 282 * scale,
         1420 * scale, 112 * scale);
   layout->main_footer = hh_quick_menu_rect(
         (width - design_width * scale) / 2 + 688 * scale,
         (height - design_height * scale) / 2 + 966 * scale,
         550 * scale, 52 * scale);
   layout->main_feedback = hh_quick_menu_rect(
         layout->main_header.x, layout->items[0].y + 248 * scale,
         layout->main_header.width, 40 * scale);
   layout->header = layout->main_header;
   for (i = 0; i < HH_QUICK_MENU_CONTROL_ROWS; i++)
      layout->control_rows[i] = hh_quick_menu_rect(
            layout->header.x, layout->header.y + (18 + i * 70) * scale,
            layout->header.width, 62 * scale);
   for (i = 0; i < HH_QUICK_MENU_SLOT_COUNT; i++)
   {
      hh_quick_menu_slot_card_bounds(layout,
            hh_quick_menu_slot_scroll(i < 5 ? 0 : HH_QUICK_MENU_SLOT_COUNT - 1),
            i, &layout->slot_cards[i]);
      layout->slot_dots[i] = hh_quick_menu_rect(
            width / 2 + (-178 + i * 36) * scale,
            (height - design_height * scale) / 2 + 752 * scale,
            32 * scale, 32 * scale);
   }
   layout->shader_panel = hh_quick_menu_rect(
         (width - design_width * scale) / 2 + 1190 * scale,
         (height - design_height * scale) / 2 + 100 * scale,
         670 * scale, 824 * scale);
   for (i = 0; i < HH_QUICK_MENU_SHADER_ROWS; i++)
      layout->shader_rows[i] = hh_quick_menu_rect(
            layout->shader_panel.x + 28 * scale,
            layout->shader_panel.y + (100 + i * 76) * scale,
            614 * scale, 64 * scale);
   for (i = 0; i < HH_QUICK_MENU_SHADER_SCOPES; i++)
      layout->shader_scopes[i] = hh_quick_menu_rect(
            layout->shader_panel.x + 28 * scale,
            layout->shader_panel.y + (180 + i * 74) * scale,
            614 * scale, 66 * scale);
   layout->shader_filter = hh_quick_menu_rect(layout->shader_panel.x + 28 * scale,
         layout->shader_panel.y + 472 * scale, 614 * scale, 36 * scale);
   layout->shader_fullscreen = hh_quick_menu_rect(layout->shader_panel.x + 28 * scale,
         layout->shader_panel.y + 736 * scale, 614 * scale, 56 * scale);
   layout->slot_card = layout->slot_cards[0];
   layout->slot_preview = hh_quick_menu_rect(
         layout->slot_card.x + 20 * scale, layout->slot_card.y + 72 * scale,
         layout->slot_card.width - 40 * scale, layout->slot_card.height - 90 * scale);
   layout->slot_info = hh_quick_menu_rect(
         layout->slot_card.x + 20 * scale, layout->slot_card.y + 18 * scale,
         layout->slot_card.width - 40 * scale, 40 * scale);
   layout->feedback = hh_quick_menu_rect(layout->header.x,
         (height - design_height * scale) / 2 + 838 * scale,
         layout->header.width, 40 * scale);
   layout->footer = layout->main_footer;
   layout->dialog = hh_quick_menu_rect((width - 840 * scale) / 2,
         (height - 400 * scale) / 2, 840 * scale, 400 * scale);
   for (i = 0; i < 2; i++)
      layout->dialog_buttons[i] = hh_quick_menu_rect(
            layout->dialog.x + (40 + i * 392) * scale,
            layout->dialog.y + 284 * scale, 368 * scale, 72 * scale);
   return true;
}

float hh_quick_menu_slot_scroll(int selected)
{
   int first = selected - 2;
   if (first < 0)
      first = 0;
   if (first > HH_QUICK_MENU_SLOT_COUNT - 5)
      first = HH_QUICK_MENU_SLOT_COUNT - 5;
   return (float)first;
}

float hh_quick_menu_recent_scroll(size_t selected, size_t count)
{
   size_t first = selected > 2 ? selected - 2 : 0;
   size_t last = count > 5 ? count - 5 : 0;
   return (float)(first > last ? last : first);
}

bool hh_quick_menu_recent_card_bounds(const hh_quick_menu_layout_t *layout,
      float scroll, size_t index, hh_ui_rect_t *bounds)
{
   float s;
   if (!layout || !bounds || scroll < 0)
      return false;
   s = layout->scale;
   *bounds = hh_quick_menu_rect(
         layout->header.x + ((float)index - scroll) * 320 * s,
         (layout->viewport.height - 1080 * s) / 2 + 450 * s,
         304 * s, 291 * s);
   return bounds->x < layout->viewport.width && bounds->x + bounds->width > 0;
}

bool hh_quick_menu_slot_card_bounds(const hh_quick_menu_layout_t *layout,
      float scroll, int slot, hh_ui_rect_t *bounds)
{
   float s;
   if (!layout || !bounds || scroll < 0 || scroll > HH_QUICK_MENU_SLOT_COUNT - 5
         || slot < 0 || slot >= HH_QUICK_MENU_SLOT_COUNT)
      return false;
   s = layout->scale;
   *bounds = hh_quick_menu_rect(
         layout->header.x + (slot - scroll) * 320 * s,
         (layout->viewport.height - 1080 * s) / 2 + 450 * s,
         304 * s, 270 * s);
   return bounds->x < layout->viewport.width && bounds->x + bounds->width > 0;
}

float hh_quick_menu_main_scroll(size_t selected)
{
   size_t first = selected > 2 ? selected - 2 : 0;
   size_t last = HH_QUICK_MENU_ITEM_COUNT > 5 ? HH_QUICK_MENU_ITEM_COUNT - 5 : 0;
   return (float)(first > last ? last : first);
}

bool hh_quick_menu_main_item_bounds(const hh_quick_menu_layout_t *layout,
      float scroll, size_t index, hh_ui_rect_t *bounds)
{
   if (!layout || !bounds || scroll < 0
         || index >= HH_QUICK_MENU_ITEM_COUNT)
      return false;
   *bounds = layout->items[index];
   bounds->x -= scroll * 240 * layout->scale;
   return bounds->x < layout->viewport.width && bounds->x + bounds->width > 0;
}

bool hh_quick_menu_list_row_bounds(const hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *layout, size_t row, hh_ui_rect_t *bounds)
{
   hh_ui_rect_t first;
   float pitch, bottom, top;
   unsigned rows;
   if (hh_quick_menu_is_controls_page(menu))
   {
      first = layout->control_rows[0];
      pitch = 70 * layout->scale;
      rows = HH_QUICK_MENU_CONTROL_ROWS;
   }
   else
   {
      first = layout->shader_rows[0];
      pitch = 76 * layout->scale;
      rows = HH_QUICK_MENU_SHADER_ROWS;
   }
   top = first.y;
   bottom = top + (rows - 1) * pitch + first.height;
   *bounds = first;
   bounds->y += ((float)row - menu->list_scroll_fraction) * pitch;
   if (bounds->y >= bottom || bounds->y + bounds->height <= top)
      return false;
   if (bounds->y < top)
   {
      bounds->height -= top - bounds->y;
      bounds->y = top;
   }
   if (bounds->y + bounds->height > bottom)
      bounds->height = bottom - bounds->y;
   return true;
}
