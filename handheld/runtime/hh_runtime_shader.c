#include "hh_runtime_internal.h"

#include <stdlib.h>
#include <file/file_path.h>
#include <streams/file_stream.h>
#include <string/stdstring.h>
#include "menu/menu_shader.h"
#include "defaults.h"

#if defined(HAVE_MENU) && (defined(HAVE_GLSL) || defined(HAVE_SLANG))
typedef struct hh_shader_session
{
   bool active;
   bool original_enabled;
   bool last_enabled;
   bool valid;
   int active_id;
   char original[PATH_MAX_LENGTH];
   char backup[PATH_MAX_LENGTH];
   char last[PATH_MAX_LENGTH];
   char content[PATH_MAX_LENGTH];
   char core[HH_RUNTIME_CORE_NAME_MAX];
   char source[128];
   char original_loaded[PATH_MAX_LENGTH];
   uint8_t original_flags;
} hh_shader_session_t;

static hh_shader_session_t hh_shader_session;

static const char *hh_shader_names[] = {
   "进入前的效果", "关闭着色器", "液晶像素", "柔和液晶",
   "清晰液晶", "经典电视", "轻量扫描线", "浓郁电视"
};
static const char *hh_shader_descriptions[] = {
   "保留进入前的画面设置", "显示原始游戏画面",
   "模拟掌机屏幕的像素纹理", "较浅的纹理，保留画面亮度",
   "更鲜明的掌机像素纹理", "扫描线与彩色电视遮罩",
   "较浅的扫描线，适合小屏幕", "增强扫描线与电视遮罩"
};
static const char *hh_shader_paths[] = {
   "", "gamego/stock/stock", "gamego/handheld/sameboy-lcd",
   "gamego/handheld/lcd-soft", "gamego/handheld/lcd-sharp",
   "gamego/crt/zfast_crt_nogeo", "gamego/crt/crt-soft",
   "gamego/crt/crt-strong"
};

static bool hh_shader_owner(void)
{
   return hh_runtime_state.initialized && hh_runtime_state.owner_thread_id
      && hh_runtime_state.owner_thread_id == sthread_get_current_thread_id();
}

static enum auto_shader_type hh_shader_scope(int scope)
{
   return scope == 1 ? SHADER_PRESET_GAME :
      scope == 2 ? SHADER_PRESET_CORE : SHADER_PRESET_GLOBAL;
}

static enum rarch_shader_type hh_shader_backend(void)
{
   gfx_ctx_flags_t flags;
   flags.flags = 0;
   video_context_driver_get_flags(&flags);
   if (BIT32_GET(flags.flags, GFX_CTX_FLAGS_SHADERS_GLSL))
      return RARCH_SHADER_GLSL;
   if (BIT32_GET(flags.flags, GFX_CTX_FLAGS_SHADERS_SLANG))
      return RARCH_SHADER_SLANG;
   return RARCH_SHADER_NONE;
}

static bool hh_shader_path(int id, char *path, size_t size)
{
   enum rarch_shader_type type = hh_shader_backend();
   settings_t *settings = config_get_ptr();
   size_t length, i;
   const char *dirs[2];
   if (id < 0 || id >= HH_RUNTIME_SHADER_COUNT || type == RARCH_SHADER_NONE)
      return false;
   if (!id)
   {
      strlcpy(path, hh_shader_session.backup[0]
            ? hh_shader_session.backup : hh_shader_session.original, size);
      return true;
   }
   dirs[0] = settings->paths.directory_video_shader;
   dirs[1] = g_defaults.dirs[DEFAULT_DIR_SHADER];
   for (i = 0; i < 2; i++)
   {
      if (!dirs[i] || !*dirs[i])
         continue;
      length = fill_pathname_join(path, dirs[i], hh_shader_paths[id], size);
      strlcpy(path + length, video_shader_get_preset_extension(type), size - length);
      if (path_is_valid(path))
         return true;
   }
   return false;
}

static bool hh_shader_load(const char *path, bool enabled)
{
   settings_t *settings = config_get_ptr();
   enum rarch_shader_type type = path && *path
      ? video_shader_parse_type(path) : hh_shader_backend();
   bool deferred = settings->bools.video_shader_deferred_loading;
   bool result;
   /* A preview result must mean compilation finished. */
   settings->bools.video_shader_deferred_loading = false;
   settings->bools.video_shader_enable = true;
   result = video_shader_apply_shader(settings, type,
         path && *path ? path : NULL, false);
   settings->bools.video_shader_deferred_loading = deferred;
   settings->bools.video_shader_enable = enabled;
   return result;
}

