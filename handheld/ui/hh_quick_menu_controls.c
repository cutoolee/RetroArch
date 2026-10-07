#include "hh_quick_menu_internal.h"

#include <stdio.h>
#include <string.h>

static const char *hh_control_modes[] = {"原有映射", "普通按键", "按住连发", "组合按键"};

typedef struct hh_control_target
{
   unsigned mask;
   const char *name;
} hh_control_target_t;

static unsigned hh_control_target_rank(const char *name)
{
   static const char *names[] = {"A", "B", "C", "D", "X", "Y",
      "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12",
      "L", "R", "L1", "R1", "L2", "R2", "L3", "R3",
      "Turbo A", "Turbo B", "Turbo C", "Turbo D", "Turbo X", "Turbo Y",
      "Turbo L", "Turbo R", "Turbo L1", "Turbo R1", "Turbo L2", "Turbo R2",
      "AB", "CD", "ABC", "BCD", "ABCD",
      "Turbo AB", "Turbo CD", "Turbo ABC", "Turbo BCD", "Turbo ABCD",
      "Select", "Start", "Up", "Down", "Left", "Right"};
   char key[64];
   unsigned i, length = 0;
   if (!strncmp(name, "Button ", 7))
      name += 7;
   else if (!strncmp(name, "Buttons ", 8))
      name += 8;
   else if (!strncmp(name, "D-Pad ", 6))
      name += 6;
   for (i = 0; name[i] && length < sizeof(key) - 1; i++)
      if (name[i] != '+')
         key[length++] = name[i];
   key[length] = '\0';
   name = key;
   for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)
      if (!strcmp(name, names[i]) || (names[i][1] == '\0'
               && name[0] == names[i][0] && !strncmp(name + 1, " / ", 3)))
         return i;
   return i;
}

static unsigned hh_control_targets(const hh_quick_menu_t *menu,
      hh_control_target_t *targets)
{
   unsigned i, j, rank, count = 0, abcd = 0, buttons = 0;
   bool has_abcd = false;
   hh_control_target_t target;
   for (i = 0; i < 16; i++)
   {
      if (!menu->view.controls.available[i])
         continue;
      targets[count].mask = 1U << i;
      targets[count++].name = menu->view.controls.targets[i];
      rank = hh_control_target_rank(menu->view.controls.targets[i]);
      if (rank < 4 && !(buttons & (1U << rank)))
      {
         abcd |= 1U << i;
         buttons |= 1U << rank;
      }
      if (rank == hh_control_target_rank("ABCD"))
         has_abcd = true;
   }
   if (buttons == 15 && !has_abcd)
   {
      targets[count].mask = abcd;
      targets[count++].name = "Buttons ABCD";
   }
   for (i = 1; i < count; i++)
   {
      target = targets[i];
      rank = hh_control_target_rank(target.name);
      for (j = i; j > 0 && hh_control_target_rank(targets[j - 1].name) > rank; j--)
      {
         targets[j] = targets[j - 1];
      }
      targets[j] = target;
   }
   return count;
}

bool hh_quick_menu_is_controls_page(const hh_quick_menu_t *menu)
{
   return menu && (menu->view.page == HH_QUICK_MENU_PAGE_CONTROLS
         || menu->view.page == HH_QUICK_MENU_PAGE_CONTROL_EDIT);
}

unsigned hh_quick_menu_control_count(const hh_quick_menu_t *menu)
{
   hh_control_target_t targets[17];
   if (menu->view.page == HH_QUICK_MENU_PAGE_CONTROLS)
      return 20;
   return 2 + hh_control_targets(menu, targets);
}

static bool hh_control_target(const hh_quick_menu_t *menu, unsigned row,
      hh_control_target_t *target)
{
   hh_control_target_t targets[17];
   unsigned count = hh_control_targets(menu, targets);
   if (row < 2 || row - 2 >= count)
      return false;
   *target = targets[row - 2];
   return true;
}

static hh_ui_action_t hh_control_pending(hh_quick_menu_t *menu, hh_ui_action_t action)
{
   hh_quick_menu_clear_feedback(menu);
   menu->view.pending_action = action;
   menu->state = HH_QUICK_MENU_ACTION_PENDING;
   return action;
}

static void hh_control_mode_change(hh_quick_menu_t *menu, int direction)
{
   hh_quick_menu_view_t *v = &menu->view;
   unsigned i, mask = 0;
   v->control_mode = (v->control_mode + (direction > 0 ? 1 : 3)) % 4;
   for (i = 0; i < 16; i++)
      if (v->controls.available[i])
         mask |= 1U << i;
   v->control_mask &= mask;
   if (v->control_mode == 1 || v->control_mode == 2)
   {
      if (!v->control_mask)
         v->control_mask = mask;
      v->control_mask &= (~v->control_mask + 1U);
   }
}

