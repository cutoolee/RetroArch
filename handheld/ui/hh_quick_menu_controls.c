#include "hh_quick_menu_internal.h"

#include <stdio.h>
#include <string.h>

static const char *hh_control_modes[] = {"原有映射", "普通按键", "按住连发", "组合按键"};

bool hh_quick_menu_is_controls_page(const hh_quick_menu_t *menu)
{
   return menu && (menu->view.page == HH_QUICK_MENU_PAGE_CONTROLS
         || menu->view.page == HH_QUICK_MENU_PAGE_CONTROL_EDIT);
}

unsigned hh_quick_menu_control_count(const hh_quick_menu_t *menu)
{
   unsigned i, count = 3;
   if (menu->view.page == HH_QUICK_MENU_PAGE_CONTROLS)
      return 20;
   for (i = 0; i < 16; i++)
      if (menu->view.controls.available[i])
         count++;
   return count;
}

static unsigned hh_control_target(const hh_quick_menu_t *menu, unsigned row)
{
   unsigned i, index = 2;
   for (i = 0; i < 16; i++)
      if (menu->view.controls.available[i] && index++ == row)
         return i;
   return 16;
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

hh_ui_action_t hh_quick_menu_controls_input(hh_quick_menu_t *menu,
      hh_quick_menu_input_t input)
{
   hh_quick_menu_view_t *v = &menu->view;
   unsigned count = hh_quick_menu_control_count(menu);
   unsigned row = v->control_selected;
   unsigned target, i;
   bool edit = v->page == HH_QUICK_MENU_PAGE_CONTROL_EDIT;
   int direction = input == HH_QUICK_MENU_INPUT_LEFT ? -1 : 1;
   if (input == HH_QUICK_MENU_INPUT_BACK)
   {
      v->page = edit ? HH_QUICK_MENU_PAGE_CONTROLS : HH_QUICK_MENU_PAGE_MAIN;
      v->control_selected = edit ? v->control_source + 2 : 0;
      v->control_first = v->control_selected >= HH_QUICK_MENU_CONTROL_ROWS ? v->control_selected - HH_QUICK_MENU_CONTROL_ROWS + 1 : 0;
      return HH_UI_ACTION_NONE;
   }
   if (input == HH_QUICK_MENU_INPUT_UP || input == HH_QUICK_MENU_INPUT_DOWN)
   {
      if (input == HH_QUICK_MENU_INPUT_UP && row)
         v->control_selected--;
      else if (input == HH_QUICK_MENU_INPUT_DOWN && row + 1 < count)
         v->control_selected++;
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
   else if (row + 1 == count)
   {
      if (input != HH_QUICK_MENU_INPUT_CONFIRM)
         return HH_UI_ACTION_NONE;
      if (v->control_mode && !v->control_mask)
      {
         v->feedback = HH_QUICK_MENU_ACTION_ERROR;
         strcpy(v->message, "请选择至少一个游戏按键");
         return HH_UI_ACTION_NONE;
      }
      return hh_control_pending(menu, HH_UI_ACTION_CONTROLS_SET);
   }
   else if (row >= 2 && v->control_mode)
   {
      target = hh_control_target(menu, row);
      if (target < 16)
      {
         if (v->control_mode == 3)
            v->control_mask ^= 1U << target;
         else
            v->control_mask = 1U << target;
      }
   }
   return HH_UI_ACTION_NONE;
}

void hh_quick_menu_control_label(const hh_quick_menu_t *menu, unsigned row,
      char *label, size_t size)
{
   const hh_quick_menu_view_t *v = &menu->view;
   unsigned i, source, mask;
   char action[256];
   size_t length;
   if (v->page == HH_QUICK_MENU_PAGE_CONTROL_EDIT)
   {
      if (!row)
         snprintf(label, size, "行为：%s  < >", hh_control_modes[v->control_mode]);
      else if (row == 1)
         snprintf(label, size, v->control_mode == 2 ? "连发周期：%u 帧  < >" : "连发速度：仅连发模式可用", v->control_period);
      else if (row + 1 == hh_quick_menu_control_count(menu))
         snprintf(label, size, "应用并返回（随后可保存到游戏）");
      else
      {
         i = hh_control_target(menu, row);
         snprintf(label, size, "%s %s", i < 16 && (v->control_mask & (1U << i)) ? "[已选]" : "[未选]",
               i < 16 ? v->controls.targets[i] : "");
      }
      return;
   }
   if (!row)
      snprintf(label, size, "玩家：%u / %u  < >", v->control_player + 1, v->controls.player_count);
   else if (!v->controls_valid)
      snprintf(label, size, "当前玩家的设备类型不支持按键配置");
   else if (row == 1)
   {
      const char *name = "未检测到手柄（请按一下手柄按键）";
      for (i = 0; i < v->controls.device_count; i++)
         if (v->controls.device_ids[i] == v->controls.device_index)
            name = v->controls.devices[i];
      snprintf(label, size, "手柄：%s  < >", name);
   }
   else if (row >= 18)
      snprintf(label, size, row == 18 ? "保存到当前游戏" : "保存到当前核心（游戏配置优先）");
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
