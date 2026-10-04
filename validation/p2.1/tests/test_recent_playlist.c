#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "handheld/runtime/hh_runtime_internal.h"
#include "defaults.h"

struct defaults g_defaults;
static struct playlist_entry entries[6];
static size_t entry_count;

size_t playlist_size(playlist_t *playlist)
{
   (void)playlist;
   return entry_count;
}

void playlist_get_index(playlist_t *playlist, size_t index,
      const struct playlist_entry **entry)
{
   (void)playlist;
   *entry = index < entry_count ? &entries[index] : NULL;
}

void playlist_resolve_path(enum playlist_file_mode mode, bool core,
      char *path, size_t size)
{
   (void)mode;
   (void)core;
   if (strcmp(path, "relative/game.zip") == 0)
      strlcpy(path, "/roms/game.zip", size);
}

int main(void)
{
   g_defaults.content_history = (playlist_t*)entries;
   entries[0].path = "/roms/game.zip";
   entries[0].core_path = "/new/core.so";
   entries[1].path = "/roms/other.zip";
   entries[2].path = "/roms/game.zip";
   entries[2].core_path = "/old/core.so";
   entries[3].path = "relative/game.zip";
   entries[4].path = "/different/game.zip";
   entries[5].path = "/roms/other.zip";
   entry_count = 6;
   assert(hh_runtime_recent_entry_index(0) == 0);
   assert(hh_runtime_recent_entry_index(1) == 1);
   assert(hh_runtime_recent_entry_index(2) == 4);
   assert(hh_runtime_recent_entry_index(3) == (size_t)-1);
   entry_count = 0;
   assert(hh_runtime_recent_entry_index(0) == (size_t)-1);
   g_defaults.content_history = NULL;
   assert(hh_runtime_recent_entry_index(0) == (size_t)-1);
   puts("PASS: recent entries keep newest path across cores, resolve paths and preserve distinct directories");
   return 0;
}
