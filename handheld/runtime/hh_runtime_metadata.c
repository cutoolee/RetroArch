#include "hh_runtime_metadata.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <retro_miscellaneous.h>
#include <compat/strl.h>
#include <file/file_path.h>
#include <streams/file_stream.h>
#include <string/stdstring.h>
#include <lists/dir_list.h>

static bool hh_metadata_file_matches(const char *metadata_path,
      const char *file, const char *content_path)
{
   char path[PATH_MAX_LENGTH];
   if (!*file)
      return false;
   fill_pathname_resolve_relative(path, metadata_path, file, sizeof(path));
   return string_is_equal(path, content_path);
}

static bool hh_metadata_read(const char *metadata_path,
      const char *content_path, char *title, size_t title_size,
      char *thumbnail, size_t thumbnail_size,
      char *media_name, size_t media_name_size,
      char *video, size_t video_size)
{
   RFILE *file;
   char *line;
   char name[256];
   char image[PATH_MAX_LENGTH];
   char clip[PATH_MAX_LENGTH];
   bool matched = false;
   bool files = false;
   bool first_line = true;

   file = filestream_open(metadata_path, RETRO_VFS_FILE_ACCESS_READ,
         RETRO_VFS_FILE_ACCESS_HINT_NONE);
   if (!file)
      return false;
   name[0] = image[0] = clip[0] = media_name[0] = '\0';
   while ((line = filestream_getline(file)) != NULL)
   {
      char *key = line;
      char *value;
      bool continuation = *line == ' ' || *line == '\t';
      if (first_line && strlen(line) >= 3
            && memcmp(line, "\xef\xbb\xbf", 3) == 0)
         key += 3;
      first_line = false;
      key = string_trim_whitespace(key);
      if (!*key || *key == '#')
      {
         free(line);
         continue;
      }
      if (files && continuation)
      {
         if (!*media_name)
            fill_pathname(media_name, path_basename(key), "", media_name_size);
         if (hh_metadata_file_matches(metadata_path, key, content_path))
            matched = true;
         free(line);
         continue;
      }
      files = false;
      value = strchr(key, ':');
      if (!value)
      {
         free(line);
         continue;
      }
      *value++ = '\0';
      key = string_trim_whitespace(key);
      value = string_trim_whitespace(value);
      if (string_is_equal(key, "game") || string_is_equal(key, "collection"))
      {
         if (matched)
         {
            free(line);
            break;
         }
         strlcpy(name, string_is_equal(key, "game") ? value : "", sizeof(name));
         image[0] = clip[0] = media_name[0] = '\0';
      }
      else if (*name && (string_is_equal(key, "file") || string_is_equal(key, "files")))
      {
         if (*value && !*media_name)
            fill_pathname(media_name, path_basename(value), "", media_name_size);
         if (hh_metadata_file_matches(metadata_path, value, content_path))
            matched = true;
         files = true;
      }
      else if (*name && (string_is_equal(key, "assets.box_front")
               || (!*image && string_is_equal(key, "assets.screenshot"))))
         fill_pathname_resolve_relative(image, metadata_path, value, sizeof(image));
      else if (*name && string_is_equal(key, "assets.video"))
         fill_pathname_resolve_relative(clip, metadata_path, value, sizeof(clip));
      free(line);
   }
   filestream_close(file);
   if (!matched || !*name)
      return false;
   strlcpy(title, name, title_size);
   if (thumbnail && thumbnail_size && *image && path_is_valid(image))
      strlcpy(thumbnail, image, thumbnail_size);
   if (video && video_size && *clip && path_is_valid(clip))
      strlcpy(video, clip, video_size);
   return true;
}

static void hh_metadata_find_media(const char *directory, const char *name,
      const char *const *files, size_t count, char *out, size_t size)
{
   char media[PATH_MAX_LENGTH];
   char folder[PATH_MAX_LENGTH];
   size_t i;
   if (!out || !size || *out || string_is_empty(name))
      return;
   fill_pathname_join_special(media, directory, "media", sizeof(media));
   fill_pathname_join_special(folder, media, name, sizeof(folder));
   for (i = 0; i < count; i++)
   {
      fill_pathname_join_special(media, folder, files[i], sizeof(media));
      if (path_is_valid(media))
      {
         strlcpy(out, media, size);
         return;
      }
   }
}

