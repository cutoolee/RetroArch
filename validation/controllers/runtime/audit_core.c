#include <stdint.h>
#include <string.h>
#include <android/log.h>
#include <libretro.h>

static retro_environment_t environment;
static retro_video_refresh_t video;
static retro_input_poll_t poll_input;
static retro_input_state_t input;
static uint32_t pixels[320 * 240];
static unsigned frame, previous[2];
void retro_set_environment(retro_environment_t cb) { environment = cb; }
void retro_set_video_refresh(retro_video_refresh_t cb) { video = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { (void)cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { (void)cb; }
void retro_set_input_poll(retro_input_poll_t cb) { poll_input = cb; }
void retro_set_input_state(retro_input_state_t cb) { input = cb; }
void retro_init(void) { frame = 0; memset(previous, 0, sizeof(previous)); }
void retro_deinit(void) {}
unsigned retro_api_version(void) { return RETRO_API_VERSION; }
void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name = "GameGo Controls Audit";
   info->library_version = "1";
   info->valid_extensions = "bin";
}
void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));
   info->geometry.base_width = info->geometry.max_width = 320;
   info->geometry.base_height = info->geometry.max_height = 240;
   info->geometry.aspect_ratio = 4.0f / 3.0f;
   info->timing.fps = 60;
   info->timing.sample_rate = 44100;
}
bool retro_load_game(const struct retro_game_info *game)
{
   enum retro_pixel_format format = RETRO_PIXEL_FORMAT_XRGB8888;
   static const struct retro_input_descriptor desc[] = {
      {0, RETRO_DEVICE_JOYPAD, 0, 0, "A"},
      {0, RETRO_DEVICE_JOYPAD, 0, 1, "B"},
      {0, RETRO_DEVICE_JOYPAD, 0, 8, "C"},
      {1, RETRO_DEVICE_JOYPAD, 0, 0, "A"},
      {1, RETRO_DEVICE_JOYPAD, 0, 1, "B"},
      {1, RETRO_DEVICE_JOYPAD, 0, 8, "C"},
      {0, 0, 0, 0, NULL}
   };
   (void)game;
   environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format);
   environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void*)desc);
   return true;
}
void retro_run(void)
{
   unsigned port, id;
   poll_input();
   for (port = 0; port < 2; port++)
   {
      unsigned scalar = 0;
      unsigned mask = (uint16_t)input(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
      for (id = 0; id < 16; id++)
         if (input(port, RETRO_DEVICE_JOYPAD, 0, id))
            scalar |= 1U << id;
      if (mask != previous[port] || scalar != mask)
         __android_log_print(ANDROID_LOG_INFO, "GG_INPUT", "frame=%u port=%u mask=%u scalar=%u", frame, port, mask, scalar);
      previous[port] = mask;
   }
   frame++;
   video(pixels, 320, 240, 320 * 4);
}
void retro_reset(void) {}
void retro_set_controller_port_device(unsigned port, unsigned device) { (void)port; (void)device; }
void retro_unload_game(void) {}
size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void *data, size_t size) { (void)data; (void)size; return false; }
bool retro_unserialize(const void *data, size_t size) { (void)data; (void)size; return false; }
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char *code) { (void)index; (void)enabled; (void)code; }
bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t count) { (void)type; (void)info; (void)count; return false; }
unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }
void *retro_get_memory_data(unsigned id) { (void)id; return NULL; }
size_t retro_get_memory_size(unsigned id) { (void)id; return 0; }
