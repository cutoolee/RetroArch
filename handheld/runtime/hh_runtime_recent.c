#include "hh_runtime_internal.h"
#include "hh_runtime_metadata.h"

#include <features/features_cpu.h>
#include <file/file_path.h>
#include <string/stdstring.h>
#include "tasks/tasks_internal.h"
#include "verbosity.h"

static void hh_recent_capture_path(const char *content, char *out, size_t size)
{
   const unsigned char *p = (const unsigned char*)content;
   const char *state_dir = dir_get_ptr(RARCH_DIR_SAVESTATE);
   char directory[PATH_MAX_LENGTH];
   char name[32];
   uint32_t hash = 2166136261U;
   while (*p && *p != '#')
      hash = (hash ^ *p++) * 16777619U;
   snprintf(name, sizeof(name), "%08x.png", (unsigned)hash);
   if (!string_is_empty(state_dir))
      strlcpy(directory, state_dir, sizeof(directory));
   else
      fill_pathname_basedir(directory, path_get(RARCH_PATH_CONFIG), sizeof(directory));
   if (!*directory)
   {
      out[0] = '\0';
      return;
   }
   fill_pathname_join_special(directory, directory,
         "gamego-recent", sizeof(directory));
   fill_pathname_join_special(out, directory, name, size);
}

void hh_runtime_recent_fallback(const struct playlist_entry *entry,
      const char *content_path, const char *core_path, char *out, size_t size)
{
   settings_t *settings = config_get_ptr();
   runloop_state_t *runloop_st = runloop_state_get_ptr();
   core_info_t *info = NULL;
   char directory[PATH_MAX_LENGTH];
   char base[PATH_MAX_LENGTH];
   char parent[PATH_MAX_LENGTH];
   const char *core_name = NULL;
   const char *state_dir = dir_get_ptr(RARCH_DIR_SAVESTATE);
   (void)entry;
   if (!settings || string_is_empty(content_path))
      return;
   if (string_is_equal(content_path, path_get(RARCH_PATH_CONTENT)))
      strlcpy(base, runloop_st->name.savestate, sizeof(base));
   else
   {
      strlcpy(directory, state_dir ? state_dir : "", sizeof(directory));
      if (!*directory || settings->bools.savestates_in_content_dir)
         fill_pathname_basedir(directory, content_path, sizeof(directory));
      if (settings->bools.sort_savestates_by_content_enable)
      {
         fill_pathname_parent_dir_name(parent, content_path, sizeof(parent));
         fill_pathname_join_special(directory, directory, parent, sizeof(directory));
      }
      if (core_info_find(core_path, &info) && info)
         core_name = info->core_name;
      if (settings->bools.sort_savestates_enable && !string_is_empty(core_name))
         fill_pathname_join_special(directory, directory, core_name, sizeof(directory));
      fill_pathname(parent, path_basename(content_path), ".state", sizeof(parent));
      fill_pathname_join_special(base, directory, parent, sizeof(base));
   }
   if (*base && hh_runtime_metadata_state_thumbnail(base, out, size))
      return;
   hh_recent_capture_path(content_path, base, sizeof(base));
   if (*base && path_is_valid(base))
      strlcpy(out, base, size);
}

void hh_runtime_recent_capture_tick(void)
{
#ifdef HAVE_SCREENSHOTS
   static char content[PATH_MAX_LENGTH];
   static retro_time_t started;
   static retro_time_t previous_runtime;
   static bool captured;
   runloop_state_t *runloop_st = runloop_state_get_ptr();
   const char *path = path_get(RARCH_PATH_CONTENT);
   retro_time_t now = cpu_features_get_time_usec();
   char screenshot[PATH_MAX_LENGTH];
   char directory[PATH_MAX_LENGTH];
   if (!(runloop_st->current_core.flags & RETRO_CORE_FLAG_GAME_LOADED)
         || string_is_empty(path))
   {
      content[0] = '\0';
      return;
   }
   if (!string_is_equal(content, path)
         || runloop_st->core_runtime_usec < previous_runtime)
   {
      strlcpy(content, path, sizeof(content));
      started = now;
      captured = false;
   }
   previous_runtime = runloop_st->core_runtime_usec;
   if (captured || now - started < 10000000
         || (runloop_st->flags & (RUNLOOP_FLAG_PAUSED | RUNLOOP_FLAG_IDLE))
         || (menu_state_get_ptr()->flags & MENU_ST_FLAG_ALIVE))
      return;
   captured = true;
   hh_recent_capture_path(content, screenshot, sizeof(screenshot));
   if (!*screenshot)
      return;
   fill_pathname_basedir(directory, screenshot, sizeof(directory));
   if (!path_is_directory(directory) && !path_mkdir(directory))
      return;
   if (take_screenshot(directory, screenshot, true,
            video_driver_cached_frame_is_hw_render(), true, true))
      RARCH_LOG("[GameGo] Recent screenshot: %s\n", screenshot);
#endif
}
