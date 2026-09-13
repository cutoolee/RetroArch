#include "hh_runtime_internal.h"

hh_runtime_state_t hh_runtime_state = {0};

void hh_runtime_set_last_error(hh_result_t result)
{
   if (!hh_runtime_state.lock)
      return;
   slock_lock(hh_runtime_state.lock);
   hh_runtime_state.last_error = result;
   if (result != HH_OK && hh_runtime_state.initialized)
      hh_runtime_state.snapshot.runtime_mode = HH_RUNTIME_MODE_ERROR;
   slock_unlock(hh_runtime_state.lock);
}

hh_result_t hh_runtime_init(void)
{
   if (!hh_runtime_state.lock)
      hh_runtime_state.lock = slock_new();
   if (!hh_runtime_state.lock)
      return HH_ERR_INTERNAL;

   slock_lock(hh_runtime_state.lock);
   if (hh_runtime_state.initialized)
   {
      slock_unlock(hh_runtime_state.lock);
      return HH_OK;
   }
   if (hh_runtime_state.processing_count)
   {
      slock_unlock(hh_runtime_state.lock);
      return HH_ERR_BUSY;
   }
   memset(hh_runtime_state.requests, 0,
         sizeof(hh_runtime_state.requests));
   memset(hh_runtime_state.events, 0,
         sizeof(hh_runtime_state.events));
   memset(&hh_runtime_state.snapshot, 0,
         sizeof(hh_runtime_state.snapshot));
   hh_runtime_state.initialized = true;
   hh_runtime_state.shutting_down = false;
   hh_runtime_state.operation_busy = false;
   hh_runtime_state.state_task_busy = false;
   hh_runtime_state.state_task_binding = false;
   hh_runtime_state.state_task_attached = false;
   hh_runtime_state.state_task_done = false;
   hh_runtime_state.state_task_result = HH_OK;
   hh_runtime_state.owner_thread_id = 0;
   hh_runtime_state.next_request_id = 1;
#ifdef HAVE_GAMEGO_E2E_HARNESS
   memset(&hh_runtime_state.e2e_state_io, 0,
         sizeof(hh_runtime_state.e2e_state_io));
#endif
   hh_runtime_state.last_error = HH_OK;
   hh_runtime_state.event_head = 0;
   hh_runtime_state.event_count = 0;
   hh_runtime_state.processing_count = 0;
   hh_runtime_state.snapshot.initialized = true;
   hh_runtime_state.snapshot.runtime_mode = HH_RUNTIME_MODE_READY_NO_CONTENT;
   hh_runtime_state.snapshot.state_slot = 0;
   slock_unlock(hh_runtime_state.lock);

   hh_runtime_emit_event(0, HH_EVENT_RUNTIME_READY, HH_OK);
   return HH_OK;
}

void hh_runtime_deinit(void)
{
   size_t i;
   size_t rejected_count = 0;
   uint64_t rejected[HH_RUNTIME_QUEUE_CAPACITY];
   if (!hh_runtime_state.lock)
      return;

   hh_runtime_state_task_cancel(HH_ERR_NOT_INITIALIZED);
   slock_lock(hh_runtime_state.lock);
   hh_runtime_state.shutting_down = true;
   for (i = 0; i < HH_RUNTIME_QUEUE_CAPACITY; i++)
   {
      if (hh_runtime_state.requests[i].in_use
            && hh_runtime_state.requests[i].queued)
      {
         rejected[rejected_count++] = hh_runtime_state.requests[i].request_id;
         memset(&hh_runtime_state.requests[i], 0,
               sizeof(hh_runtime_state.requests[i]));
      }
   }
   hh_runtime_state.operation_busy = false;
   hh_runtime_state.initialized = false;
   hh_runtime_state.state_task_busy = false;
   hh_runtime_state.state_task_binding = false;
   hh_runtime_state.state_task_attached = false;
   hh_runtime_state.owner_thread_id = 0;
   hh_runtime_state.snapshot.initialized = false;
   hh_runtime_state.snapshot.content_loaded = false;
   hh_runtime_state.snapshot.paused = false;
   hh_runtime_state.snapshot.ra_menu_open = false;
   hh_runtime_state.snapshot.runtime_mode = HH_RUNTIME_MODE_UNINITIALIZED;
   if (!hh_runtime_state.processing_count)
   {
      hh_runtime_state.shutting_down = false;
   }
   slock_unlock(hh_runtime_state.lock);

   for (i = 0; i < rejected_count; i++)
      hh_runtime_emit_event(rejected[i], HH_EVENT_ERROR,
            HH_ERR_NOT_INITIALIZED);

   slock_lock(hh_runtime_state.lock);
   if (!hh_runtime_state.processing_count && !hh_runtime_state.initialized)
   {
      hh_runtime_state.callback = NULL;
      hh_runtime_state.callback_userdata = NULL;
   }
   slock_unlock(hh_runtime_state.lock);
}

#ifdef HAVE_GAMEGO_E2E_HARNESS
hh_result_t hh_runtime_e2e_get_state_io_observation(
      hh_runtime_e2e_state_io_observation_t *out)
{
   if (!out)
      return HH_ERR_INVALID_ARGUMENT;
   if (!hh_runtime_state.lock)
      return HH_ERR_NOT_INITIALIZED;
   slock_lock(hh_runtime_state.lock);
   *out = hh_runtime_state.e2e_state_io;
   slock_unlock(hh_runtime_state.lock);
   return HH_OK;
}
#endif

void hh_runtime_emit_event(uint64_t request_id,
      hh_event_type_t type, hh_result_t result)
{
   hh_runtime_event_t event;
   hh_runtime_event_callback_t callback;
   void *userdata;

   if (!hh_runtime_state.lock)
      return;
   memset(&event, 0, sizeof(event));
   event.request_id = request_id;
   event.type = type;
   event.result = result;
   slock_lock(hh_runtime_state.lock);
   event.snapshot = hh_runtime_state.snapshot;
   if (hh_runtime_state.event_count == HH_RUNTIME_QUEUE_CAPACITY)
   {
      hh_runtime_state.event_head =
         (hh_runtime_state.event_head + 1) % HH_RUNTIME_QUEUE_CAPACITY;
      hh_runtime_state.event_count--;
   }
   {
      size_t index = (hh_runtime_state.event_head
            + hh_runtime_state.event_count) % HH_RUNTIME_QUEUE_CAPACITY;
      hh_runtime_state.events[index] = event;
      hh_runtime_state.event_count++;
   }
   callback = hh_runtime_state.callback;
   userdata = hh_runtime_state.callback_userdata;
   slock_unlock(hh_runtime_state.lock);
   if (callback)
      callback(&event, userdata);
}
