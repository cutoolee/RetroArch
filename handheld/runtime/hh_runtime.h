/* Handheld Runtime API - the only supported product boundary for RA control. */
#ifndef HH_RUNTIME_H
#define HH_RUNTIME_H

#include "hh_runtime_types.h"

#ifdef __cplusplus
extern "C" {
#endif

hh_result_t hh_runtime_init(void);
void hh_runtime_deinit(void);

hh_result_t hh_runtime_get_snapshot(hh_runtime_snapshot_t *out);

hh_result_t hh_runtime_pause(void);
hh_result_t hh_runtime_resume(void);
hh_result_t hh_runtime_reset(void);
hh_result_t hh_runtime_close_content(void);

hh_result_t hh_runtime_set_state_slot(int slot);
hh_result_t hh_runtime_get_state_slot(int *slot);
hh_result_t hh_runtime_save_state(void);
hh_result_t hh_runtime_load_state(void);
hh_result_t hh_runtime_take_screenshot(void);

hh_result_t hh_runtime_open_ra_menu(void);
bool hh_runtime_has_capability(hh_capability_t capability);

/* P1.5 queue/dispatcher boundary. The dispatcher is called on the RA owner
 * thread by the runloop integration. External callers only enqueue. */
hh_result_t hh_runtime_submit_command(
      hh_command_type_t type, int int_arg, uint64_t *request_id);
/* Called only by the RA runloop integration. Product code must not call it. */
void hh_runtime_owner_tick(void);
/* Kept for embedders that provide an equivalent owner-thread pump. It never
 * binds an owner by itself. */
void hh_runtime_dispatch_pending(void);

hh_result_t hh_runtime_set_event_callback(
      hh_runtime_event_callback_t callback, void *userdata);
hh_result_t hh_runtime_poll_event(hh_runtime_event_t *out);

const char *hh_result_to_string(hh_result_t result);
const char *hh_runtime_mode_to_string(hh_runtime_mode_t mode);
const char *hh_event_type_to_string(hh_event_type_t type);

#ifdef __cplusplus
}
#endif

#endif
