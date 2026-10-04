#ifndef HH_RUNTIME_METADATA_H
#define HH_RUNTIME_METADATA_H

#include <stdbool.h>
#include <stddef.h>

bool hh_runtime_metadata_lookup(const char *content_path,
      char *title, size_t title_size, char *thumbnail, size_t thumbnail_size);
bool hh_runtime_metadata_lookup_media(const char *content_path,
      char *title, size_t title_size, char *thumbnail, size_t thumbnail_size,
      char *video, size_t video_size);
bool hh_runtime_metadata_state_thumbnail(const char *base,
      char *out, size_t size);

#endif
