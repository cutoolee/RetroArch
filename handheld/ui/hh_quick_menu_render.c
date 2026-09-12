#include "hh_quick_menu_internal.h"

#include <string.h>

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
   0x071018d9UL, 0x15232effUL, 0x29473fffUL,
   0xf4f7f5ffUL, 0xb6c7cbffUL, 0x829397ffUL,
   0xa3efb4ffUL, 0xffb3a7ffUL,
   4.0f, 12.0f, 20.0f, 10.0f, 14.0f, 20.0f, 24.0f,
   0x00000066UL, 0x1d2e38ffUL, 0x9df2b4ffUL, 0xffd27dffUL
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

static void hh_quick_menu_paint_row(const hh_quick_menu_painter_t *p,
      const hh_quick_menu_theme_t *t, hh_ui_rect_t rect, float scale,
      const char *label, hh_quick_menu_item_state_t state, bool focused)
{
   hh_ui_rect_t text = rect;
   bool pending = state == HH_QUICK_MENU_ITEM_PENDING;
   bool disabled = state == HH_QUICK_MENU_ITEM_DISABLED;
   unsigned long color = disabled ? t->text_disabled : t->text_primary;
   unsigned long status_color = disabled ? t->text_disabled :
      pending ? t->pending : color;
   float inset = t->spacing_medium * scale;
   p->rect(p->userdata, rect, focused ? t->surface_focused : t->surface_muted,
         focused ? t->focus : 0, t->radius * scale, focused ? 2 * scale : 0);
   text.x += inset;
   text.width = 24 * scale;
   hh_quick_menu_paint_text(p, text, focused ? ">" : "", color,
         t->font_body * scale, true);
   text.x += text.width;
   text.width = rect.width - 2 * inset - 24 * scale - 80 * scale;
   hh_quick_menu_paint_text(p, text, label, color, t->font_body * scale, focused);
   text.x = rect.x + rect.width - inset - 80 * scale;
   text.width = 80 * scale;
   hh_quick_menu_paint_text(p, text, pending ? "处理中…" : disabled ? "不可用" : "",
         status_color, t->font_small * scale, pending || disabled);
}

static void hh_quick_menu_paint_slots(const hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *l, const hh_quick_menu_theme_t *t,
      const hh_quick_menu_painter_t *p)
{
   const hh_quick_menu_slot_t *slot;
   hh_ui_rect_t rect;
   bool disabled, preview = false;
   char label[32];
   int i, index = menu->view.state_slot;
   float s = l->scale;
   if (index < 0 || index >= HH_QUICK_MENU_SLOT_COUNT)
      return;
   slot = &menu->view.slots[index];
   disabled = hh_quick_menu_slot_disabled(menu, index);
   hh_quick_menu_paint_text(p, l->items[0],
         menu->view.page == HH_QUICK_MENU_PAGE_SAVE ? "保存进度" : "读取进度",
         t->text_primary, t->font_title * s, true);
   p->rect(p->userdata, l->slot_card, t->surface_focused, t->focus,
         t->radius * s, 2 * s);
   p->rect(p->userdata, l->slot_preview, t->background, t->text_disabled,
         t->radius * s, s);
   if (slot->occupied && slot->preview_available
         && menu->view.capabilities.screenshot_enabled && p->preview)
      preview = p->preview(p->userdata, l->slot_preview, index);
   if (!preview)
   {
      rect = l->slot_preview;
      rect.x += t->spacing_medium * s;
      rect.width -= 2 * t->spacing_medium * s;
      hh_quick_menu_paint_text(p, rect, "No Preview",
            t->text_secondary, t->font_small * s, false);
   }
   strcpy(label, "Slot 0  /  9");
   label[5] = (char)('0' + index);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(l->slot_info, 0, 30, s),
         label, t->text_primary, t->font_title * s, true);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(l->slot_info, 36, 24, s),
         slot->occupied ? (slot->timestamp[0] ? slot->timestamp : "已有进度") : "空",
         t->text_secondary, t->font_small * s, false);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(l->slot_info, 64, 24, s),
         slot->label, t->text_secondary, t->font_small * s, false);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(l->slot_info, 94, 24, s),
         menu->state == HH_QUICK_MENU_ACTION_PENDING || menu->view.busy
         ? "处理中…" : disabled ? "不可用" :
         menu->view.page == HH_QUICK_MENU_PAGE_SAVE ? "A 保存到此位置" : "A 读取此进度",
         disabled ? t->text_disabled : t->focus, t->font_small * s, true);
   for (i = 0; i < HH_QUICK_MENU_SLOT_COUNT; i++)
   {
      rect = l->slot_numbers[i];
      p->rect(p->userdata, rect, i == index ? t->surface_focused : t->surface,
            i == index ? t->focus : 0, t->radius * s, i == index ? 2 * s : 0);
      label[0] = i == index ? '>' : ' ';
      label[1] = (char)('0' + i);
      label[2] = hh_quick_menu_slot_disabled(menu, i) ? '-' : ' ';
      label[3] = '\0';
      rect.x += t->spacing_small * s;
      rect.width -= 2 * t->spacing_small * s;
      hh_quick_menu_paint_text(p, rect, label,
            hh_quick_menu_slot_disabled(menu, i) ? t->text_disabled : t->text_primary,
            t->font_small * s, i == index);
   }
}

