#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "handheld/runtime/hh_runtime_internal.h"
#include "input/input_driver.h"
#include "retroarch.h"

static settings_t settings;
static runloop_state_t runloop_st;
static input_driver_state_t input_st;
static bool save_ok = true, config_ok = true, has_config;
static unsigned saved_mask, saved_period;
static char saved_path[4096];
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
bool path_is_empty(enum rarch_path_type type) { (void)type; return !has_config; }
bool config_save_file(const char *path) { (void)path; return config_ok; }
bool input_remapping_save_file(const char *path)
{
   snprintf(saved_path, sizeof(saved_path), "%s", path);
   saved_mask = settings.uints.input_action_mask[0][0];
   saved_period = settings.uints.input_action_period[1][0];
   return save_ok;
}
bool retroarch_ctl(enum rarch_ctl_state state, void *data)
{ (void)state; (void)data; return true; }

static void test_console_turbo_targets(void)
{
   hh_runtime_controls_t controls;
   unsigned pass;
   for (pass = 0; pass < 2; pass++)
   {
      memset(runloop_st.system.input_desc_btn[0], 0,
            sizeof(runloop_st.system.input_desc_btn[0]));
      runloop_st.system.info.library_name = pass ? "mGBA" : "FCEUmm";
      runloop_st.system.input_desc_btn[0][0] = "B";
      runloop_st.system.input_desc_btn[0][8] = "A";
      runloop_st.system.input_desc_btn[0][1] = "Turbo B";
      runloop_st.system.input_desc_btn[0][9] = "Turbo A";
      if (pass)
      {
         runloop_st.system.input_desc_btn[0][10] = "L";
         runloop_st.system.input_desc_btn[0][11] = "R";
         runloop_st.system.input_desc_btn[0][12] = "Turbo L";
         runloop_st.system.input_desc_btn[0][13] = "Turbo R";
      }
      assert(hh_runtime_get_controls(0, &controls) == HH_OK);
      assert(controls.available[1] && controls.available[9]);
      assert(!strcmp(controls.targets[1], "Turbo B"));
      assert(!strcmp(controls.targets[9], "Turbo A"));
      assert(controls.available[12] == (pass != 0));
      assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET, 512) == HH_OK);
      assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET, (1 << 16) | 2) == HH_OK);
      assert(hh_runtime_get_controls_draft(0, &controls) == HH_OK);
      assert(controls.masks[0] == 512 && controls.masks[1] == 2);
      assert(settings.uints.input_action_mask[0][0] == 0);
      assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SAVE, pass) == HH_OK);
      assert(hh_runtime_get_controls(0, &controls) == HH_OK);
      assert(controls.custom[0] && controls.masks[0] == 512 && !controls.periods[0]);
      assert(controls.custom[1] && controls.masks[1] == 2 && !controls.periods[1]);
      assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET, 0) == HH_OK);
      assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SAVE, pass) == HH_OK);
   }
   puts("PASS: FCEUmm/mGBA native Turbo A/B exposed, draft isolation and game/core save keep native bits without double turbo");
}

int main(void)
{
   hh_runtime_controls_t controls;
   unsigned i;
   hh_runtime_state.initialized = true;
   hh_runtime_state.owner_thread_id = 1;
   settings.uints.input_max_users = 4;
   runloop_st.system.info.library_name = "Test core";
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
   assert(controls.available[0] && controls.available[3]);
   assert(!strcmp(controls.targets[3], "Turbo A"));
   assert(controls.device_count == 2);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET, 7) == HH_OK);
   assert(settings.uints.input_action_mask[0][0] == 0);
   assert(hh_runtime_get_controls_draft(0, &controls) == HH_OK);
   assert(controls.custom[0] && controls.masks[0] == 7);
   assert(settings.uints.input_action_mask[1][0] == 0);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET,
            (6 << 24) | (1 << 20) | 1) == HH_OK);
   assert(settings.uints.input_action_period[1][0] == 0);
   assert(hh_runtime_get_controls_draft(1, &controls) == HH_OK);
   assert(controls.periods[0] == 6);
   assert(settings.uints.input_action_period[0][0] == 0);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET, 8) == HH_OK);
   assert(hh_runtime_get_controls_draft(0, &controls) == HH_OK);
   assert(controls.masks[0] == 8);
   assert(settings.uints.input_action_mask[0][0] == 0);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET, 16) == HH_ERR_INVALID_ARGUMENT);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET, (1 << 24) | 1) == HH_ERR_INVALID_ARGUMENT);
   memset(runloop_st.system.input_desc_btn[0], 0,
         sizeof(runloop_st.system.input_desc_btn[0]));
   runloop_st.system.input_desc_btn[0][0] = "Button A";
   runloop_st.system.input_desc_btn[0][8] = "Button B";
   runloop_st.system.input_desc_btn[0][1] = "Button C";
   runloop_st.system.input_desc_btn[0][9] = "Button D";
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET, 771) == HH_OK);
   assert(hh_runtime_get_controls_draft(0, &controls) == HH_OK);
   assert(controls.custom[0] && controls.masks[0] == 771);
   assert(hh_runtime_get_controls(0, &controls) == HH_OK);
   assert(!controls.custom[0]);
   assert(settings.uints.input_action_mask[1][0] == 0);
   input_st.input_action_phase[0][0] = 3;
   has_config = true;
   config_ok = false;
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SAVE, 0) == HH_ERR_SAVE_FAILED);
   assert(settings.uints.input_action_mask[0][0] == 0);
   assert(hh_runtime_state.control_draft_changed[0] == 1);
   config_ok = true;
   save_ok = false;
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SAVE, 0) == HH_ERR_SAVE_FAILED);
   assert(settings.uints.input_action_mask[0][0] == 0);
   assert(settings.uints.input_action_period[1][0] == 0);
   assert(input_st.input_action_phase[0][0] == 3);
   assert(hh_runtime_get_controls_draft(0, &controls) == HH_OK);
   assert(controls.custom[0] && controls.masks[0] == 771);
   save_ok = true;
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SAVE, 0) == HH_OK);
   assert(strstr(saved_path, "Test core/game.rmp"));
   assert(saved_mask == 771 && saved_period == 6);
   assert(settings.uints.input_action_mask[0][0] == 771);
   assert(settings.uints.input_action_mask[1][0] == 1);
   assert(settings.uints.input_action_period[1][0] == 6);
   assert(input_st.input_action_phase[0][0] == 0);
   assert(hh_runtime_state.control_draft_changed[0] == 0);
   assert(hh_runtime_state.control_draft_changed[1] == 0);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET, 0) == HH_OK);
   hh_runtime_controls_discard();
   assert(hh_runtime_get_controls_draft(0, &controls) == HH_OK);
   assert(controls.custom[0] && controls.masks[0] == 771);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SET, 0) == HH_OK);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_SAVE, 1) == HH_OK);
   assert(strstr(saved_path, "Test core/Test core.rmp"));
   assert(settings.uints.input_action_mask[0][0] == 0);
   input_st.input_action_phase[0][0] = 3;
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_DEVICE, 1) == HH_OK);
   assert(settings.uints.input_joypad_index[0] == 1);
   assert(settings.uints.input_joypad_index[1] == 0);
   assert(settings.uints.input_joypad_index[2] == 2);
   assert(input_st.input_action_phase[0][0] == 0);
   assert(hh_runtime_controls_command(HH_CMD_CONTROLS_DEVICE, 2) == HH_ERR_INVALID_ARGUMENT);
   test_console_turbo_targets();
   puts("PASS: runtime draft isolation, batch game/core save, failure rollback, discard and controller assignment swap");
   return 0;
}
