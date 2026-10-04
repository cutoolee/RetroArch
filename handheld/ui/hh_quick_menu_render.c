#include "hh_quick_menu_internal.h"

#include <string.h>
#include <stdio.h>

const char *hh_ui_action_to_string(hh_ui_action_t action)
{
   switch (action)
   {
      case HH_UI_ACTION_CONTINUE:       return "CONTINUE";
      case HH_UI_ACTION_SAVE:           return "SAVE";
      case HH_UI_ACTION_LOAD:           return "LOAD";
      case HH_UI_ACTION_RESET:          return "RESET";
      case HH_UI_ACTION_ADVANCED_MENU:  return "ADVANCED_MENU";
      case HH_UI_ACTION_EXIT:           return "EXIT";
      case HH_UI_ACTION_SHADER_BEGIN: return "SHADER_BEGIN";
      case HH_UI_ACTION_SHADER_APPLY: return "SHADER_APPLY";
      case HH_UI_ACTION_SHADER_CANCEL: return "SHADER_CANCEL";
      case HH_UI_ACTION_CONTROLS_BEGIN: return "CONTROLS_BEGIN";
      case HH_UI_ACTION_CONTROLS_SET: return "CONTROLS_SET";
      case HH_UI_ACTION_CONTROLS_SAVE: return "CONTROLS_SAVE";
      case HH_UI_ACTION_CONTROLS_DEVICE: return "CONTROLS_DEVICE";
      case HH_UI_ACTION_RECENT:         return "RECENT";
      default:                          return "NONE";
   }
}

const char *hh_quick_menu_state_to_string(hh_quick_menu_state_t state)
{
   switch (state)
   {
      case HH_QUICK_MENU_CLOSED:           return "CLOSED";
      case HH_QUICK_MENU_OPENING:          return "OPENING";
      case HH_QUICK_MENU_OPEN:             return "OPEN";
      case HH_QUICK_MENU_CONFIRM_DIALOG:   return "CONFIRM_DIALOG";
      case HH_QUICK_MENU_ACTION_PENDING:   return "ACTION_PENDING";
      case HH_QUICK_MENU_CLOSING:          return "CLOSING";
      default:                             return "UNKNOWN";
   }
}

static const hh_quick_menu_theme_t hh_quick_menu_theme =
{
   0x080b11ccUL, 0x141c27ffUL, 0x214d42ccUL,
   0xeff3f8ffUL, 0xa5b6c8ffUL, 0x6f8093ffUL,
   0x76e2b2ffUL, 0xff938affUL,
   8.0f, 24.0f, 36.0f, 24.0f, 28.0f, 36.0f, 44.0f,
   0x00000055UL, 0x1c2d3eb8UL, 0x76e2b2ffUL, 0xffd27dffUL
};

const hh_quick_menu_theme_t *hh_quick_menu_default_theme(void)
{
   return &hh_quick_menu_theme;
}

hh_ui_rect_t hh_quick_menu_rect(float x, float y, float width, float height)
{
   hh_ui_rect_t rect;
   rect.x = x;
   rect.y = y;
   rect.width = width;
   rect.height = height;
   return rect;
}

static hh_ui_rect_t hh_quick_menu_line(hh_ui_rect_t rect,
      float offset, float height, float scale)
{
   rect.y += offset * scale;
   rect.height = height * scale;
   return rect;
}

static hh_ui_rect_t hh_quick_menu_expand(hh_ui_rect_t rect,
      float amount)
{
   rect.x -= amount;
   rect.y -= amount;
   rect.width += amount * 2.0f;
   rect.height += amount * 2.0f;
   return rect;
}

static void hh_quick_menu_paint_text(const hh_quick_menu_painter_t *p,
      hh_ui_rect_t rect, const char *text, unsigned long color,
      float size, bool bold)
{
   p->text(p->userdata, rect, text, color, size, bold);
}

static const char *hh_quick_menu_display_title(const char *title)
{
   const char *slash;
   if (!title || !*title)
      return title;
   slash = strrchr(title, '/');
   return slash && slash[1] ? slash + 1 : title;
}

static unsigned long hh_quick_menu_mix(unsigned long a, unsigned long b,
      float amount)
{
   unsigned long color = 0;
   int shift;
   for (shift = 0; shift <= 24; shift += 8)
   {
      float first = (float)((a >> shift) & 255);
      float last = (float)((b >> shift) & 255);
      color |= (unsigned long)(first + (last - first) * amount) << shift;
   }
   return color;
}

