#ifndef HH_QUICK_MENU_RENDER_H
#define HH_QUICK_MENU_RENDER_H

#include "hh_quick_menu.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hh_ui_rect
{
   float x, y, width, height;
} hh_ui_rect_t;

typedef struct hh_quick_menu_theme
{
   unsigned long background, surface, surface_focused;
   unsigned long text_primary, text_secondary, text_disabled;
   unsigned long focus, danger;
   float spacing_small, spacing_medium, spacing_large;
   float radius, font_small, font_body, font_title;
} hh_quick_menu_theme_t;

typedef struct hh_quick_menu_layout
{
   float scale;
   hh_ui_rect_t viewport, panel, header;
   hh_ui_rect_t items[HH_QUICK_MENU_ITEM_COUNT];
   hh_ui_rect_t slot_card, slot_preview, slot_info;
   hh_ui_rect_t slot_numbers[HH_QUICK_MENU_SLOT_COUNT];
   hh_ui_rect_t feedback, footer, dialog, dialog_buttons[2];
} hh_quick_menu_layout_t;

/* Colors are RRGGBBAA. Callbacks consume values synchronously. */
typedef struct hh_quick_menu_painter
{
   void *userdata;
   void (*rect)(void *userdata, hh_ui_rect_t bounds,
         unsigned long fill, unsigned long border, float radius,
         float border_width);
   /* Draw one UTF-8 line, clipped/ellipsized to bounds, vertically centered. */
   void (*text)(void *userdata, hh_ui_rect_t bounds, const char *text,
         unsigned long color, float font_size, bool emphasized);
   /* Optional caller-owned preview. False requests the placeholder. */
   bool (*preview)(void *userdata, hh_ui_rect_t bounds, int slot);
} hh_quick_menu_painter_t;

const hh_quick_menu_theme_t *hh_quick_menu_default_theme(void);
bool hh_quick_menu_compute_layout(float width, float height,
      hh_quick_menu_layout_t *layout);
void hh_quick_menu_render(const hh_quick_menu_t *menu, float width,
      float height, const hh_quick_menu_theme_t *theme,
      const hh_quick_menu_painter_t *painter);

#ifdef __cplusplus
}
#endif

#endif