static void hh_shader_source(const char *path, char *source, size_t size)
{
   char suffix[PATH_MAX_LENGTH];
   char parent[NAME_MAX_LENGTH];
   char candidate[PATH_MAX_LENGTH];
   char config_dir[DIR_MAX_LENGTH];
   char old_dir[DIR_MAX_LENGTH];
   const char *dirs[3];
   const char *names[4];
   const char *labels[] = {"当前游戏", "游戏文件夹", "当前模拟器", "全局默认"};
   const char *core = runloop_state_get_ptr()->system.info.library_name;
   const char *content = path_get(RARCH_PATH_BASENAME);
   settings_t *settings = config_get_ptr();
   size_t i, j, length;
   strlcpy(source, "仅本次使用 / 自定义", size);
   if (!path || !*path)
   {
      strlcpy(source, "关闭着色器", size);
      return;
   }
   config_dir[0] = '\0';
   if (!path_is_empty(RARCH_PATH_CONFIG))
      fill_pathname_basedir(config_dir, path_get(RARCH_PATH_CONFIG), sizeof(config_dir));
   fill_pathname_join(old_dir, settings->paths.directory_video_shader,
         "presets", sizeof(old_dir));
   fill_pathname_parent_dir_name(parent, content, sizeof(parent));
   dirs[0] = settings->paths.directory_menu_config;
   dirs[1] = config_dir;
   dirs[2] = old_dir;
   names[0] = path_basename(content);
   names[1] = parent;
   names[2] = core;
   names[3] = "global";
   for (i = 0; i < 3; i++)
   {
      if (!dirs[i] || !*dirs[i])
         continue;
      for (j = 0; j < 4; j++)
      {
         if (!names[j] || !*names[j])
            continue;
         if (j < 3)
            fill_pathname_join(suffix, core, names[j], sizeof(suffix));
         else
            strlcpy(suffix, names[j], sizeof(suffix));
         length = fill_pathname_join(candidate, dirs[i], suffix, sizeof(candidate));
         strlcpy(candidate + length, video_shader_get_preset_extension(
                  video_shader_parse_type(path)), sizeof(candidate) - length);
         if (string_is_equal(candidate, path))
         {
            strlcpy(source, labels[j], size);
            return;
         }
      }
   }
}
typedef struct hh_shader_file_backup
{
   char path[PATH_MAX_LENGTH];
   void *data;
   int64_t size;
} hh_shader_file_backup_t;

static hh_result_t hh_shader_remove(int scope)
{
   hh_shader_file_backup_t *files;
   char config_dir[DIR_MAX_LENGTH], old_dir[DIR_MAX_LENGTH];
   char suffix[PATH_MAX_LENGTH], path[PATH_MAX_LENGTH];
   const char *dirs[3];
   const char *core = runloop_state_get_ptr()->system.info.library_name;
   const char *name = scope == 1 ? path_basename(path_get(RARCH_PATH_BASENAME)) : core;
   enum rarch_shader_type types[] = {RARCH_SHADER_GLSL, RARCH_SHADER_SLANG, RARCH_SHADER_CG};
   settings_t *settings = config_get_ptr();
   gfx_ctx_flags_t flags;
   size_t i, j, count = 0, length;
   bool rollback = false, restored = true;
   hh_result_t result = HH_OK;
   files = (hh_shader_file_backup_t*)calloc(9, sizeof(*files));
   if (!files)
      return HH_ERR_INTERNAL;
   config_dir[0] = '\0';
   if (!path_is_empty(RARCH_PATH_CONFIG))
      fill_pathname_basedir(config_dir, path_get(RARCH_PATH_CONFIG), sizeof(config_dir));
   fill_pathname_join(old_dir, settings->paths.directory_video_shader, "presets", sizeof(old_dir));
   dirs[0] = settings->paths.directory_menu_config;
   dirs[1] = config_dir;
   dirs[2] = old_dir;
   if (scope == 3)
      strlcpy(suffix, "global", sizeof(suffix));
   else
      fill_pathname_join(suffix, core, name, sizeof(suffix));
   flags.flags = 0;
   video_context_driver_get_flags(&flags);
   for (i = 0; i < 3; i++)
   {
      if (!dirs[i] || !*dirs[i])
         continue;
      for (j = 0; j < 3; j++)
      {
         if (!BIT32_GET(flags.flags, video_shader_type_to_flag(types[j])))
            continue;
         length = fill_pathname_join(path, dirs[i], suffix, sizeof(path));
         strlcpy(path + length, video_shader_get_preset_extension(types[j]), sizeof(path) - length);
         if (!path_is_valid(path))
            continue;
         strlcpy(files[count].path, path, sizeof(files[count].path));
         if (filestream_read_file(path, &files[count].data, &files[count].size) < 0)
         {
            free(files[count].data);
            result = HH_ERR_SAVE_FAILED;
            goto end;
         }
         count++;
      }
   }
   if (!menu_shader_manager_remove_auto_preset(hh_shader_scope(scope),
            settings->paths.directory_video_shader, settings->paths.directory_menu_config))
   {
      result = HH_ERR_SAVE_FAILED;
      rollback = true;
      goto end;
   }
   path[0] = '\0';
   video_shader_get_auto_preset_path(path, sizeof(path));
   if (!hh_shader_load(path, true))
   {
      result = HH_ERR_SHADER_LOAD_FAILED;
      rollback = true;
      hh_shader_session.valid = hh_shader_load(hh_shader_session.last,
            hh_shader_session.last_enabled);
      restored = hh_shader_session.valid;
   }
end:
   for (i = 0; i < count; i++)
   {
      if (rollback && !filestream_write_file_atomic(files[i].path, files[i].data, files[i].size))
         restored = false;
      free(files[i].data);
   }
   free(files);
   if (!restored)
      result = HH_ERR_SHADER_RESTORE_FAILED;
   if (result == HH_OK)
      hh_runtime_shader_discard();
   return result;
}