static void hh_quick_menu_card_band(const hh_quick_menu_painter_t *p,
      hh_ui_rect_t r, float skew, float y1, float y2, float inset1,
      float inset2, unsigned long top, unsigned long bottom)
{
   float points[8];
   float left1 = r.x + skew * (1.0f - y1 / r.height);
   float left2 = r.x + skew * (1.0f - y2 / r.height);
   points[0] = left1 + inset1;
   points[1] = points[3] = r.y + y1;
   points[2] = left1 + r.width - skew - inset1;
   points[4] = left2 + inset2;
   points[5] = points[7] = r.y + y2;
   points[6] = left2 + r.width - skew - inset2;
   if (p->quad)
      p->quad(p->userdata, points,
            hh_quick_menu_mix(top, bottom, y1 / r.height),
            hh_quick_menu_mix(top, bottom, y2 / r.height));
   else
      p->rect(p->userdata, hh_quick_menu_rect(points[0], points[1],
               points[2] - points[0], y2 - y1),
            hh_quick_menu_mix(top, bottom, y1 / r.height), 0, 0, 0);
}

static void hh_quick_menu_card_fill(const hh_quick_menu_painter_t *p,
      hh_ui_rect_t r, float skew, float radius,
      unsigned long top, unsigned long bottom)
{
   static const float arc[] = {
      0.0f, 0.019215f, 0.076120f, 0.168530f, 0.292893f,
      0.444430f, 0.617317f, 0.804910f, 1.0f
   };
   int i;
   for (i = 0; i < 8; i++)
   {
      hh_quick_menu_card_band(p, r, skew, radius * arc[i],
            radius * arc[i + 1], radius * arc[8 - i],
            radius * arc[7 - i], top, bottom);
      hh_quick_menu_card_band(p, r, skew,
            r.height - radius * arc[i + 1], r.height - radius * arc[i],
            radius * arc[7 - i], radius * arc[8 - i], top, bottom);
   }
   hh_quick_menu_card_band(p, r, skew, radius, r.height - radius,
         0, 0, top, bottom);
}

static void hh_quick_menu_card_edge(hh_ui_rect_t r, float skew,
      float radius, int point, float *x, float *y)
{
   static const float arc[] = {
      0.0f, 0.019215f, 0.076120f, 0.168530f, 0.292893f,
      0.444430f, 0.617317f, 0.804910f, 1.0f
   };
   int corner = (point % 36) / 9;
   int step = point % 9;
   float a = radius * arc[step];
   float b = radius * arc[8 - step];
   *x = corner == 0 ? a : corner == 1 ? r.width - skew - b :
      corner == 2 ? r.width - skew - a : b;
   *y = corner == 0 ? b : corner == 1 ? a :
      corner == 2 ? r.height - b : r.height - a;
   *x += r.x + skew * (1 - *y / r.height);
   *y += r.y;
}

static void hh_quick_menu_paint_card(const hh_quick_menu_painter_t *p,
      hh_ui_rect_t r, float skew, float radius, float stroke,
      unsigned long fill, unsigned long border)
{
   hh_ui_rect_t inner = hh_quick_menu_expand(r, -stroke);
   float points[8];
   int i;
   if (!p->quad)
   {
      p->rect(p->userdata, r, fill, border, radius, stroke);
      return;
   }
   hh_quick_menu_card_fill(p, inner, skew, radius - stroke, fill, fill);
   for (i = 0; i < 36; i++)
   {
      hh_quick_menu_card_edge(r, skew, radius, i, &points[0], &points[1]);
      hh_quick_menu_card_edge(r, skew, radius, i + 1, &points[2], &points[3]);
      hh_quick_menu_card_edge(inner, skew, radius - stroke,
            i, &points[4], &points[5]);
      hh_quick_menu_card_edge(inner, skew, radius - stroke,
            i + 1, &points[6], &points[7]);
      p->quad(p->userdata, points, border, border);
   }
}

static void hh_quick_menu_paint_tile(const hh_quick_menu_painter_t *p,
      const hh_quick_menu_theme_t *t, hh_ui_rect_t rect, float s,
      const char *label, hh_quick_menu_item_state_t state, bool focused,
      hh_quick_menu_icon_t icon)
{
   hh_ui_rect_t text;
   bool disabled = state == HH_QUICK_MENU_ITEM_DISABLED;
   bool pending = state == HH_QUICK_MENU_ITEM_PENDING;
   unsigned long color = disabled ? t->text_disabled : t->text_primary;
   unsigned long border = focused ? t->focus : 0x344253ffUL;
   unsigned long fill = focused ? t->surface_focused : t->surface_muted;
   float stroke = (focused ? 2 : 1) * s;
   hh_quick_menu_paint_card(p, rect, 24 * s, 16 * s, stroke, fill, border);
   text = hh_quick_menu_rect(rect.x + (rect.width - 64 * s) / 2,
         rect.y + 48 * s, 64 * s, 64 * s);
   if (p->icon)
      p->icon(p->userdata, text, icon, disabled ? t->text_disabled :
            focused ? t->focus : t->text_secondary);
   text = hh_quick_menu_rect(rect.x + 24 * s,
         rect.y + 132 * s, rect.width - 48 * s, 48 * s);
   if (p->text_centered)
      p->text_centered(p->userdata, text, label, color, 30 * s, true);
   else
      hh_quick_menu_paint_text(p, text, label, color, 30 * s, true);
   if (disabled || pending)
   {
      text.x = rect.x + 24 * s;
      text.y = rect.y + 181 * s;
      text.width = rect.width - 48 * s;
      text.height = 28 * s;
      if (p->text_centered)
         p->text_centered(p->userdata, text, pending ? "处理中…" : "不可用",
               color, 22 * s, false);
      else
         hh_quick_menu_paint_text(p, text, pending ? "处理中…" : "不可用",
               color, 22 * s, false);
   }
}