static void hh_control_move(hh_quick_menu_t *menu,
      hh_quick_menu_input_t input)
{
   hh_quick_menu_layout_t layout;
   hh_ui_rect_t current, next;
   unsigned i, selected = menu->view.control_selected;
   float dx, dy, along, across, score, best = 1000000000.0f;
   bool horizontal = input == HH_QUICK_MENU_INPUT_LEFT
      || input == HH_QUICK_MENU_INPUT_RIGHT;
   bool reverse = input == HH_QUICK_MENU_INPUT_LEFT
      || input == HH_QUICK_MENU_INPUT_UP;
   if (selected < 2 && input == HH_QUICK_MENU_INPUT_UP)
   {
      menu->view.control_selected = selected == 0 ? 1 : 18;
      hh_quick_menu_clear_feedback(menu);
      return;
   }
   hh_quick_menu_compute_layout(1920, 1080, &layout);
   if (!hh_quick_menu_control_bounds(&layout, selected, &current))
      return;
   for (i = 0; i < 20; i++)
   {
      hh_quick_menu_control_bounds(&layout, i, &next);
      dx = next.x + next.width / 2 - current.x - current.width / 2;
      dy = next.y + next.height / 2 - current.y - current.height / 2;
      along = horizontal ? dx : dy;
      across = horizontal ? dy : dx;
      if (reverse)
         along = -along;
      if (along <= 1)
         continue;
      score = along * along + across * across * 4;
      if (score < best)
      {
         best = score;
         selected = i;
      }
   }
   menu->view.control_selected = selected;
   menu->view.control_first = 0;
   hh_quick_menu_clear_feedback(menu);
}

hh_ui_action_t hh_quick_menu_controls_input(hh_quick_menu_t *menu,
      hh_quick_menu_input_t input)
{
   hh_quick_menu_view_t *v = &menu->view;
   unsigned count = hh_quick_menu_control_count(menu);
   unsigned row = v->control_selected;
   unsigned i;
   hh_control_target_t target;
   bool edit = v->page == HH_QUICK_MENU_PAGE_CONTROL_EDIT;
   int direction = input == HH_QUICK_MENU_INPUT_LEFT ? -1 : 1;
   if (input == HH_QUICK_MENU_INPUT_BACK)
   {
      v->page = edit ? HH_QUICK_MENU_PAGE_CONTROLS : HH_QUICK_MENU_PAGE_MAIN;
      v->control_selected = edit ? v->control_source + 2 : 0;
      v->control_first = 0;
      return HH_UI_ACTION_NONE;
   }
   if (!edit && (input == HH_QUICK_MENU_INPUT_UP
            || input == HH_QUICK_MENU_INPUT_DOWN
            || ((input == HH_QUICK_MENU_INPUT_LEFT
                  || input == HH_QUICK_MENU_INPUT_RIGHT) && row >= 2)))
   {
      hh_control_move(menu, input);
      return HH_UI_ACTION_NONE;
   }
   if (input == HH_QUICK_MENU_INPUT_UP || input == HH_QUICK_MENU_INPUT_DOWN)
   {
      v->control_selected = input == HH_QUICK_MENU_INPUT_UP
         ? (row ? row - 1 : count - 1) : (row + 1) % count;
      if (v->control_selected < v->control_first)
         v->control_first = v->control_selected;
      else if (v->control_selected >= v->control_first + HH_QUICK_MENU_CONTROL_ROWS)
         v->control_first = v->control_selected - HH_QUICK_MENU_CONTROL_ROWS + 1;
      hh_quick_menu_clear_feedback(menu);
      return HH_UI_ACTION_NONE;
   }
   if (input != HH_QUICK_MENU_INPUT_LEFT && input != HH_QUICK_MENU_INPUT_RIGHT
         && input != HH_QUICK_MENU_INPUT_CONFIRM)
      return HH_UI_ACTION_NONE;
   if (!v->controls_valid && row != 0)
      return HH_UI_ACTION_NONE;
   if (!edit)
   {
      if (!row && v->controls.player_count)
      {
         v->control_player = (v->control_player + (direction > 0 ? 1 :
                  v->controls.player_count - 1)) % v->controls.player_count;
         return HH_UI_ACTION_NONE;
      }
      if (row == 1)
      {
         for (i = 0; i < v->controls.device_count; i++)
            if (v->controls.device_ids[i] == v->controls.device_index)
               break;
         if (!v->controls.device_count)
            return HH_UI_ACTION_NONE;
         i = i >= v->controls.device_count ? 0 : (i + (direction > 0 ? 1 :
                  v->controls.device_count - 1)) % v->controls.device_count;
         v->control_device = v->controls.device_ids[i];
         return hh_control_pending(menu, HH_UI_ACTION_CONTROLS_DEVICE);
      }
      if (input != HH_QUICK_MENU_INPUT_CONFIRM)
         return HH_UI_ACTION_NONE;
      if (row >= 18)
      {
         v->control_save_scope = row - 18;
         return hh_control_pending(menu, HH_UI_ACTION_CONTROLS_SAVE);
      }
      v->control_source = row - 2;
      v->control_mask = v->controls.masks[v->control_source];
      v->control_period = v->controls.periods[v->control_source];
      v->control_mode = !v->controls.custom[v->control_source] ? 0 :
         v->control_period ? 2 : (v->control_mask & (v->control_mask - 1)) ? 3 : 1;
      if (!v->control_period)
         v->control_period = 6;
      v->page = HH_QUICK_MENU_PAGE_CONTROL_EDIT;
      v->control_selected = v->control_first = 0;
      return HH_UI_ACTION_NONE;
   }
   if (!row)
      hh_control_mode_change(menu, direction);
   else if (row == 1 && v->control_mode == 2)
   {
      static const unsigned periods[] = {12, 8, 6, 4, 2};
      for (i = 0; i < 5; i++)
         if (periods[i] == v->control_period)
            break;
      v->control_period = periods[i >= 5 ? 2 : (i + (direction > 0 ? 1 : 4)) % 5];
   }
   else if (row >= 2 && v->control_mode)
   {
      if (hh_control_target(menu, row, &target))
      {
         if (v->control_mode == 3)
         {
            if ((v->control_mask & target.mask) == target.mask)
            {
               if (!(v->control_mask & ~target.mask))
               {
                  v->feedback = HH_QUICK_MENU_ACTION_ERROR;
                  strcpy(v->message, "至少保留一个按键");
                  return HH_UI_ACTION_NONE;
               }
               v->control_mask &= ~target.mask;
            }
            else
               v->control_mask |= target.mask;
         }
         else
         {
            v->control_mask = target.mask;
            if (v->control_mode == 1 && (target.mask & (target.mask - 1U)))
               v->control_mode = 3;
         }
      }
      else
         return HH_UI_ACTION_NONE;
   }
   else
      return HH_UI_ACTION_NONE;
   return hh_control_pending(menu, HH_UI_ACTION_CONTROLS_SET);
}