void hh_quick_menu_render(const hh_quick_menu_t *menu, float width,
      float height, const hh_quick_menu_theme_t *theme,
      const hh_quick_menu_painter_t *p)
{
   hh_quick_menu_layout_t l;
   const hh_quick_menu_theme_t *t = theme ? theme : &hh_quick_menu_theme;
   hh_ui_rect_t rect;
   float s;
   size_t i;
   bool pending;
   if (!menu || menu->state == HH_QUICK_MENU_CLOSED || !p
         || !p->rect || !p->text || !hh_quick_menu_compute_layout(width, height, &l))
      return;
   s = l.scale;
   pending = menu->state == HH_QUICK_MENU_ACTION_PENDING || menu->view.busy;
   p->rect(p->userdata, l.viewport, t->background, 0, 0, 0);
   p->rect(p->userdata, hh_quick_menu_expand(l.panel, 6.0f * s),
         t->panel_shadow, 0, (t->radius + 4.0f) * s, 0);
   p->rect(p->userdata, l.panel, t->surface, t->surface_muted,
         t->radius * s, s);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(l.header, 0, 16, s),
         "GAMEGO  /  快捷菜单", t->focus, t->font_small * s, true);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(l.header, 18, 28, s),
         menu->view.game.title[0] ? menu->view.game.title : menu->view.title,
         t->text_primary, t->font_title * s, true);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(l.header, 48, 18, s),
         menu->view.game.platform, t->text_secondary, t->font_small * s, false);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(l.header, 68, 16, s),
         menu->view.game.subtitle, t->text_secondary, t->font_small * s, false);
   rect = l.header;
   rect.y += rect.height - s;
   rect.height = s;
   p->rect(p->userdata, rect, t->surface_muted, 0, 0, 0);
   if (menu->view.page == HH_QUICK_MENU_PAGE_MAIN)
   {
      for (i = 0; i < menu->view.item_count && i < HH_QUICK_MENU_ITEM_COUNT; i++)
         hh_quick_menu_paint_row(p, t, l.items[i], s, menu->view.items[i].label,
               hh_quick_menu_item_state(menu, i), i == menu->view.selected_index);
   }
   else
      hh_quick_menu_paint_slots(menu, &l, t, p);
   if (pending || menu->view.feedback != HH_QUICK_MENU_FEEDBACK_NONE)
   {
      bool error = menu->view.feedback == HH_QUICK_MENU_ACTION_ERROR;
      bool complete = menu->view.feedback != HH_QUICK_MENU_FEEDBACK_NONE;
      unsigned long feedback_color = error ? t->danger :
         pending ? t->pending : t->success;
      p->rect(p->userdata, l.feedback, t->surface_focused,
            feedback_color, t->radius * s, s);
      rect = l.feedback;
      rect.x += t->spacing_medium * s;
      rect.width -= 2 * t->spacing_medium * s;
      hh_quick_menu_paint_text(p, rect, pending ? "… 处理中，请稍候" :
            error ? "! 操作失败" : "✓ 操作完成", feedback_color,
            t->font_small * s, true);
      if (complete)
      {
         rect.x += 116 * s;
         rect.width -= 116 * s;
         hh_quick_menu_paint_text(p, rect, menu->view.message,
               t->text_primary, t->font_small * s, false);
      }
   }
   hh_quick_menu_paint_text(p, l.footer,
         pending ? "请等待操作结果" : menu->view.page == HH_QUICK_MENU_PAGE_MAIN
         ? "A 确认    B 返回    ↑↓ 选择" : "A 确认    B 返回    ↑↓ 切换 Slot    - 不可用",
         t->text_secondary, t->font_small * s, false);
   if (menu->state != HH_QUICK_MENU_CONFIRM_DIALOG || !menu->view.dialog.visible)
      return;
   p->rect(p->userdata, l.viewport, t->background, 0, 0, 0);
   p->rect(p->userdata, hh_quick_menu_expand(l.dialog, 4.0f * s),
         t->panel_shadow, 0, (t->radius + 3.0f) * s, 0);
   p->rect(p->userdata, l.dialog, t->surface, t->danger, t->radius * s, 2 * s);
   rect = l.dialog;
   rect.x += t->spacing_large * s;
   rect.width -= 2 * t->spacing_large * s;
   hh_quick_menu_paint_text(p, hh_quick_menu_line(rect, 20, 32, s),
         menu->view.dialog.message, t->text_primary, t->font_title * s, true);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(rect, 62, 26, s),
         menu->view.dialog.detail, t->text_secondary, t->font_body * s, false);
   hh_quick_menu_paint_text(p, hh_quick_menu_line(rect, 94, 20, s),
         "←→ 选择    A 确认    B 取消", t->text_secondary, t->font_small * s, false);
   for (i = 0; i < 2; i++)
      hh_quick_menu_paint_row(p, t, l.dialog_buttons[i], s,
            i ? menu->view.dialog.confirm_label : "取消",
            HH_QUICK_MENU_ITEM_NORMAL, (i == 1) == menu->view.dialog.confirm_selected);
}