static void hh_quick_menu_paint_background(const hh_quick_menu_layout_t *l,
      const hh_quick_menu_painter_t *p)
{
   hh_ui_rect_t r;
   p->rect(p->userdata, l->viewport, 0x020d1988UL, 0, 0, 0);
   if (p->quad)
   {
      r = l->viewport;
      r.height *= 0.45f;
      hh_quick_menu_card_band(p, r, 0, 0, r.height, 0, 0,
            0x00071360UL, 0x00071300UL);
      r = l->viewport;
      r.y = r.height * 0.60f;
      r.height *= 0.40f;
      hh_quick_menu_card_band(p, r, 0, 0, r.height, 0, 0,
            0x00071300UL, 0x000713e0UL);
   }
}

static void hh_quick_menu_paint_footer(const hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *l, const hh_quick_menu_theme_t *t,
      const hh_quick_menu_painter_t *p)
{
   hh_ui_rect_t r;
   float s = l->scale;
   size_t i;
   bool disabled = menu->view.busy || menu->state == HH_QUICK_MENU_ACTION_PENDING
      || ((menu->view.page == HH_QUICK_MENU_PAGE_SAVE
            || menu->view.page == HH_QUICK_MENU_PAGE_LOAD)
         && hh_quick_menu_slot_disabled(menu, menu->view.state_slot));
   if (menu->view.page == HH_QUICK_MENU_PAGE_RECENT)
   {
      size_t row = menu->view.recent_selected - menu->view.recent_first;
      disabled = disabled || !menu->view.recent_count
         || row >= HH_QUICK_MENU_RECENT_ROWS
         || menu->view.recent[row].disabled;
   }
   if (hh_quick_menu_is_shader_page(menu))
      disabled = disabled || !menu->view.shader_valid
         || (menu->view.page == HH_QUICK_MENU_PAGE_SHADER
            && (menu->view.shader_selected >= menu->view.shader_count
               || menu->view.shaders[menu->view.shader_selected].id != menu->view.shader_active_id))
         || (menu->view.page == HH_QUICK_MENU_PAGE_SHADER_SCOPE
            && menu->view.shader_scope >= 4
            && !menu->view.shader_removable[menu->view.shader_scope - 4]);
   for (i = 0; i < 3; i++)
   {
      r = l->footer;
      r.x += (i == 2 ? 380 : i * 188) * s;
      r.y -= 8 * s;
      r.width = (i == 2 ? 188 : 144) * s;
      r.height += 16 * s;
      r.x -= 10 * s;
      r.width += 20 * s;
      p->rect(p->userdata, r,
            menu->touch_active && !menu->touch_dragged
            && menu->touch_start_x >= r.x && menu->touch_start_x < r.x + r.width
            && menu->touch_start_y >= r.y && menu->touch_start_y < r.y + r.height
            ? t->surface_focused : 0x111d2a99UL, 0, r.height * 0.5f, 0);
      r.x += 10 * s;
      r.y += 8 * s;
      r.width = r.height = 48 * s;
      r.y += 2 * s;
      if (i == 2)
      {
         r.width = r.height = 80 * s;
         r.y -= 16 * s;
      }
      if (p->icon)
         p->icon(p->userdata, r,
               i == 0 ? HH_QUICK_MENU_ICON_CONFIRM :
               i == 1 ? HH_QUICK_MENU_ICON_BACK : HH_QUICK_MENU_ICON_SELECT,
               i == 0 ? (disabled ? t->text_disabled : t->focus) :
               i == 1 ? 0xff4555ffUL : t->text_secondary);
      r = l->footer;
      r.x += (i == 2 ? 492 : 72 + i * 188) * s;
      r.width = 64 * s;
      hh_quick_menu_paint_text(p, r, i == 0 ? "确认" : i == 1 ? "返回" : "选择",
            0xaebbd0ffUL, 28 * s, false);
   }
}

