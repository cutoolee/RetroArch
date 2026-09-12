#ifndef HH_RUNTIME_INTERNAL_H
#define HH_RUNTIME_INTERNAL_H

#include "hh_runtime.h"

#include <stdint.h>
#include <string.h>

#include <rthreads/rthreads.h>
#include <compat/strl.h>

#include "command.h"
#include "configuration.h"
#include "gfx/video_driver.h"
#include "menu/menu_driver.h"
#include "paths.h"
#include "runloop.h"

#define HH_RUNTIME_QUEUE_CAPACITY 32

typedef struct hh_runtime_request
{
   bool in_use;
   bool queued;
   bool processing;
   uint64_t request_id;
   hh_command_type_t type;
   int int_arg;
} hh_runtime_request_t;

typedef struct hh_runtime_state
{
   bool initialized;
   bool shutting_down;
   bool operation_busy;
   uintptr_t owner_thread_id;
   uint64_t next_request_id;
   hh_result_t last_error;
   hh_runtime_snapshot_t snapshot;
   hh_runtime_request_t requests[HH_RUNTIME_QUEUE_CAPACITY];
   hh_runtime_event_t events[HH_RUNTIME_QUEUE_CAPACITY];
   size_t event_head;
   size_t event_count;
   slock_t *lock;
   hh_runtime_event_callback_t callback;
   void *callback_userdata;
   unsigned processing_count;
} hh_runtime_state_t;

extern hh_runtime_state_t hh_runtime_state;

hh_result_t hh_runtime_execute_command(
      hh_command_type_t type, int int_arg);
void hh_runtime_refresh_snapshot(void);
void hh_runtime_emit_event(uint64_t request_id,
      hh_event_type_t type, hh_result_t result);
void hh_runtime_set_last_error(hh_result_t result);
bool hh_runtime_command_is_state_io(hh_command_type_t type);

#endif
