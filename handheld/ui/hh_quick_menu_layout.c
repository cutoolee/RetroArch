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
   if (!menu)
      return;
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
