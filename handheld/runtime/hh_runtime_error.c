#include "hh_runtime_internal.h"

const char *hh_result_to_string(hh_result_t result)
{
   switch (result)
   {
      case HH_OK:                       return "OK";
      case HH_ERR_NOT_INITIALIZED:      return "NOT_INITIALIZED";
      case HH_ERR_NO_CONTENT:           return "NO_CONTENT";
      case HH_ERR_INVALID_STATE:        return "INVALID_STATE";
      case HH_ERR_BUSY:                return "BUSY";
      case HH_ERR_UNSUPPORTED:         return "UNSUPPORTED";
      case HH_ERR_INVALID_ARGUMENT:    return "INVALID_ARGUMENT";
      case HH_ERR_SAVE_FAILED:         return "SAVE_FAILED";
      case HH_ERR_LOAD_FAILED:         return "LOAD_FAILED";
      case HH_ERR_SCREENSHOT_FAILED:   return "SCREENSHOT_FAILED";
      case HH_ERR_RA_COMMAND_FAILED:  return "RA_COMMAND_FAILED";
      case HH_ERR_INTERNAL:            return "INTERNAL";
      default:                         return "UNKNOWN";
   }
}

const char *hh_runtime_mode_to_string(hh_runtime_mode_t mode)
{
   switch (mode)
   {
      case HH_RUNTIME_MODE_UNINITIALIZED:    return "UNINITIALIZED";
      case HH_RUNTIME_MODE_READY_NO_CONTENT: return "READY_NO_CONTENT";
      case HH_RUNTIME_MODE_CONTENT_RUNNING:  return "CONTENT_RUNNING";
      case HH_RUNTIME_MODE_CONTENT_PAUSED:   return "CONTENT_PAUSED";
      case HH_RUNTIME_MODE_RA_MENU:          return "RA_MENU";
      case HH_RUNTIME_MODE_CLOSING_CONTENT:  return "CLOSING_CONTENT";
      case HH_RUNTIME_MODE_ERROR:            return "ERROR";
      default:                               return "UNKNOWN";
   }
}

const char *hh_event_type_to_string(hh_event_type_t type)
{
   switch (type)
   {
      case HH_EVENT_RUNTIME_READY:    return "RUNTIME_READY";
      case HH_EVENT_CONTENT_LOADED:   return "CONTENT_LOADED";
      case HH_EVENT_CONTENT_CLOSED:   return "CONTENT_CLOSED";
      case HH_EVENT_RESET:            return "RESET";
      case HH_EVENT_PAUSED:           return "PAUSED";
      case HH_EVENT_RESUMED:          return "RESUMED";
      case HH_EVENT_STATE_SAVE_ACCEPTED: return "STATE_SAVE_ACCEPTED";
      case HH_EVENT_STATE_LOAD_ACCEPTED: return "STATE_LOAD_ACCEPTED";
      case HH_EVENT_SCREENSHOT_TAKEN: return "SCREENSHOT_TAKEN";
      case HH_EVENT_RA_MENU_OPENED:   return "RA_MENU_OPENED";
      case HH_EVENT_RA_MENU_CLOSED:   return "RA_MENU_CLOSED";
      case HH_EVENT_ERROR:            return "ERROR";
      case HH_EVENT_STATE_SLOT_CHANGED: return "STATE_SLOT_CHANGED";
      default:                        return "UNKNOWN";
   }
}
