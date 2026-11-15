#include <assert.h>
#include <stdio.h>

#include "handheld/runtime/hh_runtime_internal.h"

hh_runtime_state_t hh_runtime_state;
static hh_runtime_snapshot_t snapshot;
static unsigned command_count;
static bool command_ok = true;

void hh_runtime_refresh_snapshot(void) {}
hh_result_t hh_runtime_get_snapshot(hh_runtime_snapshot_t *out)
{
   *out = snapshot;
   return HH_OK;
}
void hh_runtime_set_last_error(hh_result_t result)
{
   hh_runtime_state.last_error = result;
}
bool command_event(enum event_command command, void *data)
{
   (void)data;
   assert(command == CMD_EVENT_UNPAUSE);
   command_count++;
   if (command_ok)
      snapshot.paused = false;
   return command_ok;
}
settings_t *config_get_ptr(void) { return NULL; }
hh_result_t hh_runtime_controls_command(hh_command_type_t type, int argument)
{ (void)type; (void)argument; return HH_ERR_UNSUPPORTED; }
hh_result_t hh_runtime_shader_command(hh_command_type_t type, int argument)
{ (void)type; (void)argument; return HH_ERR_UNSUPPORTED; }
void hh_runtime_shader_discard(void) {}
void hh_runtime_controls_discard(void) {}
void hh_runtime_state_task_cancel(hh_result_t result) { (void)result; }

int main(void)
{
   hh_runtime_state.lock = (slock_t*)&snapshot;
   snapshot.content_loaded = true;
   snapshot.paused = true;
   assert(hh_runtime_execute_command(HH_CMD_RESUME, 0) == HH_OK);
   assert(command_count == 1 && !snapshot.paused);
   assert(hh_runtime_execute_command(HH_CMD_RESUME, 0) == HH_OK);
   assert(command_count == 1);

   snapshot.content_loaded = false;
   assert(hh_runtime_execute_command(HH_CMD_RESUME, 0) == HH_ERR_NO_CONTENT);
   assert(command_count == 1);
   snapshot.content_loaded = true;
   snapshot.paused = true;
   command_ok = false;
   assert(hh_runtime_execute_command(HH_CMD_RESUME, 0)
         == HH_ERR_RA_COMMAND_FAILED);
   assert(hh_runtime_state.last_error == HH_ERR_RA_COMMAND_FAILED);
   hh_runtime_state.lock = NULL;
   assert(hh_runtime_execute_command(HH_CMD_RESUME, 0)
         == HH_ERR_NOT_INITIALIZED);
   puts("PASS: resume accepts running content and preserves command errors");
   return 0;
}
