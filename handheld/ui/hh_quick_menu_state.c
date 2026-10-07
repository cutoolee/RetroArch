#include "hh_quick_menu_internal.h"

static bool hh_quick_menu_is_navigable(const hh_quick_menu_t *menu)
{
   return menu && menu->state == HH_QUICK_MENU_OPEN
      && !menu->view.dialog.visible && !menu->view.busy;
}

static void hh_quick_menu_move(hh_quick_menu_t *menu, int direction)
{
   size_t index;
   size_t count;

   if (!hh_quick_menu_is_navigable(menu))
      return;
   if (hh_quick_menu_is_shader_page(menu))
   {
      size_t *selected = menu->view.page == HH_QUICK_MENU_PAGE_SHADER_SCOPE
         ? &menu->view.shader_scope : &menu->view.shader_selected;
      count = menu->view.page == HH_QUICK_MENU_PAGE_SHADER_SCOPE
         ? HH_QUICK_MENU_SHADER_SCOPES : menu->view.shader_count;
      if (menu->view.shader_fullscreen)
         return;
      if (direction < 0 && *selected > 0)
         (*selected)--;
      else if (direction > 0 && *selected + 1 < count)
         (*selected)++;
      if (menu->view.shader_selected < menu->view.shader_first)
         menu->view.shader_first = menu->view.shader_selected;
      else if (menu->view.shader_selected >= menu->view.shader_first + HH_QUICK_MENU_SHADER_ROWS)
         menu->view.shader_first = menu->view.shader_selected - HH_QUICK_MENU_SHADER_ROWS + 1;
      hh_quick_menu_clear_feedback(menu);
      return;
   }
   if (menu->view.page == HH_QUICK_MENU_PAGE_RECENT)
   {
      if (direction < 0 && menu->view.recent_selected > 0)
         menu->view.recent_selected--;
      else if (direction > 0
            && menu->view.recent_selected + 1 < menu->view.recent_count)
         menu->view.recent_selected++;
      menu->view.recent_first = (size_t)hh_quick_menu_recent_scroll(
            menu->view.recent_selected, menu->view.recent_count);
      if (menu->view.recent_first)
         menu->view.recent_first--;
      hh_quick_menu_clear_feedback(menu);
      return;
   }
   if (menu->view.page != HH_QUICK_MENU_PAGE_MAIN)
   {
      if (direction < 0)
         hh_quick_menu_slot_prev(menu);
      else
         hh_quick_menu_slot_next(menu);
      return;
   }
   count = menu->view.item_count;
   if (!count)
      return;
   index = menu->view.selected_index;
   do
   {
      if (direction < 0)
         index = index ? index - 1 : count - 1;
      else
         index = (index + 1) % count;
      if (menu->view.items[index].visible)
         break;
   } while (index != menu->view.selected_index);
   if (!menu->view.items[index].visible)
      return;
   menu->view.selected_index = index;
}

void hh_quick_menu_move_up(hh_quick_menu_t *menu)
{
   hh_quick_menu_move(menu, -1);
}

void hh_quick_menu_move_down(hh_quick_menu_t *menu)
{
   hh_quick_menu_move(menu, 1);
}

void hh_quick_menu_slot_prev(hh_quick_menu_t *menu)
{
   if (!menu || menu->state != HH_QUICK_MENU_OPEN || menu->view.busy)
      return;
   if (menu->view.state_slot > 0)
      menu->view.state_slot--;
}

void hh_quick_menu_slot_next(hh_quick_menu_t *menu)
{
   if (!menu || menu->state != HH_QUICK_MENU_OPEN || menu->view.busy)
      return;
   if (menu->view.state_slot < HH_QUICK_MENU_SLOT_COUNT - 1)
      menu->view.state_slot++;
}

hh_quick_menu_item_state_t hh_quick_menu_item_state(
      const hh_quick_menu_t *menu, size_t index)
{
   bool enabled = true;
   const hh_quick_menu_capabilities_t *caps;
   if (!menu || index >= menu->view.item_count
         || index >= HH_QUICK_MENU_ITEM_COUNT)
      return HH_QUICK_MENU_ITEM_DISABLED;
   if (!menu->view.items[index].visible)
      return HH_QUICK_MENU_ITEM_DISABLED;
   caps = &menu->view.capabilities;
   switch (menu->view.items[index].id)
   {
      case HH_QUICK_MENU_ITEM_SHADER: enabled = caps->shader_enabled; break;
      case HH_QUICK_MENU_ITEM_SAVE: enabled = caps->save_enabled; break;
      case HH_QUICK_MENU_ITEM_LOAD: enabled = caps->load_enabled; break;
      case HH_QUICK_MENU_ITEM_RESET: enabled = caps->reset_enabled; break;
      case HH_QUICK_MENU_ITEM_ADVANCED_MENU:
         enabled = caps->advanced_menu_enabled;
         break;
      default: break;
   }
   if (index == menu->view.selected_index
         && (menu->state == HH_QUICK_MENU_ACTION_PENDING || menu->view.busy))
      return HH_QUICK_MENU_ITEM_PENDING;
   if (!enabled || menu->view.items[index].disabled)
      return HH_QUICK_MENU_ITEM_DISABLED;
   return index == menu->view.selected_index
      ? HH_QUICK_MENU_ITEM_FOCUSED : HH_QUICK_MENU_ITEM_NORMAL;
}

