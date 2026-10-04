#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "handheld/runtime/hh_runtime_internal.h"

static settings_t settings;
static runloop_state_t loop;
static struct menu_state menu;
static retro_time_t now;
static char content[128] = "/roms/game.zip";
static unsigned captures;

settings_t *config_get_ptr(void) { return &settings; }
runloop_state_t *runloop_state_get_ptr(void) { return &loop; }
struct menu_state *menu_state_get_ptr(void) { return &menu; }
retro_time_t cpu_features_get_time_usec(void) { return now; }
const char *path_get(enum rarch_path_type type)
{ (void)type; return content; }
bool path_is_directory(const char *path) { (void)path; return true; }
bool path_mkdir(const char *path) { (void)path; return true; }
bool video_driver_cached_frame_is_hw_render(void) { return false; }
void RARCH_LOG(const char *format, ...) { (void)format; }

bool take_screenshot(const char *directory, const char *path,
      bool silent, bool hardware, bool fullpath, bool threaded)
{
   assert(strstr(directory, "/screenshots/gamego-recent/") != NULL);
   assert(strstr(path, "/screenshots/gamego-recent/") != NULL);
   assert(silent && !hardware && fullpath && threaded);
   captures++;
   return true;
}

int main(void)
{
   strcpy(settings.paths.directory_screenshot, "/screenshots");
   loop.current_core.flags |= RETRO_CORE_FLAG_GAME_LOADED;
   now = 1000000;
   hh_runtime_recent_capture_tick();
   now = 10999999;
   loop.core_runtime_usec = 9999999;
   hh_runtime_recent_capture_tick();
   assert(captures == 0);
   now = 11000000;
   loop.core_runtime_usec = 10000000;
   hh_runtime_recent_capture_tick();
   assert(captures == 1);
   now = 40000000;
   loop.core_runtime_usec = 39000000;
   hh_runtime_recent_capture_tick();
   assert(captures == 1);
   loop.core_runtime_usec = 0;
   hh_runtime_recent_capture_tick();
   now = 49999999;
   hh_runtime_recent_capture_tick();
   assert(captures == 1);
   now = 50000000;
   loop.flags |= RUNLOOP_FLAG_PAUSED;
   hh_runtime_recent_capture_tick();
   assert(captures == 1);
   loop.flags &= ~RUNLOOP_FLAG_PAUSED;
   hh_runtime_recent_capture_tick();
   assert(captures == 2);
   strcpy(content, "/roms/other.zip");
   hh_runtime_recent_capture_tick();
   now = 60000000;
   hh_runtime_recent_capture_tick();
   assert(captures == 3);
   now = 80000000;
   hh_runtime_recent_capture_tick();
   assert(captures == 3);
   puts("PASS: capture at 10 seconds once per launch, including same-game reload and pause deferral");
   return 0;
}