bool hh_runtime_metadata_lookup_media(const char *content_path,
      char *title, size_t title_size, char *thumbnail, size_t thumbnail_size,
      char *video, size_t video_size)
{
   static const char *metadata_names[] = {"metadata.pegasus.txt", "metadata.txt"};
   static const char *image_names[] = {"boxfront.png", "boxFront.png",
      "boxFront.jpg", "boxfront.jpg", "boxFront.jpeg", "boxfront.jpeg",
      "screenshot.png", "screenshot.jpg", "logo.png"};
   static const char *video_names[] = {"video.mp4", "video.webm", "video.mkv"};
   char content[PATH_MAX_LENGTH];
   char directory[PATH_MAX_LENGTH];
   char metadata[PATH_MAX_LENGTH];
   char media_name[PATH_MAX_LENGTH];
   char content_name[PATH_MAX_LENGTH];
   const char *media_names[3];
   const char *archive_delim;
   size_t i;
   size_t j;
   bool matched;

   if (!content_path || !*content_path || !title || !title_size)
      return false;
   strlcpy(content, content_path, sizeof(content));
   archive_delim = path_get_archive_delim(content);
   if (archive_delim)
      content[archive_delim - content] = '\0';
   fill_pathname(content_name, path_basename(content), "", sizeof(content_name));
   strlcpy(directory, content, sizeof(directory));
   path_basedir(directory);
   if (thumbnail && thumbnail_size)
      thumbnail[0] = '\0';
   if (video && video_size)
      video[0] = '\0';
   while (*directory)
   {
      matched = false;
      media_name[0] = '\0';
      for (i = 0; i < sizeof(metadata_names) / sizeof(metadata_names[0]); i++)
      {
         fill_pathname_join_special(metadata, directory, metadata_names[i], sizeof(metadata));
         if (hh_metadata_read(metadata, content, title, title_size,
                  thumbnail, thumbnail_size, media_name, sizeof(media_name),
                  video, video_size))
         {
            matched = true;
            break;
         }
      }
      media_names[0] = matched ? title : "";
      media_names[1] = content_name;
      media_names[2] = matched ? media_name : "";
      for (j = 0; j < sizeof(media_names) / sizeof(media_names[0]); j++)
      {
         hh_metadata_find_media(directory, media_names[j], image_names,
               sizeof(image_names) / sizeof(image_names[0]), thumbnail, thumbnail_size);
         hh_metadata_find_media(directory, media_names[j], video_names,
               sizeof(video_names) / sizeof(video_names[0]), video, video_size);
      }
      if (matched)
         return true;
      i = strlen(directory);
      path_parent_dir(directory, i);
      if (strlen(directory) >= i)
         break;
   }
   return false;
}

bool hh_runtime_metadata_lookup(const char *content_path,
      char *title, size_t title_size, char *thumbnail, size_t thumbnail_size)
{
   return hh_runtime_metadata_lookup_media(content_path, title, title_size,
         thumbnail, thumbnail_size, NULL, 0);
}

bool hh_runtime_metadata_state_thumbnail(const char *base,
      char *out, size_t size)
{
   struct string_list *list;
   struct stat state;
   char directory[PATH_MAX_LENGTH];
   char path[PATH_MAX_LENGTH];
   time_t newest = 0;
   size_t i, length = strlen(base);
   bool found = false;
   fill_pathname_basedir(directory, base, sizeof(directory));
   list = dir_list_new(directory, "png", false, false, false, false);
   if (!list)
      return false;
   for (i = 0; i < list->size; i++)
   {
      const char *suffix;
      fill_pathname(path, list->elems[i].data, "", sizeof(path));
      if (strncmp(path, base, length) != 0)
         continue;
      suffix = path + length;
      if (!string_is_equal(suffix, ".auto"))
      {
         while (*suffix >= '0' && *suffix <= '9')
            suffix++;
         if (*suffix)
            continue;
      }
      if (stat(path, &state) != 0 || (found && state.st_mtime <= newest))
         continue;
      newest = state.st_mtime;
      found = true;
      strlcpy(out, list->elems[i].data, size);
   }
   string_list_free(list);
   return found;
}