bool hh_quick_menu_slot_disabled(const hh_quick_menu_t *menu, int slot)
{
   if (!menu || slot < 0 || slot >= HH_QUICK_MENU_SLOT_COUNT)
      return true;
   if (menu->view.slots[slot].disabled)
      return true;
   if (menu->view.page == HH_QUICK_MENU_PAGE_LOAD)
      return !menu->view.capabilities.load_enabled
         || !menu->view.slots[slot].occupied;
   return !menu->view.capabilities.save_enabled;
}

hh_ui_action_t hh_quick_menu_input(hh_quick_menu_t *menu,
      hh_quick_menu_input_t input)
{
   if (!menu)
      return HH_UI_ACTION_NONE;
   menu->touch_scrolled = false;
   menu->list_scroll_fraction = 0;
   if (hh_quick_menu_is_controls_page(menu))
   {
      if (!hh_quick_menu_is_navigable(menu))
         return HH_UI_ACTION_NONE;
      return hh_quick_menu_controls_input(menu, input);
   }
   switch (input)
   {
      case HH_QUICK_MENU_INPUT_UP: hh_quick_menu_move_up(menu); break;
      case HH_QUICK_MENU_INPUT_DOWN: hh_quick_menu_move_down(menu); break;
      case HH_QUICK_MENU_INPUT_LEFT:
         if (menu->state == HH_QUICK_MENU_CONFIRM_DIALOG)
            hh_quick_menu_dialog_move_left(menu);
         else if (menu->view.page == HH_QUICK_MENU_PAGE_SHADER)
         {
            if (!menu->view.busy && menu->state == HH_QUICK_MENU_OPEN)
            {
               if (menu->view.shader_fullscreen)
                  menu->view.shader_fullscreen = false;
               else
                  menu->view.shader_recommended = !menu->view.shader_recommended;
            }
         }
         else if (menu->view.page == HH_QUICK_MENU_PAGE_SHADER_SCOPE)
            hh_quick_menu_move(menu, -1);
         else if (menu->view.page == HH_QUICK_MENU_PAGE_RECENT)
            hh_quick_menu_move(menu, -1);
         else if (menu->view.page != HH_QUICK_MENU_PAGE_MAIN)
            hh_quick_menu_slot_prev(menu);
         else
            hh_quick_menu_move(menu, -1);
         break;
      case HH_QUICK_MENU_INPUT_RIGHT:
         if (menu->state == HH_QUICK_MENU_CONFIRM_DIALOG)
            hh_quick_menu_dialog_move_right(menu);
         else if (menu->view.page == HH_QUICK_MENU_PAGE_SHADER)
         {
            if (!menu->view.busy && menu->state == HH_QUICK_MENU_OPEN)
               menu->view.shader_fullscreen = !menu->view.shader_fullscreen;
         }
         else if (menu->view.page == HH_QUICK_MENU_PAGE_SHADER_SCOPE)
            hh_quick_menu_move(menu, 1);
         else if (menu->view.page == HH_QUICK_MENU_PAGE_RECENT)
            hh_quick_menu_move(menu, 1);
         else if (menu->view.page != HH_QUICK_MENU_PAGE_MAIN)
            hh_quick_menu_slot_next(menu);
         else
            hh_quick_menu_move(menu, 1);
         break;
      case HH_QUICK_MENU_INPUT_CONFIRM:
         if (menu->state == HH_QUICK_MENU_CONFIRM_DIALOG)
            return hh_quick_menu_dialog_confirm(menu);
         return hh_quick_menu_confirm(menu);
      case HH_QUICK_MENU_INPUT_BACK:
         if (menu->state == HH_QUICK_MENU_OPEN && !menu->view.busy
               && menu->view.page == HH_QUICK_MENU_PAGE_SHADER
               && !menu->view.shader_fullscreen)
         {
            menu->state = HH_QUICK_MENU_ACTION_PENDING;
            menu->view.pending_action = HH_UI_ACTION_SHADER_CANCEL;
            return HH_UI_ACTION_SHADER_CANCEL;
         }
         hh_quick_menu_back(menu);
         break;
      default: break;
   }
   return HH_UI_ACTION_NONE;
}