static void hh_quick_menu_paint_pause(const hh_quick_menu_layout_t *l,
      const hh_quick_menu_painter_t *p)
{
   hh_ui_rect_t r;
   float s = l->scale;
   int i;
   for (i = 0; i < 2; i++)
   {
      r = hh_quick_menu_rect(l->viewport.width - (148 - i * 48) * s,
            72 * s, 18 * s, 90 * s);
      p->rect(p->userdata, r, 0xaab3c28cUL, 0, 9 * s, 0);
   }
}

static void hh_quick_menu_paint_main(const hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *l, const hh_quick_menu_theme_t *t,
      const hh_quick_menu_painter_t *p)
{
   float s = l->scale;
   size_t i;
   static const hh_quick_menu_icon_t icons[] = {
      HH_QUICK_MENU_ICON_CONTINUE, HH_QUICK_MENU_ICON_SAVE,
      HH_QUICK_MENU_ICON_LOAD, HH_QUICK_MENU_ICON_ADVANCED,
      HH_QUICK_MENU_ICON_GAMEPAD, HH_QUICK_MENU_ICON_RESET,
      HH_QUICK_MENU_ICON_ADVANCED, HH_QUICK_MENU_ICON_GAMEPAD,
      HH_QUICK_MENU_ICON_EXIT
   };
   hh_ui_rect_t rect;
   hh_quick_menu_paint_background(l, p);
   hh_quick_menu_paint_pause(l, p);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(l->main_header, 0, 64, s),
         hh_quick_menu_display_title(menu->view.game.title),
         0xf5f6fcffUL, 48 * s, true);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(l->main_header, 70, 44, s),
         menu->view.game.platform, 0x8998aeffUL, 36 * s, false);
   for (i = 0; i < menu->view.item_count && i < HH_QUICK_MENU_ITEM_COUNT; i++)
      if (hh_quick_menu_main_item_bounds(l, menu->main_scroll, i, &rect))
         hh_quick_menu_paint_tile(p, t, rect, s, menu->view.items[i].label,
               hh_quick_menu_item_state(menu, i), i == menu->view.selected_index,
               icons[menu->view.items[i].id]);
   if (menu->view.busy || menu->state == HH_QUICK_MENU_ACTION_PENDING
         || menu->view.feedback != HH_QUICK_MENU_FEEDBACK_NONE)
      hh_quick_menu_paint_text(p, l->main_feedback,
            menu->view.busy || menu->state == HH_QUICK_MENU_ACTION_PENDING
            ? "处理中，请稍候…" : menu->view.message,
            menu->view.feedback == HH_QUICK_MENU_ACTION_ERROR ? t->danger : t->pending,
            28 * s, false);
   hh_quick_menu_paint_footer(menu, l, t, p);
}

static void hh_quick_menu_paint_controls(const hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *l, const hh_quick_menu_theme_t *t,
      const hh_quick_menu_painter_t *p)
{
   unsigned row, index;
   char label[512];
   hh_ui_rect_t rect, header;
   float s = l->scale;
   bool edit = menu->view.page == HH_QUICK_MENU_PAGE_CONTROL_EDIT;
   header = l->header;
   header.y -= 132 * s;
   hh_quick_menu_paint_background(l, p);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(header, 0, 64, s),
         edit ? "编辑按键" : "按键配置", t->text_primary, 48 * s, true);
   snprintf(label, sizeof(label), "玩家 %u · %s%s", menu->view.control_player + 1,
         edit ? menu->view.controls.sources[menu->view.control_source] : menu->view.game.title,
         menu->view.controls_dirty ? " · 尚未保存" : "");
   hh_quick_menu_paint_text(p, hh_quick_menu_line(header, 70, 44, s),
         label, t->text_secondary, 30 * s, false);
   for (row = 0; row <= HH_QUICK_MENU_CONTROL_ROWS; row++)
   {
      bool focused;
      index = menu->view.control_first + row;
      if (index >= hh_quick_menu_control_count(menu))
         break;
      focused = index == menu->view.control_selected;
      if (!hh_quick_menu_list_row_bounds(menu, l, row, &rect))
         continue;
      p->rect(p->userdata, rect,
            focused ? t->surface_focused : t->surface_muted,
            focused ? t->focus : 0x344253ffUL, 10 * s, focused ? 2 * s : s);
      rect.x += 20 * s;
      rect.width -= 40 * s;
      if (rect.height < 30 * s)
         continue;
      hh_quick_menu_control_label(menu, index, label, sizeof(label));
      hh_quick_menu_paint_text(p, rect, label,
            focused ? t->focus : t->text_primary, 30 * s, focused);
   }
   rect = hh_quick_menu_line(l->header, 594, 40, s);
   hh_quick_menu_paint_text(p, rect, edit ? "连发：按住重复、松开停止；组合：同时按下"
         : "左右切换玩家或手柄；动作按玩家独立设置", t->text_secondary, 26 * s, false);
   if (menu->view.busy || menu->view.feedback != HH_QUICK_MENU_FEEDBACK_NONE)
      hh_quick_menu_paint_text(p, hh_quick_menu_line(l->header, 642, 32, s), menu->view.busy ? "处理中…" : menu->view.message,
            menu->view.feedback == HH_QUICK_MENU_ACTION_ERROR ? t->danger : t->success,
            24 * s, false);
   hh_quick_menu_paint_footer(menu, l, t, p);
}