#endif

bool hh_runtime_shader_supported(void)
{
#if defined(HAVE_MENU) && (defined(HAVE_GLSL) || defined(HAVE_SLANG))
   return hh_shader_owner() && hh_shader_backend() != RARCH_SHADER_NONE
      && video_state_get_ptr()->current_video
      && video_state_get_ptr()->current_video->set_shader;
#else
   return false;
#endif
}

void hh_runtime_shader_discard(void)
{
#if defined(HAVE_MENU) && (defined(HAVE_GLSL) || defined(HAVE_SLANG))
   if (hh_shader_session.backup[0])
      filestream_delete(hh_shader_session.backup);
   memset(&hh_shader_session, 0, sizeof(hh_shader_session));
#endif
}

hh_result_t hh_runtime_get_shader_list(bool recommended,
      hh_runtime_shader_list_t *out)
{
#if defined(HAVE_MENU) && (defined(HAVE_GLSL) || defined(HAVE_SLANG))
   char path[PATH_MAX_LENGTH];
   int id, group, pass;
   const char *ext;
   const char *extensions;
   settings_t *settings;
   if (!out)
      return HH_ERR_INVALID_ARGUMENT;
   if (!hh_shader_owner() || !hh_shader_session.active)
      return HH_ERR_INVALID_STATE;
   ext = path_get_extension(path_get(RARCH_PATH_CONTENT));
   extensions = runloop_state_get_ptr()->system.info.valid_extensions;
   settings = config_get_ptr();
   memset(out, 0, sizeof(*out));
   out->active_id = hh_shader_session.active_id;
   out->valid = hh_shader_session.valid;
   strlcpy(out->source, hh_shader_session.source, sizeof(out->source));
   group = ext && (string_is_equal_noncase(ext, "gb")
         || string_is_equal_noncase(ext, "gbc")
         || string_is_equal_noncase(ext, "gba")
         || string_is_equal_noncase(ext, "gg")
         || string_is_equal_noncase(ext, "ngp")
         || string_is_equal_noncase(ext, "ngc")) ? 1 : 2;
   if ((!ext || !*ext || string_is_equal_noncase(ext, "zip")
            || string_is_equal_noncase(ext, "7z")) && extensions
         && (strstr(extensions, "gba") || strstr(extensions, "gbc")
            || strstr(extensions, "ngp") || strstr(extensions, "ws")))
      group = 1;
   for (pass = 0; pass < 2; pass++)
      for (id = 0; id < HH_RUNTIME_SHADER_COUNT; id++)
      {
         bool match = id < 2 || (id < 5 ? group == 1 : group == 2);
         hh_runtime_shader_entry_t *entry;
         if ((pass == 0) != match || (recommended && !match)
               || !hh_shader_path(id, path, sizeof(path)))
            continue;
         entry = &out->entries[out->count++];
         entry->id = id;
         strlcpy(entry->name, hh_shader_names[id], sizeof(entry->name));
         strlcpy(entry->description, hh_shader_descriptions[id], sizeof(entry->description));
         strlcpy(entry->detail, !id ? "进入前的预设与参数" : id == 1
               ? "无视觉效果" : id < 5 ? "SameBoy LCD · 掌机 · 轻量"
               : "zfast CRT · 家用机 / 街机 · 轻量", sizeof(entry->detail));
      }
   for (id = 1; id <= 3; id++)
      out->removable[id - 1] = menu_shader_manager_auto_preset_exists(
            hh_shader_scope(id), settings->paths.directory_video_shader,
            settings->paths.directory_menu_config);
   return HH_OK;
#else
   (void)recommended;
   (void)out;
   return HH_ERR_UNSUPPORTED;
#endif
}

