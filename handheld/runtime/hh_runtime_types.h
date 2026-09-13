/* Handheld Runtime API - stable product-facing value types. */
#ifndef HH_RUNTIME_TYPES_H
#define HH_RUNTIME_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HH_RUNTIME_CONTENT_NAME_MAX 256
#define HH_RUNTIME_CORE_NAME_MAX 128
#define HH_RUNTIME_CORE_VERSION_MAX 64

typedef enum hh_result
{
   HH_OK = 0,
   HH_ERR_NOT_INITIALIZED,
   HH_ERR_NO_CONTENT,
   HH_ERR_INVALID_STATE,
   HH_ERR_BUSY,
   HH_ERR_UNSUPPORTED,
   HH_ERR_INVALID_ARGUMENT,
   HH_ERR_SAVE_FAILED,
   HH_ERR_LOAD_FAILED,
   HH_ERR_SCREENSHOT_FAILED,
   HH_ERR_RA_COMMAND_FAILED,
   HH_ERR_INTERNAL
} hh_result_t;

typedef enum hh_runtime_mode
{
   HH_RUNTIME_MODE_UNINITIALIZED = 0,
   HH_RUNTIME_MODE_READY_NO_CONTENT,
   HH_RUNTIME_MODE_CONTENT_RUNNING,
   HH_RUNTIME_MODE_CONTENT_PAUSED,
   HH_RUNTIME_MODE_RA_MENU,
   HH_RUNTIME_MODE_CLOSING_CONTENT,
   HH_RUNTIME_MODE_ERROR
} hh_runtime_mode_t;

typedef enum hh_capability
{
   HH_CAP_PAUSE = 0,
   HH_CAP_RESET,
   HH_CAP_SAVE_STATE,
   HH_CAP_LOAD_STATE,
   HH_CAP_SCREENSHOT,
   HH_CAP_RA_MENU,
   HH_CAP_COUNT
} hh_capability_t;

typedef enum hh_command_type
{
   HH_CMD_NONE = 0,
   HH_CMD_PAUSE,
   HH_CMD_RESUME,
   HH_CMD_RESET,
   HH_CMD_CLOSE_CONTENT,
   HH_CMD_SET_STATE_SLOT,
   HH_CMD_SAVE_STATE,
   HH_CMD_LOAD_STATE,
   HH_CMD_SCREENSHOT,
   HH_CMD_OPEN_RA_MENU
} hh_command_type_t;

typedef struct hh_runtime_snapshot
{
   bool initialized;
   bool content_loaded;
   bool paused;
   bool ra_menu_open;
   int state_slot;
   char content_name[HH_RUNTIME_CONTENT_NAME_MAX];
   char core_name[HH_RUNTIME_CORE_NAME_MAX];
   char core_version[HH_RUNTIME_CORE_VERSION_MAX];
   hh_runtime_mode_t runtime_mode;
} hh_runtime_snapshot_t;

typedef enum hh_event_type
{
   HH_EVENT_RUNTIME_READY = 0,
   HH_EVENT_CONTENT_LOADED,
   HH_EVENT_CONTENT_CLOSED,
   HH_EVENT_RESET,
   HH_EVENT_PAUSED,
   HH_EVENT_RESUMED,
   HH_EVENT_STATE_SAVE_ACCEPTED,
   HH_EVENT_STATE_LOAD_ACCEPTED,
   HH_EVENT_SCREENSHOT_TAKEN,
   HH_EVENT_RA_MENU_OPENED,
   HH_EVENT_RA_MENU_CLOSED,
   HH_EVENT_ERROR,
   HH_EVENT_STATE_SLOT_CHANGED,
   /* ACCEPTED events above are queue acknowledgement only. These are emitted
    * by the state task callbacks after the final save/load result is known. */
   HH_EVENT_STATE_SAVE_COMPLETED,
   HH_EVENT_STATE_LOAD_COMPLETED
} hh_event_type_t;

/* P1 compatibility names; these events mean upstream command acceptance,
 * not completion of an asynchronous disk task. */
#define HH_EVENT_STATE_SAVED  HH_EVENT_STATE_SAVE_ACCEPTED
#define HH_EVENT_STATE_LOADED HH_EVENT_STATE_LOAD_ACCEPTED

typedef struct hh_runtime_event
{
   uint64_t request_id;
   hh_event_type_t type;
   /* COMPLETED events carry the final task result; ACCEPTED events carry
    * HH_OK only for queue acceptance. */
   hh_result_t result;
   hh_runtime_snapshot_t snapshot;
} hh_runtime_event_t;

typedef void (*hh_runtime_event_callback_t)(
      const hh_runtime_event_t *event, void *userdata);

#ifdef __cplusplus
}
#endif

#endif
