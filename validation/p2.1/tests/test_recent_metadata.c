#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "handheld/runtime/hh_runtime_metadata.h"

int main(int argc, char **argv)
{
   char content[4096];
   char title[256];
   char thumbnail[4096];
   char video[4096];
   char state_base[4096];
   assert(argc == 2);
   snprintf(content, sizeof(content), "%s/kof94br.zip#rom.bin", argv[1]);
   assert(hh_runtime_metadata_lookup(content, title, sizeof(title),
            thumbnail, sizeof(thumbnail)));
   assert(strcmp(title, "拳皇94") == 0);
   assert(strstr(thumbnail, "/media/拳皇94/boxfront.png") != NULL);
   snprintf(content, sizeof(content), "%s/subdir/dino.zip", argv[1]);
   assert(hh_runtime_metadata_lookup(content, title, sizeof(title),
            thumbnail, sizeof(thumbnail)));
   assert(strcmp(title, "恐龙快打") == 0);
   assert(strstr(thumbnail, "/custom.png") != NULL);
   snprintf(content, sizeof(content), "%s/mslug.zip", argv[1]);
   assert(hh_runtime_metadata_lookup(content, title, sizeof(title), NULL, 0));
   assert(strcmp(title, "合金弹头") == 0);
   assert(hh_runtime_metadata_lookup(content, title, sizeof(title),
            thumbnail, sizeof(thumbnail)));
   assert(strstr(thumbnail, "/media/mslug/boxFront.jpg") != NULL);
   assert(hh_runtime_metadata_lookup_media(content, title, sizeof(title),
            thumbnail, sizeof(thumbnail), video, sizeof(video)));
   assert(strstr(video, "/media/mslug/video.mp4") != NULL);
   snprintf(content, sizeof(content), "%s/msluga.zip", argv[1]);
   assert(hh_runtime_metadata_lookup(content, title, sizeof(title),
            thumbnail, sizeof(thumbnail)));
   assert(strcmp(title, "合金弹头") == 0);
   assert(strstr(thumbnail, "/media/mslug/boxFront.jpg") != NULL);
   snprintf(content, sizeof(content), "%s/unknown.zip", argv[1]);
   strcpy(title, "fallback");
   assert(!hh_runtime_metadata_lookup(content, title, sizeof(title),
            thumbnail, sizeof(thumbnail)));
   assert(strcmp(title, "fallback") == 0 && thumbnail[0] == '\0');
   assert(!hh_runtime_metadata_lookup_media(content, title, sizeof(title),
            thumbnail, sizeof(thumbnail), video, sizeof(video)));
   assert(strcmp(title, "fallback") == 0 && video[0] == '\0');
   snprintf(content, sizeof(content), "%s/orphan.zip", argv[1]);
   assert(!hh_runtime_metadata_lookup_media(content, title, sizeof(title),
            thumbnail, sizeof(thumbnail), video, sizeof(video)));
   assert(strcmp(title, "fallback") == 0);
   assert(strstr(video, "/media/orphan/video.mp4") != NULL);
   snprintf(state_base, sizeof(state_base), "%s/states/game.state", argv[1]);
   assert(hh_runtime_metadata_state_thumbnail(state_base, thumbnail, sizeof(thumbnail)));
   assert(strstr(thumbnail, "/game.state12.png") != NULL);
   snprintf(state_base, sizeof(state_base), "%s/states/missing.state", argv[1]);
   assert(!hh_runtime_metadata_state_thumbnail(state_base, thumbnail, sizeof(thumbnail)));
   assert(!hh_runtime_metadata_lookup(NULL, title, sizeof(title), NULL, 0));
   puts("PASS: Pegasus titles, file/files, CRLF/BOM, archive paths, parent metadata, artwork and fallback");
   return 0;
}