static void hh_quick_menu_slot_number(char *label, int slot)
{
   label[0] = (char)('0' + (slot + 1) / 10);
   label[1] = (char)('0' + (slot + 1) % 10);
   label[2] = '\0';
}

static void hh_quick_menu_paint_slots(const hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *l, const hh_quick_menu_theme_t *t,
      const hh_quick_menu_painter_t *p)
{
   const hh_quick_menu_slot_t *slot;
   hh_ui_rect_t rect, text, preview_rect;
   bool preview, focused;
   char label[32], number[3];
   int i, index = menu->view.state_slot;
   float s = l->scale;
   float k;
   for (i = 0; i < HH_QUICK_MENU_SLOT_COUNT; i++)
   {
      if (!hh_quick_menu_slot_card_bounds(l, menu->slot_scroll, i, &rect))
         continue;
      slot = &menu->view.slots[i];
      focused = i == index;
      k = rect.width / (364 * s);
      hh_quick_menu_paint_card(p, rect, 0, 12 * s, focused ? 2 * s : s,
            focused ? t->surface_focused : t->surface_muted,
            focused ? t->focus : 0x344253ffUL);
      hh_quick_menu_slot_number(number, i);
      strcpy(label, "存档 ");
      strcat(label, number);
      text = hh_quick_menu_rect(rect.x + 18 * s, rect.y + 18 * s,
            102 * k * s, 40 * s);
      hh_quick_menu_paint_text(p, text, label,
            focused ? t->focus : t->text_primary, 28 * k * s, true);
      text.x += 112 * k * s;
      text.width = rect.x + rect.width - 18 * s - text.x;
      hh_quick_menu_paint_text(p, text,
            slot->occupied ? (slot->timestamp[0] ? slot->timestamp : "已有进度") : "",
            t->text_secondary, 18 * k * s, false);
      preview_rect = hh_quick_menu_rect(rect.x + 18 * s,
            rect.y + 72 * s, rect.width - 36 * s, rect.height - 90 * s);
      p->rect(p->userdata, preview_rect, 0x0b1018ffUL, 0, 6 * s, 0);
      preview = false;
      if (slot->occupied && slot->preview_available
            && menu->view.capabilities.screenshot_enabled && p->preview)
         preview = p->preview(p->userdata, preview_rect, i);
      if (!preview)
      {
         text = hh_quick_menu_rect(
               preview_rect.x + (preview_rect.width - 44 * s) / 2,
               preview_rect.y + (preview_rect.height - 84 * s) / 2,
               44 * s, 44 * s);
         if (p->icon)
            p->icon(p->userdata, text, HH_QUICK_MENU_ICON_GAMEPAD, t->text_disabled);
         text.y += 48 * s;
         text.width = (slot->occupied ? 104 : 78) * s;
         text.x = preview_rect.x + (preview_rect.width - text.width) / 2;
         text.height = 32 * s;
         hh_quick_menu_paint_text(p, text,
               slot->occupied ? "暂无预览" : "空卡槽", t->text_secondary,
               26 * s, false);
      }
   }
   for (i = 0; i < HH_QUICK_MENU_SLOT_COUNT; i++)
   {
      float diameter;
      unsigned long color;
      rect = l->slot_dots[i];
      focused = i == index;
      diameter = (focused ? 16 : 12) * s;
      rect.x += (rect.width - diameter) / 2;
      rect.y += (rect.height - diameter) / 2;
      rect.width = rect.height = diameter;
      color = focused ? t->focus : t->text_secondary;
      p->rect(p->userdata, rect,
            focused || menu->view.slots[i].occupied ? color : t->surface,
            color, diameter / 2, focused || menu->view.slots[i].occupied ? 0 : s);
   }
}

