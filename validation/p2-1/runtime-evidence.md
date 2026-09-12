# P2.1 runtime lifecycle evidence

Status: `NOT_RUN`

## Required event traces

Capture a timestamp, command, action, `request_id`, event type, result, and UI
state for each trace.

```text
SAVE_COMPLETION_EVENT=HH_EVENT_STATE_SAVE_COMPLETED
LOAD_COMPLETION_EVENT=HH_EVENT_STATE_LOAD_COMPLETED
```

| Trace | Expected sequence | Result |
|---|---|---|
| Save success | SUBMIT → SAVE ACCEPTED → `HH_EVENT_STATE_SAVE_COMPLETED` (`HH_OK`) → success | NOT_RUN |
| Save failure | SUBMIT → SAVE ACCEPTED → `HH_EVENT_STATE_SAVE_COMPLETED` (error) → error | NOT_RUN |
| Load success | SUBMIT → LOAD ACCEPTED → `HH_EVENT_STATE_LOAD_COMPLETED` (`HH_OK`) → success | NOT_RUN |
| Load failure | SUBMIT → LOAD ACCEPTED → `HH_EVENT_STATE_LOAD_COMPLETED` (error) → error | NOT_RUN |
| Continue | pause ownership → resume → close | NOT_RUN |
| Reset | confirm → reset request/event → running content | NOT_RUN |
| Advanced | release pause ownership → native menu open | NOT_RUN |
| Exit | confirm → close content → closed menu | NOT_RUN |

An accepted Save/Load event must leave the UI in `ACTION_PENDING`/busy. No
success may be recorded from acceptance alone.
