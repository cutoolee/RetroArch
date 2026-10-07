#ifndef HH_QUICK_MENU_TYPES_H
#define HH_QUICK_MENU_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#define HH_QUICK_MENU_ITEM_COUNT 9
#define HH_QUICK_MENU_CONTROL_ROWS 8
#define HH_QUICK_MENU_SHADER_COUNT 8
#define HH_QUICK_MENU_SHADER_ROWS 4
#define HH_QUICK_MENU_SHADER_SCOPES 7
#define HH_QUICK_MENU_LABEL_MAX 32
#define HH_QUICK_MENU_MESSAGE_MAX 64
#define HH_QUICK_MENU_TEXT_MAX 128
#define HH_QUICK_MENU_SLOT_COUNT 10
#define HH_QUICK_MENU_RECENT_ROWS 8
#define HH_QUICK_MENU_PATH_MAX 4096

typedef enum hh_quick_menu_page
{
   HH_QUICK_MENU_PAGE_MAIN = 0,
   HH_QUICK_MENU_PAGE_SAVE,
   HH_QUICK_MENU_PAGE_LOAD,
   HH_QUICK_MENU_PAGE_RECENT,
   HH_QUICK_MENU_PAGE_SHADER,
   HH_QUICK_MENU_PAGE_SHADER_SCOPE,
   HH_QUICK_MENU_PAGE_CONTROLS,
   HH_QUICK_MENU_PAGE_CONTROL_EDIT
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
   HH_QUICK_MENU_INPUT_NONE = -1,
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
   bool shader_enabled;
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
   HH_UI_ACTION_EXIT,
   HH_UI_ACTION_RECENT,
   HH_UI_ACTION_SHADER_BEGIN,
   HH_UI_ACTION_SHADER_APPLY,
   HH_UI_ACTION_SHADER_CANCEL,
   HH_UI_ACTION_CONTROLS_BEGIN,
   HH_UI_ACTION_CONTROLS_SET,
   HH_UI_ACTION_CONTROLS_SAVE,
   HH_UI_ACTION_CONTROLS_DEVICE
} hh_ui_action_t;

typedef enum hh_quick_menu_item_id
{
   HH_QUICK_MENU_ITEM_CONTINUE = 0,
   HH_QUICK_MENU_ITEM_SAVE,
   HH_QUICK_MENU_ITEM_LOAD,
   HH_QUICK_MENU_ITEM_SHADER,
   HH_QUICK_MENU_ITEM_CONTROLS,
   HH_QUICK_MENU_ITEM_RESET,
   HH_QUICK_MENU_ITEM_ADVANCED_MENU,
   HH_QUICK_MENU_ITEM_RECENT,
   HH_QUICK_MENU_ITEM_EXIT
} hh_quick_menu_item_id_t;

typedef struct hh_quick_menu_item
{
   hh_quick_menu_item_id_t id;
   char label[HH_QUICK_MENU_LABEL_MAX];
   bool disabled;
   bool visible;
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

typedef struct hh_quick_menu_recent
{
   char title[HH_QUICK_MENU_TEXT_MAX * 2];
   char thumbnail[HH_QUICK_MENU_PATH_MAX];
   char video[HH_QUICK_MENU_PATH_MAX];
   bool disabled;
} hh_quick_menu_recent_t;

typedef struct hh_quick_menu_shader
{
   int id;
   char name[64];
   char description[128];
   char detail[128];
} hh_quick_menu_shader_t;

typedef struct hh_quick_menu_controls
{
   unsigned masks[16], periods[16];
   bool custom[16], available[16];
   char sources[16][64], targets[16][64];
   char devices[16][128];
   unsigned device_ids[16], device_count, device_index, player_count;
} hh_quick_menu_controls_t;

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
   hh_quick_menu_recent_t recent[HH_QUICK_MENU_RECENT_ROWS];
   size_t recent_count;
   size_t recent_selected;
   size_t recent_first;
   char recent_thumbnail[HH_QUICK_MENU_PATH_MAX];
   char recent_video[HH_QUICK_MENU_PATH_MAX];
   hh_quick_menu_shader_t shaders[HH_QUICK_MENU_SHADER_COUNT];
   size_t shader_count, shader_selected, shader_first;
   int shader_active_id;
   bool shader_valid, shader_recommended, shader_fullscreen;
   bool shader_removable[3];
   size_t shader_scope;
   char shader_source[128];
   hh_quick_menu_controls_t controls;
   unsigned control_player, control_selected, control_first, control_source;
   unsigned control_mode, control_mask, control_period, control_device;
   unsigned control_save_scope;
   bool controls_valid, controls_dirty;
} hh_quick_menu_view_t;

#endif
