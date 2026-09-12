#include "hh_runtime_internal.h"
#include "hh_runtime_task.h"
#include <stdlib.h>

typedef struct hh_runtime_task_token
{
   uint64_t id;
   uint64_t generation;
} hh_runtime_task_token_t;

void hh_runtime_state_task_begin(uint64_t id, hh_command_type_t type)
{
   slock_lock(hh_runtime_state.lock);
   hh_runtime_state.state_task_generation++;
   hh_runtime_state.state_task_id       = id;
   hh_runtime_state.state_task_type     = type;
   hh_runtime_state.state_task_busy     = true;
   hh_runtime_state.state_task_binding  = true;
   hh_runtime_state.state_task_attached = false;
   hh_runtime_state.state_task_done     = false;
   slock_unlock(hh_runtime_state.lock);
}

void *hh_runtime_state_task_attach(void)
{
   hh_runtime_task_token_t *token = NULL;
   if (!hh_runtime_state.lock)
      return NULL;
   slock_lock(hh_runtime_state.lock);
   if (hh_runtime_state.state_task_binding
         && !hh_runtime_state.state_task_attached
         && hh_runtime_state.owner_thread_id == sthread_get_current_thread_id())
   {
      token = (hh_runtime_task_token_t*)malloc(sizeof(*token));
      if (token)
      {
         token->id = hh_runtime_state.state_task_id;
         token->generation = hh_runtime_state.state_task_generation;
         hh_runtime_state.state_task_attached = true;
      }
   }
   slock_unlock(hh_runtime_state.lock);
   return token;
}

void hh_runtime_state_task_complete(void *data, bool success)
{
   hh_runtime_task_token_t *token = (hh_runtime_task_token_t*)data;
   if (!token)
      return;
   slock_lock(hh_runtime_state.lock);
   if (hh_runtime_state.state_task_busy
         && token->id == hh_runtime_state.state_task_id
         && token->generation == hh_runtime_state.state_task_generation)
   {
      hh_runtime_state.state_task_done = true;
      hh_runtime_state.state_task_result = success ? HH_OK :
         hh_runtime_state.state_task_type == HH_CMD_SAVE_STATE
         ? HH_ERR_SAVE_FAILED : HH_ERR_LOAD_FAILED;
   }
   slock_unlock(hh_runtime_state.lock);
   free(token);
}

void hh_runtime_state_task_end(hh_result_t result)
{
   slock_lock(hh_runtime_state.lock);
   hh_runtime_state.state_task_binding = false;
   if (result != HH_OK)
   {
      /* The command was not accepted, so there is no ACCEPTED event to
       * complete. Drop the provisional binding without publishing a second
       * lifecycle. */
      hh_runtime_state.state_task_busy = false;
      hh_runtime_state.state_task_done = false;
      hh_runtime_state.state_task_generation++;
   }
   else if (!hh_runtime_state.state_task_attached)
   {
      hh_runtime_state.state_task_done = true;
      hh_runtime_state.state_task_result =
         hh_runtime_state.state_task_type == HH_CMD_SAVE_STATE
         ? HH_ERR_SAVE_FAILED : HH_ERR_LOAD_FAILED;
   }
   slock_unlock(hh_runtime_state.lock);
}

void hh_runtime_state_task_publish(void)
{
   uint64_t id;
   hh_event_type_t type;
   hh_result_t result;
   slock_lock(hh_runtime_state.lock);
   if (!hh_runtime_state.state_task_busy || !hh_runtime_state.state_task_done
         || hh_runtime_state.state_task_binding)
   {
      slock_unlock(hh_runtime_state.lock);
      return;
   }
   id = hh_runtime_state.state_task_id;
   type = hh_runtime_state.state_task_type == HH_CMD_SAVE_STATE
      ? HH_EVENT_STATE_SAVE_COMPLETED : HH_EVENT_STATE_LOAD_COMPLETED;
   result = hh_runtime_state.state_task_result;
   hh_runtime_state.state_task_busy = false;
   slock_unlock(hh_runtime_state.lock);
   hh_runtime_refresh_snapshot();
   hh_runtime_emit_event(id, type, result);
}

void hh_runtime_state_task_cancel(hh_result_t result)
{
   slock_lock(hh_runtime_state.lock);
   if (hh_runtime_state.state_task_busy)
   {
      hh_runtime_state.state_task_result = result;
      hh_runtime_state.state_task_done = true;
      hh_runtime_state.state_task_generation++;
   }
   slock_unlock(hh_runtime_state.lock);
   hh_runtime_state_task_publish();
}
