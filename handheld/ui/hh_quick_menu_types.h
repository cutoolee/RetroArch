#ifndef HH_QUICK_MENU_TYPES_H
#define HH_QUICK_MENU_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#define HH_QUICK_MENU_ITEM_COUNT 6
#define HH_QUICK_MENU_LABEL_MAX 32
#define HH_QUICK_MENU_MESSAGE_MAX 64
#define HH_QUICK_MENU_TEXT_MAX 128
#define HH_QUICK_MENU_SLOT_COUNT 10

typedef enum hh_quick_menu_page
{
   HH_QUICK_MENU_PAGE_MAIN = 0,
   HH_QUICK_MENU_PAGE_SAVE,
   HH_QUICK_MENU_PAGE_LOAD
} hh_quick_menu_page_t;

typedef enum hh_quick_menu_item_state
{
   HH_QUICK_MENU_ITEM_NORMAL = 0,
   HH_QUICK_MENU_ITEM_FOCUSED,
   HH_QUICK_MENU_ITEM_DISABLED,
   HH_QUICK_MENU_ITEM_PENDING
} hh_quick_menu_item_state_t;

typedef enum hh_quick_menu_feedback
{
   HH_QUICK_MENU_FEEDBACK_NONE = 0,
   HH_QUICK_MENU_SAVE_SUCCESS,
   HH_QUICK_MENU_LOAD_SUCCESS,
   HH_QUICK_MENU_ACTION_SUCCESS,
   HH_QUICK_MENU_ACTION_ERROR
} hh_quick_menu_feedback_t;

typedef enum hh_quick_menu_input
{
   HH_QUICK_MENU_INPUT_UP = 0,
   HH_QUICK_MENU_INPUT_DOWN,
   HH_QUICK_MENU_INPUT_LEFT,
   HH_QUICK_MENU_INPUT_RIGHT,
   HH_QUICK_MENU_INPUT_CONFIRM,
   HH_QUICK_MENU_INPUT_BACK
} hh_quick_menu_input_t;

typedef struct hh_quick_menu_game
{
   char title[HH_QUICK_MENU_TEXT_MAX];
   char platform[HH_QUICK_MENU_TEXT_MAX];
   char subtitle[HH_QUICK_MENU_TEXT_MAX];
} hh_quick_menu_game_t;

typedef struct hh_quick_menu_slot
{
   int index;
   bool occupied;
   bool preview_available;
   bool disabled;
   char timestamp[HH_QUICK_MENU_MESSAGE_MAX];
   char label[HH_QUICK_MENU_TEXT_MAX];
} hh_quick_menu_slot_t;

typedef struct hh_quick_menu_capabilities
{
   bool save_enabled;
   bool load_enabled;
   bool screenshot_enabled;
   bool advanced_menu_enabled;
   bool reset_enabled;
} hh_quick_menu_capabilities_t;

typedef enum hh_quick_menu_state
{
   HH_QUICK_MENU_CLOSED = 0,
   HH_QUICK_MENU_OPENING,
   HH_QUICK_MENU_OPEN,
   HH_QUICK_MENU_CONFIRM_DIALOG,
   HH_QUICK_MENU_ACTION_PENDING,
   HH_QUICK_MENU_CLOSING
} hh_quick_menu_state_t;

typedef enum hh_ui_action
{
   HH_UI_ACTION_NONE = 0,
   HH_UI_ACTION_CONTINUE,
   HH_UI_ACTION_SAVE,
   HH_UI_ACTION_LOAD,
   HH_UI_ACTION_RESET,
   HH_UI_ACTION_ADVANCED_MENU,
   HH_UI_ACTION_EXIT
} hh_ui_action_t;

typedef enum hh_quick_menu_item_id
{
   HH_QUICK_MENU_ITEM_CONTINUE = 0,
   HH_QUICK_MENU_ITEM_SAVE,
   HH_QUICK_MENU_ITEM_LOAD,
   HH_QUICK_MENU_ITEM_RESET,
   HH_QUICK_MENU_ITEM_ADVANCED_MENU,
   HH_QUICK_MENU_ITEM_EXIT
} hh_quick_menu_item_id_t;

typedef struct hh_quick_menu_item
{
   hh_quick_menu_item_id_t id;
   char label[HH_QUICK_MENU_LABEL_MAX];
   bool disabled;
} hh_quick_menu_item_t;

typedef struct hh_quick_menu_dialog
{
   bool visible;
   hh_ui_action_t action;
   bool confirm_selected;
   char message[HH_QUICK_MENU_MESSAGE_MAX];
   char detail[HH_QUICK_MENU_TEXT_MAX];
   char confirm_label[HH_QUICK_MENU_LABEL_MAX];
} hh_quick_menu_dialog_t;

typedef struct hh_quick_menu_view
{
   char title[HH_QUICK_MENU_LABEL_MAX];
   hh_quick_menu_item_t items[HH_QUICK_MENU_ITEM_COUNT];
   size_t item_count;
   size_t selected_index;
   int state_slot;
   hh_quick_menu_dialog_t dialog;
   bool busy;
   char message[HH_QUICK_MENU_MESSAGE_MAX];
   hh_quick_menu_page_t page;
   hh_quick_menu_game_t game;
   hh_quick_menu_slot_t slots[HH_QUICK_MENU_SLOT_COUNT];
   hh_quick_menu_capabilities_t capabilities;
   hh_quick_menu_feedback_t feedback;
   hh_ui_action_t pending_action;
   int pending_slot;
} hh_quick_menu_view_t;

#endif
