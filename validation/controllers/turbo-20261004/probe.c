#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "libretro.h"
static const char *mode;
static unsigned button;
static bool changed;
static bool env(unsigned cmd, void *data)
{
   struct retro_variable *v;
   if (cmd == RETRO_ENVIRONMENT_GET_VARIABLE) {
      v = (struct retro_variable *)data;
      if (!strcmp(v->key, "fceumm_turbo_enable")) v->value = mode;
      else if (!strcmp(v->key, "fceumm_turbo_delay")) v->value = "3";
      else { v->value = NULL; return false; }
      return true;
   }
   if (cmd == RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE) {
      *(bool *)data = changed; changed = false; return true;
   }
   if (cmd == RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION) {
      *(unsigned *)data = 2; return true;
   }
   if (cmd == RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY || cmd == RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY) {
      *(const char **)data = "/data/local/tmp"; return true;
   }
   if (cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT || cmd == RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS || cmd == RETRO_ENVIRONMENT_SET_CONTROLLER_INFO || cmd == RETRO_ENVIRONMENT_SET_VARIABLES || cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS || cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2 || cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL || cmd == RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME) return true;
   return false;
}
static void video(const void *d, unsigned w, unsigned h, size_t p) { (void)d; (void)w; (void)h; (void)p; }
static void audio(int16_t l, int16_t r) { (void)l; (void)r; }
static size_t batch(const int16_t *d, size_t n) { (void)d; return n; }
static void poll(void) {}
static int16_t input(unsigned p, unsigned d, unsigned i, unsigned id) { (void)i; return p == 0 && d == RETRO_DEVICE_JOYPAD && id == button; }
#define LOAD(name) name = dlsym(core, #name); if (!name) return 3
int main(int argc, char **argv)
{
   void *core;
   void (*retro_set_environment)(retro_environment_t);
   void (*retro_set_video_refresh)(retro_video_refresh_t);
   void (*retro_set_audio_sample)(retro_audio_sample_t);
   void (*retro_set_audio_sample_batch)(retro_audio_sample_batch_t);
   void (*retro_set_input_poll)(retro_input_poll_t);
   void (*retro_set_input_state)(retro_input_state_t);
   void (*retro_init)(void), (*retro_deinit)(void), (*retro_run)(void), (*retro_unload_game)(void);
   bool (*retro_load_game)(const struct retro_game_info *);
   void *(*retro_get_memory_data)(unsigned);
   struct retro_game_info game;
   unsigned char rom[16 + 16384 + 8192];
   unsigned char code[] = {0x78,0xd8,0xa2,0xff,0x9a,0xa9,0,0x8d,0,0x20,0x8d,1,0x20,0xa9,1,0x8d,0x16,0x40,0xa9,0,0x8d,0x16,0x40,0x85,0,0xa2,8,0xad,0x16,0x40,0x4a,0x26,0,0xca,0xd0,0xf7,0xa5,0,0x85,1,0x4c,0x0d,0x80};
   unsigned ids[] = {8,0,9,1,9,1};
   const char *names[] = {"normal_A", "normal_B", "turbo_A_disabled", "turbo_B_disabled", "turbo_A_enabled", "turbo_B_enabled"};
   unsigned t, f, on, off, other;
   unsigned char *ram;
   if (argc != 2) return 2;
   core = dlopen(argv[1], RTLD_NOW); if (!core) { puts(dlerror()); return 3; }
   LOAD(retro_set_environment); LOAD(retro_set_video_refresh); LOAD(retro_set_audio_sample); LOAD(retro_set_audio_sample_batch); LOAD(retro_set_input_poll); LOAD(retro_set_input_state); LOAD(retro_init); LOAD(retro_deinit); LOAD(retro_run); LOAD(retro_unload_game); LOAD(retro_load_game); LOAD(retro_get_memory_data);
   memset(rom, 0, sizeof(rom)); memcpy(rom, "NES\032", 4); rom[4] = 1; rom[5] = 1;
   memcpy(rom+16, code, sizeof(code));
   for (t = 0; t < 3; t++) { rom[16+0x3ffa+t*2] = 0; rom[16+0x3ffb+t*2] = 0x80; }
   memset(&game, 0, sizeof(game)); game.path = "/data/user/0/com.retroarch.aarch64/cache/turbo-probe.nes"; game.data = rom; game.size = sizeof(rom);
   { FILE *fp = fopen(game.path, "wb"); if (!fp) return 6; fwrite(rom, 1, sizeof(rom), fp); fclose(fp); }
   retro_set_environment(env); retro_set_video_refresh(video); retro_set_audio_sample(audio); retro_set_audio_sample_batch(batch); retro_set_input_poll(poll); retro_set_input_state(input);
   for (t = 0; t < 6; t++) {
      mode = t < 4 ? "None" : "Player 1"; button = ids[t]; changed = true;
      retro_init(); if (!retro_load_game(&game)) { puts("LOAD_FAILED"); return 4; }
      on = off = other = 0;
      for (f = 0; f < 64; f++) {
         retro_run(); ram = retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
         if (!ram) return 5;
         if (f < 4) continue;
         if (ram[1] == (t % 2 == 0 ? 128 : 64)) on++;
         else if (ram[1] == 0) off++;
         else other++;
      }
      printf("%s mode=%s on=%u off=%u other=%u\n", names[t], mode, on, off, other);
      retro_unload_game(); retro_deinit();
   }
   return 0;
}
