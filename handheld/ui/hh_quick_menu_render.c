#include "hh_quick_menu.h"

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
