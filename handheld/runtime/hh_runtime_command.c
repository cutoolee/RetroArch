#include "hh_runtime_internal.h"

#include "defaults.h"
#include "tasks/task_content.h"

static hh_result_t hh_runtime_load_recent(int index)
{
#ifdef HAVE_MENU
   const struct playlist_entry *entry = NULL;
   content_ctx_info_t content_info;
   char content_path[PATH_MAX_LENGTH];
   char core_path[PATH_MAX_LENGTH];
   char label[NAME_MAX_LENGTH];
   hh_result_t result;
   size_t entry_index;

   if (!g_defaults.content_history || index < 0
         || (size_t)index >= playlist_size(g_defaults.content_history))
      return HH_ERR_INVALID_ARGUMENT;
   entry_index = hh_runtime_recent_entry_index((size_t)index);
   if (entry_index == (size_t)-1)
      return HH_ERR_INVALID_ARGUMENT;
   playlist_get_index(g_defaults.content_history, entry_index, &entry);
   result = hh_runtime_recent_paths(entry, content_path, sizeof(content_path),
         core_path, sizeof(core_path));
   if (result != HH_OK)
      return result;
   strlcpy(label, entry->label ? entry->label : "", sizeof(label));
   memset(&content_info, 0, sizeof(content_info));
   hh_runtime_shader_discard();
   hh_runtime_controls_discard();
   hh_runtime_state_task_cancel(HH_ERR_NO_CONTENT);
   if (!task_push_load_content_from_playlist_from_menu(core_path, content_path,
            *label ? label : NULL, &content_info, NULL, NULL))
      return HH_ERR_LOAD_FAILED;
   command_event(CMD_EVENT_UNPAUSE, NULL);
   return HH_OK;
#else
   (void)index;
   return HH_ERR_UNSUPPORTED;
#endif
}

static hh_result_t hh_runtime_command_result(
      hh_command_type_t type, bool command_ok)
{
   /* CMD_EVENT_RESET performs the reset and intentionally returns false in
    * upstream command_event(); false is not a failure for this command. */
   if (type == HH_CMD_RESET)
      return HH_OK;
   if (command_ok)
      return HH_OK;
   if (type == HH_CMD_SAVE_STATE)
      return HH_ERR_SAVE_FAILED;
   if (type == HH_CMD_LOAD_STATE)
      return HH_ERR_LOAD_FAILED;
   if (type == HH_CMD_SCREENSHOT)
      return HH_ERR_SCREENSHOT_FAILED;
   return HH_ERR_RA_COMMAND_FAILED;
}

bool hh_runtime_command_is_state_io(hh_command_type_t type)
{
   return type == HH_CMD_SAVE_STATE || type == HH_CMD_LOAD_STATE;
}

hh_result_t hh_runtime_execute_command(
      hh_command_type_t type, int int_arg)
{
   hh_runtime_snapshot_t snapshot;
   settings_t *settings;
   bool command_ok = false;
   hh_result_t result;

   if (!hh_runtime_state.lock)
      return HH_ERR_NOT_INITIALIZED;

   hh_runtime_refresh_snapshot();
   if (hh_runtime_get_snapshot(&snapshot) != HH_OK)
      return HH_ERR_NOT_INITIALIZED;

   if (type != HH_CMD_SET_STATE_SLOT
         && type != HH_CMD_OPEN_RA_MENU
         && type != HH_CMD_QUIT
         && type != HH_CMD_LOAD_RECENT
         && !snapshot.content_loaded)
      return HH_ERR_NO_CONTENT;

   switch (type)
   {
      case HH_CMD_CONTROLS_SET:
      case HH_CMD_CONTROLS_SAVE:
      case HH_CMD_CONTROLS_DEVICE:
         return hh_runtime_controls_command(type, int_arg);
      case HH_CMD_SHADER_BEGIN:
      case HH_CMD_SHADER_PREVIEW:
      case HH_CMD_SHADER_APPLY:
      case HH_CMD_SHADER_REMOVE:
      case HH_CMD_SHADER_CANCEL:
         return hh_runtime_shader_command(type, int_arg);
      case HH_CMD_PAUSE:
         if (snapshot.paused)
            return HH_ERR_INVALID_STATE;
         command_ok = command_event(CMD_EVENT_PAUSE, NULL);
         break;
      case HH_CMD_RESUME:
         if (!snapshot.paused)
            return HH_OK;
         command_ok = command_event(CMD_EVENT_UNPAUSE, NULL);
         break;
      case HH_CMD_RESET:
         command_ok = command_event(CMD_EVENT_RESET, NULL);
         break;
      case HH_CMD_CLOSE_CONTENT:
         hh_runtime_shader_discard();
         hh_runtime_controls_discard();
         hh_runtime_state_task_cancel(HH_ERR_NO_CONTENT);
         command_ok = command_event(CMD_EVENT_CLOSE_CONTENT, NULL);
         break;
      case HH_CMD_QUIT:
         hh_runtime_shader_discard();
         hh_runtime_controls_discard();
         hh_runtime_state_task_cancel(HH_ERR_NO_CONTENT);
         command_ok = command_event(CMD_EVENT_QUIT, NULL);
         break;
      case HH_CMD_LOAD_RECENT:
         result = hh_runtime_load_recent(int_arg);
         hh_runtime_refresh_snapshot();
         return result;
      case HH_CMD_SET_STATE_SLOT:
         if (int_arg < -1 || int_arg > 999)
            return HH_ERR_INVALID_ARGUMENT;
         settings = config_get_ptr();
         if (!settings)
            return HH_ERR_INTERNAL;
         configuration_set_int(settings, settings->ints.state_slot, int_arg);
         hh_runtime_refresh_snapshot();
         return HH_OK;
      case HH_CMD_SAVE_STATE:
         command_ok = command_event(CMD_EVENT_SAVE_STATE, NULL);
         break;
      case HH_CMD_LOAD_STATE:
         command_ok = command_event(CMD_EVENT_LOAD_STATE, NULL);
         break;
      case HH_CMD_SCREENSHOT:
#ifdef HAVE_SCREENSHOTS
         command_ok = command_event(CMD_EVENT_TAKE_SCREENSHOT, NULL);
#else
         return HH_ERR_UNSUPPORTED;
#endif
         break;
      case HH_CMD_OPEN_RA_MENU:
#ifdef HAVE_MENU
         if (snapshot.ra_menu_open)
            return HH_ERR_INVALID_STATE;
         command_ok = command_event(CMD_EVENT_MENU_TOGGLE, NULL);
#else
         return HH_ERR_UNSUPPORTED;
#endif
         break;
      default:
         return HH_ERR_INTERNAL;
   }

   result = hh_runtime_command_result(type, command_ok);
   hh_runtime_refresh_snapshot();
   if (result != HH_OK)
      hh_runtime_set_last_error(result);
   return result;
}

static hh_result_t hh_runtime_action(hh_command_type_t type)
{
   uint64_t request_id = 0;
   return hh_runtime_submit_command(type, 0, &request_id);
}

hh_result_t hh_runtime_pause(void)
{
   return hh_runtime_action(HH_CMD_PAUSE);
}

hh_result_t hh_runtime_resume(void)
{
   return hh_runtime_action(HH_CMD_RESUME);
}

hh_result_t hh_runtime_reset(void)
{
   return hh_runtime_action(HH_CMD_RESET);
}

hh_result_t hh_runtime_close_content(void)
{
   return hh_runtime_action(HH_CMD_CLOSE_CONTENT);
}
