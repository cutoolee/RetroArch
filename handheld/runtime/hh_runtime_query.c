#include "hh_runtime_internal.h"
#include "hh_runtime_metadata.h"

#include <stdlib.h>
#include <file/file_path.h>
#include <string/stdstring.h>
#include "defaults.h"
#include "gfx/gfx_thumbnail.h"

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
   if (!content_loaded || runloop_is_content_closing())
      hh_runtime_shader_discard();
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

hh_result_t hh_runtime_recent_paths(const struct playlist_entry *entry,
      char *content_path, size_t content_size,
      char *core_path, size_t core_size)
{
   core_info_t *core_info;
   const char *archive_delim;
   char archive_path[PATH_MAX_LENGTH];
   size_t length;

   content_path[0] = core_path[0] = '\0';
   if (!entry || string_is_empty(entry->path))
      return HH_ERR_LOAD_FAILED;
   if (strlcpy(content_path, entry->path, content_size) >= content_size)
      return HH_ERR_LOAD_FAILED;
   playlist_resolve_path(PLAYLIST_LOAD, false, content_path, content_size);
   if (!string_is_empty(entry->subsystem_ident))
      return HH_ERR_UNSUPPORTED;
   archive_delim = path_get_archive_delim(content_path);
   if (archive_delim)
   {
      length = (size_t)(archive_delim - content_path);
      if (!length || length >= sizeof(archive_path))
         return HH_ERR_LOAD_FAILED;
      memcpy(archive_path, content_path, length);
      archive_path[length] = '\0';
      if (!path_is_valid(archive_path))
         return HH_ERR_LOAD_FAILED;
   }
   else if (!path_is_valid(content_path))
      return HH_ERR_LOAD_FAILED;

   core_info = playlist_entry_has_core(entry)
      ? playlist_entry_get_core_info(entry)
      : playlist_get_default_core_info(g_defaults.content_history);
   if (core_info && !string_is_empty(core_info->path))
      strlcpy(core_path, core_info->path, core_size);
   else if (playlist_entry_has_core(entry))
   {
      strlcpy(core_path, entry->core_path, core_size);
      playlist_resolve_path(PLAYLIST_LOAD, true, core_path, core_size);
   }
   else
      return HH_ERR_UNSUPPORTED;
   if (!path_is_valid(core_path))
   {
      core_info = playlist_get_default_core_info(g_defaults.content_history);
      if (!core_info || string_is_empty(core_info->path))
         return HH_ERR_UNSUPPORTED;
      strlcpy(core_path, core_info->path, core_size);
      if (!path_is_valid(core_path))
         return HH_ERR_UNSUPPORTED;
   }
   return HH_OK;
}

static bool hh_runtime_recent_duplicate(size_t index)
{
   const struct playlist_entry *entry = NULL;
   const struct playlist_entry *previous = NULL;
   char path[PATH_MAX_LENGTH];
   char earlier[PATH_MAX_LENGTH];
   size_t i;
   playlist_get_index(g_defaults.content_history, index, &entry);
   if (!entry || string_is_empty(entry->path))
      return false;
   strlcpy(path, entry->path, sizeof(path));
   playlist_resolve_path(PLAYLIST_LOAD, false, path, sizeof(path));
   for (i = 0; i < index; i++)
   {
      playlist_get_index(g_defaults.content_history, i, &previous);
      if (!previous || string_is_empty(previous->path))
         continue;
      strlcpy(earlier, previous->path, sizeof(earlier));
      playlist_resolve_path(PLAYLIST_LOAD, false, earlier, sizeof(earlier));
      if (string_is_equal(path, earlier))
         return true;
   }
   return false;
}

size_t hh_runtime_recent_entry_index(size_t index)
{
   size_t i, visible = 0;
   if (g_defaults.content_history)
      for (i = 0; i < playlist_size(g_defaults.content_history); i++)
      {
         if (hh_runtime_recent_duplicate(i))
            continue;
         if (visible++ == index)
            return i;
      }
   return (size_t)-1;
}

hh_result_t hh_runtime_get_recent_count(size_t *count)
{
   hh_runtime_snapshot_t snapshot;
   hh_result_t result;
   size_t i;
   if (!count)
      return HH_ERR_INVALID_ARGUMENT;
   *count = 0;
   result = hh_runtime_get_snapshot(&snapshot);
   if (result != HH_OK)
      return result;
   if (g_defaults.content_history)
      for (i = 0; i < playlist_size(g_defaults.content_history); i++)
         if (!hh_runtime_recent_duplicate(i))
            (*count)++;
   return HH_OK;
}

hh_result_t hh_runtime_get_recent_game(size_t index,
      bool thumbnail, hh_runtime_recent_game_t *out)
{
   const struct playlist_entry *entry = NULL;
   gfx_thumbnail_path_data_t *path_data;
   char content_path[PATH_MAX_LENGTH];
   char core_path[PATH_MAX_LENGTH];
   size_t count;
   hh_result_t result;
   unsigned mode;

   if (!out)
      return HH_ERR_INVALID_ARGUMENT;
   memset(out, 0, sizeof(*out));
   result = hh_runtime_get_recent_count(&count);
   if (result != HH_OK)
      return result;
   if (index >= count)
      return HH_ERR_INVALID_ARGUMENT;
   index = hh_runtime_recent_entry_index(index);
   playlist_get_index(g_defaults.content_history, index, &entry);
   if (!entry)
      return HH_ERR_INVALID_ARGUMENT;
   if (!string_is_empty(entry->label))
      strlcpy(out->title, entry->label, sizeof(out->title));
   else if (!string_is_empty(entry->path))
      fill_pathname(out->title, path_basename(entry->path), "", sizeof(out->title));
   out->available = hh_runtime_recent_paths(entry, content_path,
         sizeof(content_path), core_path, sizeof(core_path)) == HH_OK;
   hh_runtime_metadata_lookup_media(content_path, out->title, sizeof(out->title),
         thumbnail ? out->thumbnail : NULL, thumbnail ? sizeof(out->thumbnail) : 0,
         thumbnail ? out->video : NULL, thumbnail ? sizeof(out->video) : 0);
#ifndef HAVE_MENU
   out->available = false;
#endif
   if (!thumbnail || *out->thumbnail || !config_get_ptr())
      return HH_OK;
   path_data = gfx_thumbnail_path_init();
   if (!path_data)
   {
      hh_runtime_recent_fallback(entry, content_path, core_path,
            out->thumbnail, sizeof(out->thumbnail));
      return HH_OK;
   }
   gfx_thumbnail_set_system(path_data, "history", g_defaults.content_history);
   if (gfx_thumbnail_set_content_playlist(path_data, g_defaults.content_history, index))
      for (mode = PLAYLIST_THUMBNAIL_MODE_SCREENSHOTS;
            mode <= PLAYLIST_THUMBNAIL_MODE_BOXARTS; mode++)
      {
         path_data->playlist_right_mode = (enum playlist_thumbnail_mode)mode;
         if (gfx_thumbnail_update_path(path_data, GFX_THUMBNAIL_RIGHT)
               && path_is_valid(path_data->right_path))
         {
            strlcpy(out->thumbnail, path_data->right_path, sizeof(out->thumbnail));
            break;
         }
      }
   free(path_data);
   if (!*out->thumbnail)
      hh_runtime_recent_fallback(entry, content_path, core_path,
            out->thumbnail, sizeof(out->thumbnail));
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
      case HH_CAP_SHADER:
         return hh_runtime_shader_supported();
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