void hh_quick_menu_control_label(const hh_quick_menu_t *menu, unsigned row,
      char *label, size_t size)
{
   const hh_quick_menu_view_t *v = &menu->view;
   unsigned i, source, mask;
   char action[256];
   hh_control_target_t target;
   size_t length;
   if (v->page == HH_QUICK_MENU_PAGE_CONTROL_EDIT)
   {
      if (!row)
         snprintf(label, size, "行为：%s  < >", hh_control_modes[v->control_mode]);
      else if (row == 1)
         snprintf(label, size, v->control_mode == 2 ? "连发周期：%u 帧  < >" : "连发速度：仅连发模式可用", v->control_period);
      else
      {
         bool valid = hh_control_target(menu, row, &target);
         snprintf(label, size, "%s %s", valid
               && (v->control_mask & target.mask) == target.mask ? "[已选]" : "[未选]",
               valid ? target.name : "");
      }
      return;
   }
   if (!row)
      snprintf(label, size, "玩家 %u", v->control_player + 1);
   else if (!v->controls_valid)
      snprintf(label, size, "当前玩家的设备类型不支持按键配置");
   else if (row == 1)
   {
      const char *name = "请按一下手柄按键";
      for (i = 0; i < v->controls.device_count; i++)
         if (v->controls.device_ids[i] == v->controls.device_index)
            name = v->controls.devices[i];
      snprintf(label, size, "手柄：%s", name);
   }
   else if (row >= 18)
      snprintf(label, size, row == 18 ? "保存当前游戏" : "保存当前核心");
   else
   {
      source = row - 2;
      mask = v->controls.masks[source];
      action[0] = '\0';
      for (i = 0; i < 16; i++)
         if (mask & (1U << i))
         {
            length = strlen(action);
            snprintf(action + length, sizeof(action) - length, "%s%s",
                  length ? " + " : "", v->controls.targets[i]);
         }
      snprintf(label, size, "%s → %s%s", v->controls.sources[source],
            v->controls.periods[source] ? "连发 " : "", *action ? action : "未映射");
   }
}
