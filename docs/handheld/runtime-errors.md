# Handheld Runtime Errors

All actions return `hh_result_t`. The meanings are:

| Result | Meaning |
|---|---|
| `HH_OK` | Action was accepted into the Runtime queue; execution result is in its completion event |
| `HH_ERR_NOT_INITIALIZED` | Runtime is not available or is shutting down |
| `HH_ERR_NO_CONTENT` | Action needs loaded content |
| `HH_ERR_INVALID_STATE` | Action conflicts with the current snapshot |
| `HH_ERR_BUSY` | Queue is full or Save/Load is already in flight |
| `HH_ERR_UNSUPPORTED` | Build lacks the requested upstream capability |
| `HH_ERR_INVALID_ARGUMENT` | Null output, invalid command, or slot outside -1..999 |
| `HH_ERR_SAVE_FAILED` | RetroArch rejected Save State dispatch |
| `HH_ERR_LOAD_FAILED` | RetroArch rejected Load State dispatch |
| `HH_ERR_SCREENSHOT_FAILED` | RetroArch rejected screenshot dispatch |
| `HH_ERR_RA_COMMAND_FAILED` | Another RetroArch command was rejected |
| `HH_ERR_INTERNAL` | Runtime adapter failure |

For non-owner callers, `HH_OK` means enqueue acceptance. The event result is
the authoritative execution result and is correlated by `request_id`.