static bool hh_quick_menu_rect_contains(hh_ui_rect_t rect, float x, float y)
{
   return x >= rect.x && x < rect.x + rect.width
      && y >= rect.y && y < rect.y + rect.height;
}

static bool hh_quick_menu_card_contains(hh_ui_rect_t rect, float x,
      float y, float skew, float radius)
{
   float local_y = y - rect.y;
   float local_x = x - rect.x - skew * (1 - local_y / rect.height);
   float corner_x, corner_y;
   rect.width -= skew;
   if (local_x < 0 || local_x >= rect.width
         || local_y < 0 || local_y >= rect.height)
      return false;
   corner_x = local_x < radius ? radius - local_x :
      local_x > rect.width - radius ? local_x - rect.width + radius : 0;
   corner_y = local_y < radius ? radius - local_y :
      local_y > rect.height - radius ? local_y - rect.height + radius : 0;
   return corner_x * corner_x + corner_y * corner_y <= radius * radius;
}

static bool hh_quick_menu_touch_item(const hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *layout, float x, float y, size_t *index)
{
   size_t i;
   hh_ui_rect_t rect;
   if (menu->view.page != HH_QUICK_MENU_PAGE_MAIN
         || !hh_quick_menu_rect_contains(layout->viewport, x, y))
      return false;
   for (i = 0; i < menu->view.item_count && i < HH_QUICK_MENU_ITEM_COUNT; i++)
      if (menu->view.items[i].visible
            && hh_quick_menu_main_item_bounds(layout, menu->main_scroll, i, &rect)
            && hh_quick_menu_card_contains(rect, x, y,
               24 * layout->scale, 16 * layout->scale))
      {
         *index = i;
         return true;
      }
   return false;
}

static bool hh_quick_menu_touch_slot(const hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *layout, float x, float y, int *slot)
{
   hh_ui_rect_t rect;
   int i;
   if (menu->view.page != HH_QUICK_MENU_PAGE_SAVE
         && menu->view.page != HH_QUICK_MENU_PAGE_LOAD)
      return false;
   for (i = 0; i < HH_QUICK_MENU_SLOT_COUNT; i++)
   {
      if (hh_quick_menu_card_contains(layout->slot_dots[i], x, y,
               0, 16 * layout->scale)
            || (hh_quick_menu_slot_card_bounds(layout,
                  menu->slot_scroll, i, &rect)
               && hh_quick_menu_card_contains(rect, x, y,
                  0, 12 * layout->scale)))
      {
         *slot = i;
         return true;
      }
   }
   return false;
}

