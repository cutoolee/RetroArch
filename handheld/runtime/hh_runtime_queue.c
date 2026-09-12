#include "hh_runtime_internal.h"

static hh_runtime_request_t *hh_runtime_find_free_request(void)
{
   size_t i;
   for (i = 0; i < HH_RUNTIME_QUEUE_CAPACITY; i++)
      if (!hh_runtime_state.requests[i].in_use)
         return &hh_runtime_state.requests[i];
   return NULL;
}

static hh_runtime_request_t *hh_runtime_find_queued_request(void)
{
   size_t i;
   for (i = 0; i < HH_RUNTIME_QUEUE_CAPACITY; i++)
      if (hh_runtime_state.requests[i].in_use
            && hh_runtime_state.requests[i].queued)
         return &hh_runtime_state.requests[i];
   return NULL;
}

static hh_event_type_t hh_runtime_event_for_command(hh_command_type_t type,
      bool opened)
{
   switch (type)
   {
      case HH_CMD_PAUSE:          return HH_EVENT_PAUSED;
      case HH_CMD_RESUME:         return HH_EVENT_RESUMED;
      case HH_CMD_RESET:          return HH_EVENT_RESET;
      case HH_CMD_CLOSE_CONTENT:  return HH_EVENT_CONTENT_CLOSED;
      case HH_CMD_SET_STATE_SLOT: return HH_EVENT_STATE_SLOT_CHANGED;
      case HH_CMD_SAVE_STATE:     return HH_EVENT_STATE_SAVE_ACCEPTED;
      case HH_CMD_LOAD_STATE:     return HH_EVENT_STATE_LOAD_ACCEPTED;
      case HH_CMD_SCREENSHOT:     return HH_EVENT_SCREENSHOT_TAKEN;
      case HH_CMD_OPEN_RA_MENU:
         return opened ? HH_EVENT_RA_MENU_OPENED : HH_EVENT_RA_MENU_CLOSED;
      default:                    return HH_EVENT_ERROR;
   }
}

static void hh_runtime_finish_request(hh_runtime_request_t *request,
      hh_result_t result)
{
   const uint64_t request_id = request->request_id;
   const hh_command_type_t type = request->type;
   hh_event_type_t event_type;
   hh_runtime_snapshot_t snapshot;
   hh_runtime_event_callback_t callback;
   void *userdata;
   hh_runtime_event_t event;

   hh_runtime_refresh_snapshot();
   if (result != HH_OK)
      hh_runtime_set_last_error(result);
   memset(&event, 0, sizeof(event));
   event.request_id = request_id;
   event.result = result;
   event_type = hh_runtime_event_for_command(type,
         hh_runtime_state.snapshot.ra_menu_open);
   event.type = result == HH_OK ? event_type : HH_EVENT_ERROR;

   slock_lock(hh_runtime_state.lock);
   snapshot = hh_runtime_state.snapshot;
   event.snapshot = snapshot;
   if (hh_runtime_command_is_state_io(type))
      hh_runtime_state.operation_busy = false;
   if (hh_runtime_state.processing_count)
      hh_runtime_state.processing_count--;
   request->processing = false;
   request->in_use = false;
   if (!hh_runtime_state.processing_count && !hh_runtime_state.initialized)
      hh_runtime_state.shutting_down = false;

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

   slock_lock(hh_runtime_state.lock);
   if (!hh_runtime_state.initialized && !hh_runtime_state.processing_count)
   {
      hh_runtime_state.callback = NULL;
      hh_runtime_state.callback_userdata = NULL;
   }
   slock_unlock(hh_runtime_state.lock);
}