hh_result_t hh_runtime_shader_command(hh_command_type_t type, int argument)
{
#if defined(HAVE_MENU) && (defined(HAVE_GLSL) || defined(HAVE_SLANG))
   settings_t *settings = config_get_ptr();
   runloop_state_t *runloop = runloop_state_get_ptr();
   video_driver_state_t *video = video_state_get_ptr();
   struct video_shader *shader = menu_shader_get();
   char path[PATH_MAX_LENGTH];
   const char *current;
   bool enabled;
   if (!hh_runtime_shader_supported())
      return HH_ERR_UNSUPPORTED;
   if (video->shader_deferred.state == SHADER_LOAD_COMPILING)
      return HH_ERR_BUSY;
   if (type == HH_CMD_SHADER_BEGIN)
   {
      if (hh_shader_session.active)
         return HH_ERR_BUSY;
      if (!(runloop->flags & RUNLOOP_FLAG_PAUSED))
         return HH_ERR_INVALID_STATE;
      memset(&hh_shader_session, 0, sizeof(hh_shader_session));
      current = runloop->runtime_shader_preset_path;
      if (!*current)
         current = video_shader_get_current_shader_preset();
      strlcpy(hh_shader_session.original, current ? current : "",
            sizeof(hh_shader_session.original));
      hh_shader_session.original_enabled = settings->bools.video_shader_enable;
      hh_shader_session.last_enabled = settings->bools.video_shader_enable;
      if (shader)
      {
         strlcpy(hh_shader_session.original_loaded, shader->loaded_preset_path,
               sizeof(hh_shader_session.original_loaded));
         hh_shader_session.original_flags = shader->flags;
      }
      if (shader && shader->passes)
      {
         if (settings->paths.directory_menu_config[0])
            strlcpy(path, settings->paths.directory_menu_config, sizeof(path));
         else if (!path_is_empty(RARCH_PATH_CONFIG))
            fill_pathname_basedir(path, path_get(RARCH_PATH_CONFIG), sizeof(path));
         else
            return HH_ERR_SAVE_FAILED;
         fill_pathname_join(hh_shader_session.backup, path,
               "gamego-preview-original", sizeof(hh_shader_session.backup));
         strlcat(hh_shader_session.backup, video_shader_get_preset_extension(
                  video_shader_parse_type(shader->path)), sizeof(hh_shader_session.backup));
         if (!path_mkdir(path) || !video_shader_write_preset(
                  hh_shader_session.backup, shader, false))
         {
            hh_runtime_shader_discard();
            return HH_ERR_SAVE_FAILED;
         }
      }
      strlcpy(hh_shader_session.last, hh_shader_session.backup[0]
            ? hh_shader_session.backup : hh_shader_session.original,
            sizeof(hh_shader_session.last));
      strlcpy(hh_shader_session.content, path_get(RARCH_PATH_BASENAME), sizeof(hh_shader_session.content));
      strlcpy(hh_shader_session.core, runloop->system.info.library_name, sizeof(hh_shader_session.core));
      hh_shader_source(hh_shader_session.original, hh_shader_session.source, sizeof(hh_shader_session.source));
      if (!hh_shader_session.original_enabled)
         strlcpy(hh_shader_session.source, "关闭着色器", sizeof(hh_shader_session.source));
      hh_shader_session.active = true;
      hh_shader_session.valid = true;
      return HH_OK;
   }
   if (!hh_shader_session.active)
      return HH_ERR_INVALID_STATE;
   if (!string_is_equal(hh_shader_session.content, path_get(RARCH_PATH_BASENAME))
         || !string_is_equal(hh_shader_session.core, runloop->system.info.library_name))
   {
      hh_runtime_shader_discard();
      return HH_ERR_NO_CONTENT;
   }
   if (type == HH_CMD_SHADER_PREVIEW)
   {
      if (!hh_shader_path(argument, path, sizeof(path)))
         return HH_ERR_UNSUPPORTED;
      enabled = argument ? true : hh_shader_session.original_enabled;
      if (!hh_shader_load(path, enabled))
      {
         hh_shader_session.valid = hh_shader_load(hh_shader_session.last,
               hh_shader_session.last_enabled);
         return hh_shader_session.valid ? HH_ERR_SHADER_LOAD_FAILED
            : HH_ERR_SHADER_RESTORE_FAILED;
      }
      hh_shader_session.valid = true;
      hh_shader_session.active_id = argument;
      hh_shader_session.last_enabled = enabled;
      strlcpy(hh_shader_session.last, path, sizeof(hh_shader_session.last));
      return HH_OK;
   }
   if (type == HH_CMD_SHADER_CANCEL)
   {
      if (!hh_shader_load(hh_shader_session.backup[0]
               ? hh_shader_session.backup : hh_shader_session.original,
               hh_shader_session.original_enabled))
      {
         hh_shader_session.valid = false;
         return HH_ERR_SHADER_RESTORE_FAILED;
      }
      strlcpy(runloop->runtime_shader_preset_path, hh_shader_session.original,
            sizeof(runloop->runtime_shader_preset_path));
      if (shader)
      {
         strlcpy(shader->path, hh_shader_session.original, sizeof(shader->path));
         strlcpy(shader->loaded_preset_path, hh_shader_session.original_loaded,
               sizeof(shader->loaded_preset_path));
         shader->flags = hh_shader_session.original_flags;
      }
      hh_runtime_shader_discard();
      return HH_OK;
   }
   if (type == HH_CMD_SHADER_APPLY)
   {
      if (argument < 0 || argument > 3 || !hh_shader_session.valid)
         return HH_ERR_INVALID_STATE;
      if (argument)
      {
         bool deferred;
         bool saved;
         /* Persist off without changing other scopes. */
         if (!hh_shader_session.last_enabled || !shader || !shader->passes)
         {
            if (!hh_shader_path(1, path, sizeof(path)) || !hh_shader_load(path, true))
               return HH_ERR_SHADER_LOAD_FAILED;
         }
         if (hh_shader_session.active_id == 0 && hh_shader_session.backup[0]
               && hh_shader_session.last_enabled)
         {
            strlcpy(shader->path, hh_shader_session.original, sizeof(shader->path));
            strlcpy(shader->loaded_preset_path, hh_shader_session.original_loaded,
                  sizeof(shader->loaded_preset_path));
         }
         deferred = settings->bools.video_shader_deferred_loading;
         settings->bools.video_shader_deferred_loading = false;
         saved = menu_shader_manager_save_auto_preset(shader, hh_shader_scope(argument),
               settings->paths.directory_video_shader,
               settings->paths.directory_menu_config, true);
         settings->bools.video_shader_deferred_loading = deferred;
         if (!saved)
            return HH_ERR_SAVE_FAILED;
         configuration_set_bool(settings, settings->bools.auto_shaders_enable, true);
      }
      if (!hh_shader_session.active_id && !argument)
      {
         strlcpy(runloop->runtime_shader_preset_path, hh_shader_session.original,
               sizeof(runloop->runtime_shader_preset_path));
         if (shader)
         {
            strlcpy(shader->path, hh_shader_session.original, sizeof(shader->path));
            strlcpy(shader->loaded_preset_path, hh_shader_session.original_loaded,
                  sizeof(shader->loaded_preset_path));
            shader->flags = hh_shader_session.original_flags;
         }
      }
      hh_runtime_shader_discard();
      return HH_OK;
   }
   if (type == HH_CMD_SHADER_REMOVE)
   {
      if (argument < 1 || argument > 3)
         return HH_ERR_INVALID_ARGUMENT;
      return hh_shader_remove(argument);
   }
   return HH_ERR_INVALID_ARGUMENT;
#else
   (void)type;
   (void)argument;
   return HH_ERR_UNSUPPORTED;
#endif
}