static hh_quick_menu_input_t hh_quick_menu_touch_focus(hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *layout, float x, float y)
{
   size_t index, row;
   int slot;
   hh_ui_rect_t rect;
   hh_ui_rect_t footer = menu->view.page == HH_QUICK_MENU_PAGE_CONTROLS
      ? layout->control_footer : layout->footer;
   float offset;
   if (menu->state == HH_QUICK_MENU_CONFIRM_DIALOG)
   {
      for (row = 0; row < 2; row++)
         if (hh_quick_menu_rect_contains(layout->dialog_buttons[row], x, y))
         {
            menu->view.dialog.confirm_selected = row == 1;
            return HH_QUICK_MENU_INPUT_CONFIRM;
         }
      return HH_QUICK_MENU_INPUT_NONE;
   }
   if (hh_quick_menu_is_shader_page(menu) && menu->view.shader_fullscreen)
      return HH_QUICK_MENU_INPUT_RIGHT;
   if (hh_quick_menu_rect_contains(footer, x, y))
   {
      offset = (x - footer.x) / layout->scale;
      if (offset < 144)
         return HH_QUICK_MENU_INPUT_CONFIRM;
      if (offset >= 188 && offset < 332)
         return HH_QUICK_MENU_INPUT_BACK;
      return HH_QUICK_MENU_INPUT_NONE;
   }
   if (hh_quick_menu_touch_item(menu, layout, x, y, &index))
   {
      menu->view.selected_index = index;
      return HH_QUICK_MENU_INPUT_CONFIRM;
   }
   if (hh_quick_menu_touch_slot(menu, layout, x, y, &slot))
   {
      menu->view.state_slot = slot;
      return HH_QUICK_MENU_INPUT_CONFIRM;
   }
   if (hh_quick_menu_is_controls_page(menu))
   {
      if (menu->view.page == HH_QUICK_MENU_PAGE_CONTROLS)
      {
         for (row = 0; row < 20; row++)
            if (hh_quick_menu_control_bounds(layout, (unsigned)row, &rect)
                  && hh_quick_menu_rect_contains(rect, x, y))
            {
               menu->view.control_selected = (unsigned)row;
               return HH_QUICK_MENU_INPUT_CONFIRM;
            }
         return HH_QUICK_MENU_INPUT_NONE;
      }
      for (row = 0; row <= HH_QUICK_MENU_CONTROL_ROWS; row++)
         if (menu->view.control_first + row < hh_quick_menu_control_count(menu)
               && hh_quick_menu_list_row_bounds(menu, layout, row, &rect)
               && hh_quick_menu_rect_contains(rect, x, y))
         {
            menu->view.control_selected = menu->view.control_first + row;
            return HH_QUICK_MENU_INPUT_CONFIRM;
         }
   }
   else if (menu->view.page == HH_QUICK_MENU_PAGE_SHADER_SCOPE)
   {
      for (row = 0; row < HH_QUICK_MENU_SHADER_SCOPES; row++)
         if (hh_quick_menu_rect_contains(layout->shader_scopes[row], x, y))
         {
            menu->view.shader_scope = row;
            return HH_QUICK_MENU_INPUT_CONFIRM;
         }
   }
   else if (menu->view.page == HH_QUICK_MENU_PAGE_SHADER)
   {
      if (hh_quick_menu_rect_contains(layout->shader_filter, x, y))
         return HH_QUICK_MENU_INPUT_LEFT;
      for (row = 0; row <= HH_QUICK_MENU_SHADER_ROWS; row++)
         if (menu->view.shader_first + row < menu->view.shader_count
               && hh_quick_menu_list_row_bounds(menu, layout, row, &rect)
               && hh_quick_menu_rect_contains(rect, x, y))
         {
            menu->view.shader_selected = menu->view.shader_first + row;
            return HH_QUICK_MENU_INPUT_CONFIRM;
         }
   }
   else if (menu->view.page == HH_QUICK_MENU_PAGE_RECENT)
   {
      for (row = 0; row < HH_QUICK_MENU_RECENT_ROWS
            && menu->view.recent_first + row < menu->view.recent_count; row++)
         if (hh_quick_menu_recent_card_bounds(layout, menu->slot_scroll,
                  menu->view.recent_first + row, &rect)
               && hh_quick_menu_rect_contains(rect, x, y))
         {
            menu->view.recent_selected = menu->view.recent_first + row;
            return HH_QUICK_MENU_INPUT_CONFIRM;
         }
   }
   return HH_QUICK_MENU_INPUT_NONE;
}

static float hh_quick_menu_clamp_scroll(float scroll, float maximum)
{
   return scroll < 0 ? 0 : scroll > maximum ? maximum : scroll;
}

static void hh_quick_menu_touch_scroll(hh_quick_menu_t *menu,
      const hh_quick_menu_layout_t *layout, float dx, float dy)
{
   float scroll, maximum;
   size_t count, rows;
   if (menu->state == HH_QUICK_MENU_CONFIRM_DIALOG
         || menu->view.shader_fullscreen
         || menu->view.page == HH_QUICK_MENU_PAGE_CONTROLS)
      return;
   if (hh_quick_menu_is_controls_page(menu)
         || menu->view.page == HH_QUICK_MENU_PAGE_SHADER)
   {
      bool controls = hh_quick_menu_is_controls_page(menu);
      count = controls ? hh_quick_menu_control_count(menu) : menu->view.shader_count;
      rows = controls ? HH_QUICK_MENU_CONTROL_ROWS : HH_QUICK_MENU_SHADER_ROWS;
      maximum = count > rows ? (float)(count - rows) : 0;
      scroll = (float)(controls ? menu->view.control_first : menu->view.shader_first)
         + menu->list_scroll_fraction - dy / ((controls ? 70 : 76) * layout->scale);
      scroll = hh_quick_menu_clamp_scroll(scroll, maximum);
      menu->list_scroll_fraction = scroll - (float)(unsigned)scroll;
      if (controls)
         menu->view.control_first = (unsigned)scroll;
      else
         menu->view.shader_first = (size_t)scroll;
   }
   else if (menu->view.page == HH_QUICK_MENU_PAGE_MAIN)
   {
      menu->main_scroll = hh_quick_menu_clamp_scroll(menu->main_scroll
            - dx / (240 * layout->scale), HH_QUICK_MENU_ITEM_COUNT - 5);
      menu->main_scroll_origin = menu->main_scroll_target = menu->main_scroll;
   }
   else if (menu->view.page == HH_QUICK_MENU_PAGE_SAVE
         || menu->view.page == HH_QUICK_MENU_PAGE_LOAD
         || menu->view.page == HH_QUICK_MENU_PAGE_RECENT)
   {
      count = menu->view.page == HH_QUICK_MENU_PAGE_RECENT
         ? menu->view.recent_count : HH_QUICK_MENU_SLOT_COUNT;
      maximum = count > 5 ? (float)(count - 5) : 0;
      menu->slot_scroll = hh_quick_menu_clamp_scroll(menu->slot_scroll
            - dx / (320 * layout->scale), maximum);
      menu->slot_scroll_origin = menu->slot_scroll_target = menu->slot_scroll;
      if (menu->view.page == HH_QUICK_MENU_PAGE_RECENT)
      {
         menu->view.recent_first = (size_t)menu->slot_scroll;
         if (menu->view.recent_first)
            menu->view.recent_first--;
      }
   }
   else
      return;
   menu->touch_scrolled = true;
   menu->touch_scroll_page = menu->view.page;
}

