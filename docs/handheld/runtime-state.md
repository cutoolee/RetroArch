# Handheld Runtime State

The owner thread refreshes a value snapshot once per runloop iteration and
after each dispatched command. Callers receive a copy protected by the
Runtime lock; they never read RetroArch's global state directly.

| Runtime mode | Meaning |
|---|---|
| `UNINITIALIZED` | `hh_runtime_init()` has not completed or deinit has run |
| `READY_NO_CONTENT` | Runtime is ready and no content is loaded |
| `CONTENT_RUNNING` | Content/core is loaded and not paused |
| `CONTENT_PAUSED` | Content is loaded and the RA paused flag is set |
| `RA_MENU` | Native RetroArch menu is alive |
| `CLOSING_CONTENT` | RetroArch has begun its asynchronous close path |
| `ERROR` | The last dispatched action returned a non-OK result |

The snapshot contains `initialized`, `content_loaded`, `paused`,
`ra_menu_open`, `state_slot`, `content_name`, `core_name`, `core_version`,
and `runtime_mode`.

`CLOSING_CONTENT` and a successful close event mean that the upstream close
command was accepted. The next owner-thread snapshot is the source of truth
for the final `READY_NO_CONTENT` state.
