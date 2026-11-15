# Handheld Runtime API

P1 exposes a stable C ABI in `handheld/runtime/hh_runtime.h`. Product code
must include this header only; RetroArch command, runloop, configuration,
menu, and video internals stay inside the adapter translation unit.

## Actions

`hh_runtime_init()` creates the queue and publishes a
`HH_EVENT_RUNTIME_READY` event. `hh_runtime_deinit()` rejects new work and
clears commands that have not reached the owner thread. Every action,
including actions submitted by the owner thread, is accepted into the same
queue and returns `HH_OK`; only `hh_runtime_owner_tick()` dispatches it. The
final result is carried by `hh_runtime_event_t` through the registered
callback or `hh_runtime_poll_event()`.

For callers that need a correlation key, use
`hh_runtime_submit_command(type, arg, &request_id)` directly. Request IDs are
monotonic for one Runtime lifetime and are included in every completion event.

Save and load share one in-flight guard. A second Save/Load request returns
`HH_ERR_BUSY` until the first request is dispatched. RetroArch's own save/load
tasks remain authoritative: a successful P1 result means the upstream command
was accepted, not that an asynchronous disk task has already completed.
The corresponding events therefore mean `STATE_SAVE_ACCEPTED` and
`STATE_LOAD_ACCEPTED` at the Runtime boundary; the public compatibility names
`HH_EVENT_STATE_SAVED` and `HH_EVENT_STATE_LOADED` retain the P1 terminology.

The owner thread is bound only by the guarded RA runloop integration
(`hh_runtime_owner_tick`). A normal caller cannot claim ownership by calling
the public queue pump. If deinit races with an in-flight owner operation, the
operation is allowed to finish, queued work is rejected with an event, and a
new init returns `HH_ERR_BUSY` until the in-flight operation has drained.
The registered callback remains active long enough to receive queued rejection
and in-flight completion events; it is cleared after the lifecycle has drained.

## Public surface

- lifecycle: `hh_runtime_init`, `hh_runtime_deinit`
- query: `hh_runtime_get_snapshot`, `hh_runtime_get_state_slot`
- command: pause, resume, reset, close content
- state: set slot, save, load, screenshot
- menu: `hh_runtime_open_ra_menu`
- capability/error/event: `hh_runtime_has_capability`, result strings,
  callback and event polling

No public function returns a RetroArch internal pointer.