static size_t hh_quick_menu_touch_index(const hh_quick_menu_t *menu)
{
   if (menu->state == HH_QUICK_MENU_CONFIRM_DIALOG)
      return menu->view.dialog.confirm_selected;
   if (hh_quick_menu_is_controls_page(menu))
      return menu->view.control_selected;
   switch (menu->view.page)
   {
      case HH_QUICK_MENU_PAGE_MAIN: return menu->view.selected_index;
      case HH_QUICK_MENU_PAGE_RECENT: return menu->view.recent_selected;
      case HH_QUICK_MENU_PAGE_SHADER: return menu->view.shader_selected;
      case HH_QUICK_MENU_PAGE_SHADER_SCOPE: return menu->view.shader_scope;
      default: return (size_t)menu->view.state_slot;
   }
}

bool hh_quick_menu_touch(hh_quick_menu_t *menu, float x, float y,
      bool pressed, float width, float height, hh_quick_menu_input_t *input)
{
   hh_quick_menu_layout_t layout;
   float dx, dy, threshold;
   hh_quick_menu_input_t released;
   if (input)
      *input = HH_QUICK_MENU_INPUT_NONE;
   if (!menu || !input || !hh_quick_menu_compute_layout(width, height, &layout))
      return false;
   if (menu->view.busy || (menu->state != HH_QUICK_MENU_OPEN
            && menu->state != HH_QUICK_MENU_CONFIRM_DIALOG))
   {
      menu->touch_active = false;
      return false;
   }
   if (!menu->touch_active)
   {
      if (!pressed)
         return false;
      if (menu->touch_scroll_page != menu->view.page)
         menu->list_scroll_fraction = 0;
      menu->touch_active = true;
      menu->touch_dragged = false;
      menu->touch_start_x = menu->touch_last_x = x;
      menu->touch_start_y = menu->touch_last_y = y;
      menu->touch_input = hh_quick_menu_touch_focus(menu, &layout, x, y);
      menu->touch_index = hh_quick_menu_touch_index(menu);
      return true;
   }
   dx = x - menu->touch_start_x;
   dy = y - menu->touch_start_y;
   threshold = 24 * layout.scale;
   if (!menu->touch_dragged && dx * dx + dy * dy > threshold * threshold)
   {
      menu->touch_dragged = true;
      hh_quick_menu_touch_scroll(menu, &layout, dx, dy);
   }
   else if (menu->touch_dragged)
      hh_quick_menu_touch_scroll(menu, &layout,
            x - menu->touch_last_x, y - menu->touch_last_y);
   menu->touch_last_x = x;
   menu->touch_last_y = y;
   if (!pressed)
   {
      menu->touch_active = false;
      if (!menu->touch_dragged)
      {
         released = hh_quick_menu_touch_focus(menu, &layout, x, y);
         if (released == menu->touch_input
               && menu->touch_index == hh_quick_menu_touch_index(menu))
            *input = released;
      }
      menu->touch_dragged = false;
   }
   return true;
}

hh_quick_menu_state_t hh_quick_menu_get_state(
      const hh_quick_menu_t *menu)
{
   return menu ? menu->state : HH_QUICK_MENU_CLOSED;
}

const hh_quick_menu_view_t *hh_quick_menu_get_view(
      const hh_quick_menu_t *menu)
{
   return menu ? &menu->view : (const hh_quick_menu_view_t *)0;
}
