#include "hh_runtime_internal.h"

static void hh_runtime_copy_name(char *dst, size_t dst_size, const char *src)
{
   if (!dst || !dst_size)
      return;
   if (!src)
      src = "";
   strlcpy(dst, src, dst_size);
}

void hh_runtime_refresh_snapshot(void)
{
   hh_runtime_snapshot_t snapshot;
   runloop_state_t *runloop_st;
   settings_t *settings;
   struct menu_state *menu_st;
   const char *content_path;
   bool content_loaded;

   memset(&snapshot, 0, sizeof(snapshot));
   snapshot.initialized = runloop_is_inited();
   snapshot.runtime_mode = snapshot.initialized
      ? HH_RUNTIME_MODE_READY_NO_CONTENT
      : HH_RUNTIME_MODE_UNINITIALIZED;
   snapshot.state_slot = 0;

   runloop_st = runloop_state_get_ptr();
   settings   = config_get_ptr();
   menu_st    = menu_state_get_ptr();

   if (settings)
      snapshot.state_slot = settings->ints.state_slot;

   content_path = path_get(RARCH_PATH_CONTENT);
   content_loaded = runloop_st
      && ((runloop_st->current_core.flags & RETRO_CORE_FLAG_GAME_LOADED)
         || (content_path && *content_path));
   snapshot.content_loaded = content_loaded;
   snapshot.paused = runloop_st
      && ((runloop_st->flags & RUNLOOP_FLAG_PAUSED) != 0);
   snapshot.ra_menu_open = menu_st
      && ((menu_st->flags & MENU_ST_FLAG_ALIVE) != 0);

   if (runloop_st)
   {
      hh_runtime_copy_name(snapshot.content_name,
            sizeof(snapshot.content_name),
            runloop_st->runtime_content_path_basename);
      hh_runtime_copy_name(snapshot.core_name,
            sizeof(snapshot.core_name),
            runloop_st->system.info.library_name);
      hh_runtime_copy_name(snapshot.core_version,
            sizeof(snapshot.core_version),
            runloop_st->system.info.library_version);
   }

   if (runloop_is_content_closing())
      snapshot.runtime_mode = HH_RUNTIME_MODE_CLOSING_CONTENT;
   else if (snapshot.ra_menu_open)
      snapshot.runtime_mode = HH_RUNTIME_MODE_RA_MENU;
   else if (content_loaded)
      snapshot.runtime_mode = snapshot.paused
         ? HH_RUNTIME_MODE_CONTENT_PAUSED
         : HH_RUNTIME_MODE_CONTENT_RUNNING;

   if (hh_runtime_state.lock)
   {
      slock_lock(hh_runtime_state.lock);
      snapshot.initialized = hh_runtime_state.initialized;
      if (!snapshot.initialized)
      {
         snapshot.runtime_mode = HH_RUNTIME_MODE_UNINITIALIZED;
         snapshot.content_loaded = false;
         snapshot.paused = false;
         snapshot.ra_menu_open = false;
      }
      hh_runtime_state.snapshot = snapshot;
      slock_unlock(hh_runtime_state.lock);
   }
}

hh_result_t hh_runtime_get_snapshot(hh_runtime_snapshot_t *out)
{
   if (!out)
      return HH_ERR_INVALID_ARGUMENT;
   if (!hh_runtime_state.lock)
      return HH_ERR_NOT_INITIALIZED;
   slock_lock(hh_runtime_state.lock);
   if (!hh_runtime_state.initialized)
   {
      slock_unlock(hh_runtime_state.lock);
      return HH_ERR_NOT_INITIALIZED;
   }
   *out = hh_runtime_state.snapshot;
   slock_unlock(hh_runtime_state.lock);
   return HH_OK;
}

hh_result_t hh_runtime_get_state_slot(int *slot)
{
   hh_runtime_snapshot_t snapshot;
   hh_result_t result;

   if (!slot)
      return HH_ERR_INVALID_ARGUMENT;
   result = hh_runtime_get_snapshot(&snapshot);
   if (result != HH_OK)
      return result;
   *slot = snapshot.state_slot;
   return HH_OK;
}

bool hh_runtime_has_capability(hh_capability_t capability)
{
   switch (capability)
   {
      case HH_CAP_PAUSE:
      case HH_CAP_RESET:
      case HH_CAP_SAVE_STATE:
      case HH_CAP_LOAD_STATE:
         return true;
      case HH_CAP_SCREENSHOT:
#ifdef HAVE_SCREENSHOTS
         return true;
#else
         return false;
#endif
      case HH_CAP_RA_MENU:
#ifdef HAVE_MENU
         return true;
#else
         return false;
#endif
      default:
         return false;
   }
}