hh_result_t hh_runtime_submit_command(
      hh_command_type_t type, int int_arg, uint64_t *request_id)
{
   hh_runtime_request_t *request;
   uint64_t id;

   if (!request_id)
      return HH_ERR_INVALID_ARGUMENT;
   if (type <= HH_CMD_NONE || type > HH_CMD_OPEN_RA_MENU)
      return HH_ERR_INVALID_ARGUMENT;
   if (type == HH_CMD_SCREENSHOT && !hh_runtime_has_capability(HH_CAP_SCREENSHOT))
      return HH_ERR_UNSUPPORTED;
   if (type == HH_CMD_OPEN_RA_MENU && !hh_runtime_has_capability(HH_CAP_RA_MENU))
      return HH_ERR_UNSUPPORTED;
   if (!hh_runtime_state.lock)
      return HH_ERR_NOT_INITIALIZED;

   slock_lock(hh_runtime_state.lock);
   if (!hh_runtime_state.initialized || hh_runtime_state.shutting_down)
   {
      slock_unlock(hh_runtime_state.lock);
      return HH_ERR_NOT_INITIALIZED;
   }
   if (hh_runtime_command_is_state_io(type) && (hh_runtime_state.operation_busy || hh_runtime_state.state_task_busy))
   {
      slock_unlock(hh_runtime_state.lock);
      return HH_ERR_BUSY;
   }

   request = hh_runtime_find_free_request();
   if (!request)
   {
      slock_unlock(hh_runtime_state.lock);
      return HH_ERR_BUSY;
   }
   id = hh_runtime_state.next_request_id++;
   if (!id)
      id = hh_runtime_state.next_request_id++;
   *request_id = id;
   memset(request, 0, sizeof(*request));
   request->in_use = true;
   request->queued = true;
   request->request_id = id;
   request->type = type;
   request->int_arg = int_arg;
   if (hh_runtime_command_is_state_io(type))
      hh_runtime_state.operation_busy = true;
   slock_unlock(hh_runtime_state.lock);
   return HH_OK;
}

void hh_runtime_dispatch_pending(void)
{
   hh_runtime_request_t local;

   if (!hh_runtime_state.lock)
      return;
   slock_lock(hh_runtime_state.lock);
   if (!hh_runtime_state.initialized)
   {
      slock_unlock(hh_runtime_state.lock);
      return;
   }
   if (!hh_runtime_state.owner_thread_id
         || hh_runtime_state.owner_thread_id != sthread_get_current_thread_id())
   {
      slock_unlock(hh_runtime_state.lock);
      return;
   }
   slock_unlock(hh_runtime_state.lock);

   hh_runtime_refresh_snapshot();
   for (;;)
   {
      hh_runtime_request_t *request;
      hh_result_t result;
      slock_lock(hh_runtime_state.lock);
      request = hh_runtime_find_queued_request();
      if (!request)
      {
         slock_unlock(hh_runtime_state.lock);
         break;
      }
      local = *request;
      request->queued = false;
      request->processing = true;
      hh_runtime_state.processing_count++;
      slock_unlock(hh_runtime_state.lock);

      if (hh_runtime_command_is_state_io(local.type))
         hh_runtime_state_task_begin(local.request_id, local.type);
      result = hh_runtime_execute_command(local.type, local.int_arg);
      hh_runtime_finish_request(request, result);
      if (hh_runtime_command_is_state_io(local.type))
         hh_runtime_state_task_end(result);
      hh_runtime_state_task_publish();
   }
}

void hh_runtime_owner_tick(void)
{
   if (!hh_runtime_state.lock)
      return;
   slock_lock(hh_runtime_state.lock);
   if (!hh_runtime_state.initialized)
   {
      slock_unlock(hh_runtime_state.lock);
      return;
   }
   if (!hh_runtime_state.owner_thread_id)
      hh_runtime_state.owner_thread_id = sthread_get_current_thread_id();
   slock_unlock(hh_runtime_state.lock);
   hh_runtime_state_task_publish();
   hh_runtime_dispatch_pending();
}

hh_result_t hh_runtime_set_event_callback(
      hh_runtime_event_callback_t callback, void *userdata)
{
   if (!hh_runtime_state.lock)
      return HH_ERR_NOT_INITIALIZED;
   slock_lock(hh_runtime_state.lock);
   if (!hh_runtime_state.initialized)
   {
      slock_unlock(hh_runtime_state.lock);
      return HH_ERR_NOT_INITIALIZED;
   }
   hh_runtime_state.callback = callback;
   hh_runtime_state.callback_userdata = userdata;
   slock_unlock(hh_runtime_state.lock);
   return HH_OK;
}

hh_result_t hh_runtime_poll_event(hh_runtime_event_t *out)
{
   if (!out)
      return HH_ERR_INVALID_ARGUMENT;
   if (!hh_runtime_state.lock)
      return HH_ERR_NOT_INITIALIZED;
   slock_lock(hh_runtime_state.lock);
   if (!hh_runtime_state.event_count)
   {
      slock_unlock(hh_runtime_state.lock);
      return HH_ERR_BUSY;
   }
   *out = hh_runtime_state.events[hh_runtime_state.event_head];
   hh_runtime_state.event_head =
      (hh_runtime_state.event_head + 1) % HH_RUNTIME_QUEUE_CAPACITY;
   hh_runtime_state.event_count--;
   slock_unlock(hh_runtime_state.lock);
   return HH_OK;
}
