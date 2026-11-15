#ifndef HH_QUICK_MENU_INTERNAL_H
#define HH_QUICK_MENU_INTERNAL_H

#include "hh_quick_menu.h"
#include "hh_quick_menu_render.h"

void hh_quick_menu_layout_init(hh_quick_menu_t *menu);
hh_ui_rect_t hh_quick_menu_rect(float x, float y, float width, float height);
void hh_quick_menu_copy_text(char *target, size_t capacity, const char *source);

#endif
