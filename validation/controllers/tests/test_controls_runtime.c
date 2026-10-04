#include <assert.h>
#include <stdio.h>
#include "handheld/runtime/hh_runtime_internal.h"
#include "input/input_driver.h"
#include "retroarch.h"

static settings_t settings;
static runloop_state_t runloop_st;
static input_driver_state_t input_st;
hh_runtime_state_t hh_runtime_state;
settings_t *config_get_ptr(void) { return &settings; }
runloop_state_t *runloop_state_get_ptr(void) { return &runloop_st; }
input_driver_state_t *input_state_get_ptr(void) { return &input_st; }
bool runloop_is_inited(void) { return true; }
uintptr_t sthread_get_current_thread_id(void) { return 1; }
const char *input_config_get_device_display_name(unsigned port)
{ return port < 2 ? "Test pad" : ""; }
const char *input_config_get_device_name(unsigned port)
{ return input_config_get_device_display_name(port); }
const char *path_get(enum rarch_path_type type) { (void)type; return "game"; }
bool path_is_empty(enum rarch_path_type type) { (void)type; return true; }
bool config_save_file(const char *path) { (void)path; return true; }
bool input_remapping_save_file(const char *path) { (void)path; return true; }
bool retroarch_ctl(enum rarch_ctl_state state, void *data)
{ (void)state; (void)data; return true; }

int main(void)
{
   hh_runtime_controls_t controls;
   unsigned i;
   hh_runtime_state.initialized = true;
   hh_runtime_state.owner_thread_id = 1;
   settings.uints.input_max_users = 4;
   for (i = 0; i < MAX_USERS; i++)
   {
      settings.uints.input_remap_ports[i] = i;
      settings.uints.input_joypad_index[i] = i;
      settings.uints.input_libretro_device[i] = RETRO_DEVICE_JOYPAD;
   }
   runloop_st.system.input_desc_btn[0][0] = "A";
   runloop_st.system.input_desc_btn[0][1] = "B";
   runloop_st.system.input_desc_btn[0][2] = "C";
   runloop_st.system.input_desc_btn[0][3] = "Turbo A";
   assert(hh_runtime_get_controls(0, &controls) == HH_OK);
   assert(controls.available[0] && !controls.available[3]);
   assert(controls.device_count == 2);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET, 7) == HH_OK);
   assert(settings.uints.input_action_mask[0][0] == 7);
   assert(settings.uints.input_action_mask[1][0] == 0);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET,
            (6 << 24) | (1 << 20) | 1) == HH_OK);
   assert(settings.uints.input_action_period[1][0] == 6);
   assert(settings.uints.input_action_period[0][0] == 0);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET, 8) == HH_ERR_INVALID_ARGUMENT);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET, (1 << 24) | 1) == HH_ERR_INVALID_ARGUMENT);
   input_st.input_action_phase[0][0] = 3;
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_DEVICE, 1) == HH_OK);
   assert(settings.uints.input_joypad_index[0] == 1);
   assert(settings.uints.input_joypad_index[1] == 0);
   assert(settings.uints.input_joypad_index[2] == 2);
   assert(input_st.input_action_phase[0][0] == 0);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_DEVICE, 2) == HH_ERR_INVALID_ARGUMENT);
   puts("PASS: runtime ABC/turbo per-player isolation, target validation and controller assignment swap");
   return 0;
}