static void hh_quick_menu_paint_recent(const hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *l, const hh_quick_menu_theme_t *t,
      const hh_quick_menu_painter_t *p)
{
   hh_ui_rect_t rect, text, preview_rect;
   const hh_quick_menu_recent_t *game;
   size_t row, index, dots, first;
   float s = l->scale;
   bool preview, focused;
   char position[64];
   hh_quick_menu_paint_text(p, hh_quick_menu_line(l->header, 0, 64, s),
         "最近游戏", t->text_primary, 48 * s, true);
   snprintf(position, sizeof(position), "%lu / %lu",
         (unsigned long)(menu->view.recent_count
            ? menu->view.recent_selected + 1 : 0),
         (unsigned long)menu->view.recent_count);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(l->header, 70, 44, s),
         menu->view.recent_count ? position : "暂无最近游戏",
         t->text_secondary, 30 * s, false);
   for (row = 0; row < HH_QUICK_MENU_RECENT_ROWS
         && menu->view.recent_first + row < menu->view.recent_count; row++)
   {
      index = menu->view.recent_first + row;
      if (!hh_quick_menu_recent_card_bounds(l, menu->slot_scroll, index, &rect))
         continue;
      game = &menu->view.recent[row];
      focused = index == menu->view.recent_selected;
      hh_quick_menu_paint_card(p, rect, 0, 12 * s, focused ? 2 * s : s,
            focused ? t->surface_focused : t->surface_muted,
            focused ? t->focus : 0x344253ffUL);
      text = hh_quick_menu_rect(rect.x + 18 * s, rect.y + 18 * s,
            rect.width - 36 * s, 40 * s);
      hh_quick_menu_paint_text(p, text, game->title,
            game->disabled ? t->text_disabled : focused ? t->focus : t->text_primary,
            28 * s, focused);
      preview_rect = hh_quick_menu_rect(rect.x + 18 * s, rect.y + 72 * s,
            rect.width - 36 * s, rect.height - 90 * s);
      p->rect(p->userdata, preview_rect, 0x0b1018ffUL, 0, 6 * s, 0);
      preview = false;
      if (game->video[0])
      {
         if (p->video_frame)
            preview = p->video_frame(p->userdata, preview_rect, game->video);
         if (focused && p->video)
         {
            if (menu->state == HH_QUICK_MENU_OPEN)
               preview = p->video(p->userdata, preview_rect, game->video) || preview;
            else
               preview = true;
         }
      }
      if (!preview && game->thumbnail[0] && p->thumbnail)
         preview = p->thumbnail(p->userdata, preview_rect, game->thumbnail);
      if (!preview)
         hh_quick_menu_paint_text(p, preview_rect, "暂无预览",
               t->text_secondary, 26 * s, false);
   }
   dots = menu->view.recent_count < HH_QUICK_MENU_SLOT_COUNT
      ? menu->view.recent_count : HH_QUICK_MENU_SLOT_COUNT;
   first = menu->view.recent_selected > dots / 2
      ? menu->view.recent_selected - dots / 2 : 0;
   if (first + dots > menu->view.recent_count)
      first = menu->view.recent_count - dots;
   for (row = 0; row < dots; row++)
   {
      float diameter;
      focused = first + row == menu->view.recent_selected;
      diameter = (focused ? 16 : 12) * s;
      rect = hh_quick_menu_rect(l->viewport.width / 2
            + ((float)row - (float)(dots - 1) / 2) * 36 * s - diameter / 2,
            l->slot_dots[0].y + 22 * s, diameter, diameter);
      p->rect(p->userdata, rect, focused ? t->focus : t->text_secondary,
            0, diameter / 2, 0);
   }
}

