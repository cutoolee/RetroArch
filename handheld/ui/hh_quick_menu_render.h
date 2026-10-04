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
   unsigned long panel_shadow, surface_muted, success, pending;
} hh_quick_menu_theme_t;

typedef struct hh_quick_menu_layout
{
   float scale;
   hh_ui_rect_t viewport, panel, header;
   hh_ui_rect_t items[HH_QUICK_MENU_ITEM_COUNT];
   hh_ui_rect_t slot_cards[HH_QUICK_MENU_SLOT_COUNT];
   hh_ui_rect_t slot_card, slot_preview, slot_info;
   hh_ui_rect_t slot_dots[HH_QUICK_MENU_SLOT_COUNT];
   hh_ui_rect_t feedback, footer, dialog, dialog_buttons[2];
   hh_ui_rect_t main_header, main_footer, main_feedback;
   hh_ui_rect_t control_rows[HH_QUICK_MENU_CONTROL_ROWS];
   hh_ui_rect_t shader_panel, shader_rows[HH_QUICK_MENU_SHADER_ROWS];
   hh_ui_rect_t shader_scopes[HH_QUICK_MENU_SHADER_SCOPES];
   hh_ui_rect_t shader_filter, shader_fullscreen;
} hh_quick_menu_layout_t;

typedef enum hh_quick_menu_icon
{
   HH_QUICK_MENU_ICON_CONTINUE = 0,
   HH_QUICK_MENU_ICON_SAVE,
   HH_QUICK_MENU_ICON_LOAD,
   HH_QUICK_MENU_ICON_RESET,
   HH_QUICK_MENU_ICON_ADVANCED,
   HH_QUICK_MENU_ICON_EXIT,
   HH_QUICK_MENU_ICON_CHEVRON,
   HH_QUICK_MENU_ICON_GAMEPAD,
   HH_QUICK_MENU_ICON_CONFIRM,
   HH_QUICK_MENU_ICON_BACK,
   HH_QUICK_MENU_ICON_SELECT,
   HH_QUICK_MENU_ICON_COUNT
} hh_quick_menu_icon_t;

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
   /* Optional simple vector icon. */
   void (*icon)(void *userdata, hh_ui_rect_t bounds,
         hh_quick_menu_icon_t icon, unsigned long color);
   /* Optional caller-owned preview. False requests the placeholder. */
   bool (*preview)(void *userdata, hh_ui_rect_t bounds, int slot);
   /* Four corners: top left, top right, bottom left, bottom right. */
   void (*quad)(void *userdata, const float *vertices,
         unsigned long top, unsigned long bottom);
   /* Submit buffered drawing before a modal overlay. */
   void (*flush)(void *userdata);
   bool (*thumbnail)(void *userdata, hh_ui_rect_t bounds, const char *path);
   bool (*video)(void *userdata, hh_ui_rect_t bounds, const char *path);
   bool (*video_frame)(void *userdata, hh_ui_rect_t bounds, const char *path);
   /* Optional text callback with horizontal centering. */
   void (*text_centered)(void *userdata, hh_ui_rect_t bounds, const char *text,
         unsigned long color, float font_size, bool emphasized);
} hh_quick_menu_painter_t;

const hh_quick_menu_theme_t *hh_quick_menu_default_theme(void);
bool hh_quick_menu_compute_layout(float width, float height,
      hh_quick_menu_layout_t *layout);
bool hh_quick_menu_slot_card_bounds(const hh_quick_menu_layout_t *layout,
      float scroll, int slot, hh_ui_rect_t *bounds);
bool hh_quick_menu_main_item_bounds(const hh_quick_menu_layout_t *layout,
      float scroll, size_t index, hh_ui_rect_t *bounds);
float hh_quick_menu_main_scroll(size_t selected);
float hh_quick_menu_slot_scroll(int selected);
float hh_quick_menu_recent_scroll(size_t selected, size_t count);
bool hh_quick_menu_recent_card_bounds(const hh_quick_menu_layout_t *layout,
      float scroll, size_t index, hh_ui_rect_t *bounds);
bool hh_quick_menu_list_row_bounds(const hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *layout, size_t row, hh_ui_rect_t *bounds);
void hh_quick_menu_render(const hh_quick_menu_t *menu, float width,
      float height, const hh_quick_menu_theme_t *theme,
      const hh_quick_menu_painter_t *painter);

#ifdef __cplusplus
}
#endif

#endif
