#include "hh_runtime_internal.h"

#include <stdio.h>
#include <file/file_path.h>
#include <string/stdstring.h>
#include "input/input_driver.h"
#include "input/input_remapping.h"
#include "retroarch.h"

static const char *hh_control_names[16] = {
   "B / 下方键", "Y / 左侧键", "Select", "Start",
   "方向上", "方向下", "方向左", "方向右",
   "A / 右侧键", "X / 上方键", "L1", "R1", "L2", "R2", "L3", "R3"
};

hh_result_t hh_runtime_get_controls(unsigned player, hh_runtime_controls_t *out)
{
   settings_t *settings = config_get_ptr();
   runloop_state_t *runloop_st = runloop_state_get_ptr();
   unsigned i, device, port, remap;
   bool descriptors = false;
   const char *name;
   if (!out || !settings || player >= MAX_USERS || player >= 16)
      return HH_ERR_INVALID_ARGUMENT;
   if (!hh_runtime_state.initialized || hh_runtime_state.owner_thread_id
         != sthread_get_current_thread_id())
      return HH_ERR_INVALID_STATE;
   if (!runloop_is_inited())
      return HH_ERR_NO_CONTENT;
   port = settings->uints.input_remap_ports[player];
   if (port >= MAX_USERS)
      return HH_ERR_INVALID_STATE;
   device = settings->uints.input_libretro_device[port] & RETRO_DEVICE_MASK;
   if (device != RETRO_DEVICE_JOYPAD && device != RETRO_DEVICE_ANALOG)
      return HH_ERR_UNSUPPORTED;
   memset(out, 0, sizeof(*out));
   out->player_count = settings->uints.input_max_users < 16
      ? settings->uints.input_max_users : 16;
   out->device_index = settings->uints.input_joypad_index[player];
   for (i = 0; i < 16; i++)
      if (runloop_st->system.input_desc_btn[port][i]
            && *runloop_st->system.input_desc_btn[port][i])
         descriptors = true;
   for (i = 0; i < 16; i++)
   {
      name = runloop_st->system.input_desc_btn[port][i];
      out->available[i] = !descriptors || (name && *name);
      strlcpy(out->targets[i], name && *name ? name : hh_control_names[i],
            sizeof(out->targets[i]));
      strlcpy(out->sources[i], hh_control_names[i], sizeof(out->sources[i]));
#ifdef ANDROID
      {
         const struct retro_keybind *bind = &input_config_binds[player][i];
         uint64_t key = bind->joykey;
         if (key == NO_BTN && out->device_index < MAX_USERS)
            key = input_autoconf_binds[out->device_index][i].joykey;
         if (key >= 96 && key <= 100 && key != 98)
            strlcpy(out->sources[i], key == 96 ? "A / 下方键" :
                  key == 97 ? "B / 右侧键" : key == 99 ? "X / 左侧键" :
                  "Y / 上方键", sizeof(out->sources[i]));
      }
#endif
      out->custom[i] = settings->uints.input_action_mask[player][i] != 0;
      remap = settings->uints.input_remap_ids[player][i];
      out->masks[i] = out->custom[i] ? settings->uints.input_action_mask[player][i]
         : remap < 16 ? 1U << remap : 0;
      out->periods[i] = settings->uints.input_action_period[player][i];
      name = input_config_get_device_display_name(i);
      if (!name || !*name)
         name = input_config_get_device_name(i);
      if (name && *name && out->device_count < 16)
      {
         unsigned row = out->device_count++;
         out->device_ids[row] = i;
         snprintf(out->devices[row], sizeof(out->devices[row]), "#%u %s", i + 1, name);
      }
   }
   return HH_OK;
}

void hh_runtime_controls_discard(void)
{
   memset(hh_runtime_state.control_draft_changed, 0,
         sizeof(hh_runtime_state.control_draft_changed));
}

hh_result_t hh_runtime_get_controls_draft(unsigned player, hh_runtime_controls_t *out)
{
   unsigned i, remap;
   settings_t *settings = config_get_ptr();
   hh_result_t result = hh_runtime_get_controls(player, out);
   if (result != HH_OK)
      return result;
   for (i = 0; i < 16; i++)
      if (hh_runtime_state.control_draft_changed[player] & (1U << i))
      {
         out->custom[i] = hh_runtime_state.control_draft_masks[player][i] != 0;
         remap = settings->uints.input_remap_ids[player][i];
         out->masks[i] = out->custom[i] ? hh_runtime_state.control_draft_masks[player][i]
            : remap < 16 ? 1U << remap : 0;
         out->periods[i] = hh_runtime_state.control_draft_periods[player][i];
      }
   return HH_OK;
}

