# P2.1 request correlation evidence

Status: `NOT_RUN`

Use one row per submitted request. Preserve raw log lines or screenshots for
each row.

| Case | Command/action | request_id | Accepted event | Completion event | UI result | Evidence |
|---|---|---:|---|---|---|---|
| Save success | SAVE | | | `HH_EVENT_STATE_SAVE_COMPLETED` | | |
| Save error | SAVE | | | `HH_EVENT_STATE_SAVE_COMPLETED` | | |
| Load success | LOAD | | | `HH_EVENT_STATE_LOAD_COMPLETED` | | |
| Load error | LOAD | | | `HH_EVENT_STATE_LOAD_COMPLETED` | | |
| Mismatch ignored | SAVE/LOAD | | mismatched | ignored | remains pending | |
| Late after close/deinit | SAVE/LOAD | | | ignored | closed/new request unchanged | |
| Save → Save | SAVE | | busy/ignored | first request only | | |
| Save → Load | SAVE then LOAD | | busy/ignored | first request only | | |
| Load → Save | LOAD then SAVE | | busy/ignored | first request only | | |

Required assertions: IDs are non-zero and preserved end-to-end; a mismatched
ID cannot complete the pending action; a late callback cannot complete a later
request; and a second state operation cannot replace the first pending ID.