static void hh_quick_menu_paint_shaders(const hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *l, const hh_quick_menu_theme_t *t,
      const hh_quick_menu_painter_t *p)
{
   hh_ui_rect_t rect, text;
   const hh_quick_menu_view_t *view = &menu->view;
   const hh_quick_menu_shader_t *entry;
   size_t row, index;
   float s = l->scale;
   bool scope = view->page == HH_QUICK_MENU_PAGE_SHADER_SCOPE;
   bool focused, disabled, loading;
   char subtitle[160];
   if (view->shader_fullscreen)
   {
      rect = hh_quick_menu_rect(l->viewport.width / 2 - 156 * s,
            l->footer.y, 312 * s, 52 * s);
      p->rect(p->userdata, rect, t->surface_muted, 0, 24 * s, 0);
      rect.x += 20 * s;
      rect.width -= 40 * s;
      hh_quick_menu_paint_text(p, rect, "B / 点击：返回列表",
            t->text_secondary, 26 * s, false);
      return;
   }
   rect = l->shader_panel;
   if (!scope)
      rect.height = 536 * s;
   p->rect(p->userdata, hh_quick_menu_expand(rect, 4 * s),
         t->panel_shadow, 0, t->radius * s, 0);
   p->rect(p->userdata, rect, 0x141c27f2UL, 0x344253ffUL,
         t->radius * s, s);
   rect.x += 28 * s;
   rect.width -= 56 * s;
   hh_quick_menu_paint_text(p, hh_quick_menu_line(rect, 20, 56, s),
         scope ? "应用范围" : "着色器", t->text_primary, 42 * s, true);
   if (scope)
   {
      snprintf(subtitle, sizeof(subtitle), "当前核心：%s", view->game.platform);
      hh_quick_menu_paint_text(p, hh_quick_menu_line(rect, 82, 36, s),
            subtitle, t->text_secondary, 26 * s, false);
      if (view->shader_scope < 4 && view->shader_selected < view->shader_count)
         snprintf(subtitle, sizeof(subtitle), "当前效果：%s",
               view->shaders[view->shader_selected].name);
      else
         snprintf(subtitle, sizeof(subtitle), "进入前：%s", view->shader_source);
      hh_quick_menu_paint_text(p, hh_quick_menu_line(rect, 124, 36, s),
            subtitle, t->text_secondary, 26 * s, false);
      for (row = 0; row < HH_QUICK_MENU_SHADER_SCOPES; row++)
      {
         focused = row == view->shader_scope;
         disabled = row >= 4 && !view->shader_removable[row - 4];
         text = l->shader_scopes[row];
         hh_quick_menu_paint_card(p, text, 0, 12 * s,
               focused ? 2 * s : s, focused ? t->surface_focused : t->surface_muted,
               focused ? t->focus : 0x344253ffUL);
         text.x += 20 * s;
         text.width -= 40 * s;
         hh_quick_menu_paint_text(p, text, hh_quick_menu_shader_scope_label(row),
               disabled ? t->text_disabled : focused ? t->focus : t->text_primary,
               28 * s, focused);
      }
      hh_quick_menu_paint_text(p, hh_quick_menu_line(rect, 710, 42, s),
            view->shader_scope >= 4 && !view->shader_removable[view->shader_scope - 4]
            ? "此范围尚未单独设置" : hh_quick_menu_shader_scope_detail(view->shader_scope),
            t->text_secondary, 24 * s, false);
   }
   else
   {
      for (row = 0; row <= HH_QUICK_MENU_SHADER_ROWS
            && view->shader_first + row < view->shader_count; row++)
      {
         index = view->shader_first + row;
         entry = &view->shaders[index];
         focused = index == view->shader_selected;
         if (!hh_quick_menu_list_row_bounds(menu, l, row, &text))
            continue;
         hh_quick_menu_paint_card(p, text, 0, 12 * s,
               focused ? 2 * s : s, focused ? t->surface_focused : t->surface_muted,
               focused ? t->focus : 0x344253ffUL);
         text.x += 20 * s;
         text.width -= 40 * s;
         if (text.height < 30 * s)
            continue;
         hh_quick_menu_paint_text(p, text,
               entry->name, focused ? t->focus : t->text_primary, 30 * s, focused);
      }
      text = l->shader_filter;
      hh_quick_menu_paint_text(p, text, view->shader_recommended
            ? "推荐效果　｜　← 切换为全部内置" : "全部内置　｜　← 切换为推荐",
            t->text_secondary, 26 * s, false);
   }
   loading = menu->state == HH_QUICK_MENU_ACTION_PENDING || view->busy
      || (!scope && view->shader_selected < view->shader_count
         && view->shaders[view->shader_selected].id != view->shader_active_id);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(rect, scope ? 768 : 428, 36, s),
         view->feedback == HH_QUICK_MENU_ACTION_ERROR ? view->message :
         loading ? "正在准备预览…" : scope ? "确认后生效" : "按 A 确认应用",
         view->feedback == HH_QUICK_MENU_ACTION_ERROR ? t->danger :
         loading ? t->pending : t->focus, 24 * s, false);
   hh_quick_menu_paint_footer(menu, l, t, p);
}

