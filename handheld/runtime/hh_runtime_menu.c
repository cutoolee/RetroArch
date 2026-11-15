#include "hh_runtime_internal.h"

hh_result_t hh_runtime_open_ra_menu(void)
{
   uint64_t request_id = 0;
   return hh_runtime_submit_command(HH_CMD_OPEN_RA_MENU, 0,
         &request_id);
}
