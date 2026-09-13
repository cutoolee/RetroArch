# P2.1 execution record

Baseline: P1-B remains PASS. This change requires new targeted regression;
no new CI/device gate is claimed by this record.

## Source analysis before implementation

- Save success: task_save_handler reaches written == size, then
  task_save_handler_finished closes the stream. save_state_cb receives the
  final task error. Stream-close failure must also be propagated.
- Save failure: serialization/open/write/cancellation in task_save_handler;
  close failure in task_save_handler_finished; allocation/queue rejection in
  task constructors. Missing callback data is failure, never success.
- Load success: content_load_state_cb, after content_deserialize_state returns
  true and SRAM restoration finishes. Reading all bytes alone is insufficient.
- Load failure: handler read/open/cancel errors, callback data allocation,
  hardcore rejection, invalid buffer, or deserialize failure. Constructor
  rejection must release the pending operation as failure.
- Minimal hooks: task constructors attach an opaque Runtime token; final save
  and load callbacks notify Runtime. The backup-then-save callback transfers
  the same token to the final save task, without completing at backup success.
- Association: one active state operation, token containing request_id and a
  monotonically increasing lifecycle generation. Only matching tokens may
  finish the pending operation. Late callbacks after deinit cannot finish a
  later request, even when request_id allocation restarts at 1.
- Busy: retain command operation_busy semantics; add state_task_busy for the
  asynchronous lifetime. Submission checks either. No queue/dispatcher or
  request-id allocator redesign. Owner tick publishes recorded completion
  only after the command's ACCEPTED event.
- Close/deinit: invalidate pending token and report failure. Late task callbacks
  free their own token and cannot change a newer operation.

## Implemented

`HH_EVENT_STATE_SAVE_ACCEPTED` and `HH_EVENT_STATE_LOAD_ACCEPTED` remain queue
acknowledgements. The task callbacks now emit the independent COMPLETED events
with the final `HH_OK`, `HH_ERR_SAVE_FAILED`, or `HH_ERR_LOAD_FAILED` result.
The token carries the original request id and a generation guard. The bridge
keeps `ACTION_PENDING` until the matching COMPLETED event; its unit test covers
accepted-before-completed, mismatched request ids, save success, load failure,
pause ownership, and Advanced Menu resume ordering.

Local regressions run:

```
validation/p2/tests/test_quick_menu.sh       PASS
validation/p2.1/tests/test_bridge.sh         PASS
git diff --check                             PASS
runtime/task_save/runloop/gfx syntax checks   PASS
```

Android/ADB execution is CI-only in this environment because the local host
does not have a Java runtime or Android SDK. The P2.1 workflow runs both tests
and a feature-off Gradle configuration check.