static hh_result_t hh_runtime_controls_save(const char *path)
{
   settings_t *settings = config_get_ptr();
   unsigned masks[16][16], periods[16][16];
   unsigned player, source, target;
   hh_runtime_controls_t controls;
   for (player = 0; player < 16; player++)
   {
      if (!hh_runtime_state.control_draft_changed[player])
         continue;
      if (hh_runtime_get_controls(player, &controls) != HH_OK)
         return HH_ERR_UNSUPPORTED;
      for (source = 0; source < 16; source++)
         if (hh_runtime_state.control_draft_changed[player] & (1U << source))
            for (target = 0; target < 16; target++)
               if ((hh_runtime_state.control_draft_masks[player][source] & (1U << target))
                     && !controls.available[target])
                  return HH_ERR_INVALID_ARGUMENT;
   }
   if (!path_is_empty(RARCH_PATH_CONFIG) && !config_save_file(path_get(RARCH_PATH_CONFIG)))
      return HH_ERR_SAVE_FAILED;
   for (player = 0; player < 16 && player < MAX_USERS; player++)
      for (source = 0; source < 16; source++)
      {
         masks[player][source] = settings->uints.input_action_mask[player][source];
         periods[player][source] = settings->uints.input_action_period[player][source];
         if (hh_runtime_state.control_draft_changed[player] & (1U << source))
         {
            settings->uints.input_action_mask[player][source] =
               hh_runtime_state.control_draft_masks[player][source];
            settings->uints.input_action_period[player][source] =
               hh_runtime_state.control_draft_periods[player][source];
         }
      }
   if (!input_remapping_save_file(path))
   {
      for (player = 0; player < 16 && player < MAX_USERS; player++)
         for (source = 0; source < 16; source++)
         {
            settings->uints.input_action_mask[player][source] = masks[player][source];
            settings->uints.input_action_period[player][source] = periods[player][source];
         }
      return HH_ERR_SAVE_FAILED;
   }
   for (player = 0; player < 16 && player < MAX_USERS; player++)
      if (hh_runtime_state.control_draft_changed[player])
      {
         memset(input_state_get_ptr()->input_action_phase[player], 0,
               sizeof(input_state_get_ptr()->input_action_phase[player]));
         BIT256_CLEAR_ALL(input_state_get_ptr()->mapper.buttons[player]);
      }
   hh_runtime_controls_discard();
   return HH_OK;
}

hh_result_t hh_runtime_controls_command(hh_command_type_t type, int argument)
{
   settings_t *settings = config_get_ptr();
   runloop_state_t *runloop_st = runloop_state_get_ptr();
   unsigned value = (unsigned)argument;
   unsigned player = (value >> 20) & 15;
   unsigned source = (value >> 16) & 15;
   unsigned mask = value & 0xffff;
   unsigned period = (value >> 24) & 63;
   unsigned i;
   hh_result_t result;
   hh_runtime_controls_t controls;
   char path[PATH_MAX_LENGTH], suffix[PATH_MAX_LENGTH];
   const char *core = runloop_st->system.info.library_name;
   const char *content = path_get(RARCH_PATH_BASENAME);
   if (!settings || !runloop_is_inited())
      return HH_ERR_NO_CONTENT;
   if (type == HH_CMD_CONTROLS_SAVE)
   {
      if (argument < 0 || argument > 1 || !core || !*core || !content || !*content)
         return HH_ERR_INVALID_ARGUMENT;
      strlcpy(suffix, core, sizeof(suffix));
      if (settings->bools.input_remap_sort_by_controller_enable)
      {
         const char *name = input_config_get_device_display_name(settings->uints.input_joypad_index[0]);
         if (name && *name)
            fill_pathname_join_special(suffix, core,
                  sanitize_path_part(name, strlen(name)), sizeof(suffix));
      }
      fill_pathname_join_special_ext(path, settings->paths.directory_input_remapping,
            suffix, argument ? core : path_basename(content), ".rmp", sizeof(path));
      result = hh_runtime_controls_save(path);
      if (result != HH_OK)
         return result;
      retroarch_ctl(argument ? RARCH_CTL_SET_REMAPS_CORE_ACTIVE :
            RARCH_CTL_SET_REMAPS_GAME_ACTIVE, NULL);
      input_state_get_ptr()->flags |= INP_FLAG_REMAPPING_CACHE_ACTIVE;
      return HH_OK;
   }
   if (hh_runtime_get_controls(player, &controls) != HH_OK)
      return HH_ERR_UNSUPPORTED;
   if (type == HH_CMD_CONTROLS_DEVICE)
   {
      unsigned device = value & 0xffff;
      unsigned previous = settings->uints.input_joypad_index[player];
      const char *name;
      if (device >= MAX_USERS)
         return HH_ERR_INVALID_ARGUMENT;
      name = input_config_get_device_name(device);
      if (!name || !*name)
         return HH_ERR_INVALID_ARGUMENT;
      for (i = 0; i < MAX_USERS; i++)
         if (i != player && settings->uints.input_joypad_index[i] == device)
            settings->uints.input_joypad_index[i] = previous;
      settings->uints.input_joypad_index[player] = device;
      memset(input_state_get_ptr()->input_action_phase, 0,
            sizeof(input_state_get_ptr()->input_action_phase));
      return HH_OK;
   }
   if (argument < 0 || (value >> 30) || period == 1 || period > 60)
      return HH_ERR_INVALID_ARGUMENT;
   for (i = 0; i < 16; i++)
      if ((mask & (1U << i)) && !controls.available[i])
         return HH_ERR_INVALID_ARGUMENT;
   hh_runtime_state.control_draft_masks[player][source] = mask;
   hh_runtime_state.control_draft_periods[player][source] = mask ? period : 0;
   hh_runtime_state.control_draft_changed[player] |= 1U << source;
   return HH_OK;
}
