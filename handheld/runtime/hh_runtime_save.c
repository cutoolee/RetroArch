#include "hh_runtime_internal.h"

hh_result_t hh_runtime_set_state_slot(int slot)
{
   uint64_t request_id = 0;
   if (slot < -1 || slot > 999)
      return HH_ERR_INVALID_ARGUMENT;
   return hh_runtime_submit_command(HH_CMD_SET_STATE_SLOT,
         slot, &request_id);
}

hh_result_t hh_runtime_save_state(void)
{
   uint64_t request_id = 0;
   return hh_runtime_submit_command(HH_CMD_SAVE_STATE, 0, &request_id);
}

hh_result_t hh_runtime_load_state(void)
{
   uint64_t request_id = 0;
   return hh_runtime_submit_command(HH_CMD_LOAD_STATE, 0, &request_id);
}

hh_result_t hh_runtime_take_screenshot(void)
{
   uint64_t request_id = 0;
   return hh_runtime_submit_command(HH_CMD_SCREENSHOT, 0, &request_id);
}