static void hh_quick_menu_paint_dialog(const hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *l, const hh_quick_menu_theme_t *t,
      const hh_quick_menu_painter_t *p)
{
   hh_ui_rect_t rect;
   size_t i;
   float s = l->scale;

   if (p->flush)
      p->flush(p->userdata);
   p->rect(p->userdata, l->viewport, t->background, 0, 0, 0);
   p->rect(p->userdata, hh_quick_menu_expand(l->dialog, 4.0f * s),
         t->panel_shadow, 0, (t->radius + 3.0f) * s, 0);
   p->rect(p->userdata, l->dialog, t->surface, 0x344253ffUL,
         t->radius * s, s);
   rect = hh_quick_menu_rect(l->dialog.x + 40 * s,
         l->dialog.y + 48 * s, 48 * s, 48 * s);
   if (p->icon)
      p->icon(p->userdata, rect, menu->view.dialog.action == HH_UI_ACTION_RESET
            ? HH_QUICK_MENU_ICON_RESET : HH_QUICK_MENU_ICON_EXIT, t->danger);
   rect = l->dialog;
   rect.x += 112 * s;
   rect.width -= 152 * s;
   hh_quick_menu_paint_text(p, hh_quick_menu_line(rect, 40, 64, s),
         menu->view.dialog.message, t->text_primary, 42 * s, true);
   rect = l->dialog;
   rect.x += 40 * s;
   rect.width -= 80 * s;
   hh_quick_menu_paint_text(p, hh_quick_menu_line(rect, 140, 44, s),
         menu->view.dialog.detail, t->text_secondary, 30 * s, false);
   p->rect(p->userdata, hh_quick_menu_line(rect, 236, 1, s),
         0x344253ffUL, 0, 0, 0);
   for (i = 0; i < 2; i++)
   {
      bool focused = (i == 1) == menu->view.dialog.confirm_selected;
      rect = l->dialog_buttons[i];
      p->rect(p->userdata, rect, focused ? t->surface_focused : t->surface_muted,
            focused ? (i ? t->danger : t->focus) : 0x344253ffUL,
            12 * s, focused ? 2 * s : s);
      rect.x += 24 * s;
      rect.width -= 48 * s;
      hh_quick_menu_paint_text(p, rect, i ? menu->view.dialog.confirm_label : "取消",
            focused ? (i ? t->danger : t->focus) : t->text_primary,
            30 * s, focused);
   }
}

void hh_quick_menu_render(const hh_quick_menu_t *menu, float width,
      float height, const hh_quick_menu_theme_t *theme,
      const hh_quick_menu_painter_t *p)
{
   hh_quick_menu_layout_t l;
   const hh_quick_menu_theme_t *t = theme ? theme : &hh_quick_menu_theme;
   char subtitle[HH_QUICK_MENU_TEXT_MAX * 2 + 2];
   float s;
   bool pending;
   if (!menu || menu->state == HH_QUICK_MENU_CLOSED || !p
         || !p->rect || !p->text || !hh_quick_menu_compute_layout(width, height, &l))
      return;
   s = l.scale;
   if (hh_quick_menu_is_controls_page(menu))
   {
      hh_quick_menu_paint_controls(menu, &l, t, p);
      return;
   }
   if (hh_quick_menu_is_shader_page(menu))
   {
      hh_quick_menu_paint_shaders(menu, &l, t, p);
      return;
   }
   if (menu->view.page == HH_QUICK_MENU_PAGE_MAIN)
   {
      hh_quick_menu_paint_main(menu, &l, t, p);
      if (menu->state == HH_QUICK_MENU_CONFIRM_DIALOG && menu->view.dialog.visible)
         hh_quick_menu_paint_dialog(menu, &l, t, p);
      return;
   }
   pending = menu->state == HH_QUICK_MENU_ACTION_PENDING || menu->view.busy;
   hh_quick_menu_paint_background(&l, p);
   if (menu->state == HH_QUICK_MENU_CONFIRM_DIALOG
         && menu->view.dialog.visible)
   {
      hh_quick_menu_paint_dialog(menu, &l, t, p);
      return;
   }
   hh_quick_menu_paint_pause(&l, p);
   if (menu->view.page == HH_QUICK_MENU_PAGE_RECENT)
      hh_quick_menu_paint_recent(menu, &l, t, p);
   else
   {
      hh_quick_menu_paint_text(p, hh_quick_menu_line(l.header, 0, 64, s),
            menu->view.page == HH_QUICK_MENU_PAGE_SAVE ? "保存进度" : "读取进度",
            t->text_primary, 48 * s, true);
      hh_quick_menu_copy_text(subtitle, HH_QUICK_MENU_TEXT_MAX,
            hh_quick_menu_display_title(menu->view.game.title));
      if (subtitle[0] && menu->view.game.platform[0])
         strcat(subtitle, "  ");
      strcat(subtitle, menu->view.game.platform);
      hh_quick_menu_paint_text(p, hh_quick_menu_line(l.header, 70, 44, s),
            subtitle, 0x8998aeffUL, 36 * s, false);
      hh_quick_menu_paint_slots(menu, &l, t, p);
   }
   if (pending || menu->view.feedback != HH_QUICK_MENU_FEEDBACK_NONE)
   {
      unsigned long color = menu->view.feedback == HH_QUICK_MENU_ACTION_ERROR
         ? t->danger : pending ? t->pending : t->success;
      hh_quick_menu_paint_text(p, l.feedback,
            pending ? "处理中，请稍候…" : menu->view.message,
            color, t->font_small * s, false);
   }
   hh_quick_menu_paint_footer(menu, &l, t, p);
}
