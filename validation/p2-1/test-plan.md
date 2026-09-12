# P2.1 Quick Menu validation plan

This plan validates the GameGo Quick Menu against the existing P2.0 UI tests
and the existing `validation/p2.1` bridge test. It is intentionally usable
before a device APK is available.

## Completion contract

The completion events are emitted only after the real asynchronous state task
reaches its final result.

```text
SAVE_COMPLETION_EVENT=HH_EVENT_STATE_SAVE_COMPLETED
LOAD_COMPLETION_EVENT=HH_EVENT_STATE_LOAD_COMPLETED
```

`ACCEPTED` is only queue acknowledgement. A UI success result requires the
matching completion event and the final result for the same `request_id`.

## Preconditions and evidence identity

Record the following for every device run:

```text
APK filename:
APK SHA256:
Commit/ref:
Device model:
Android version:
ABI:
Runtime/menu feature flags:
Run timestamp:
```

Host-only checks may run without an APK. Device evidence must use one locked
APK for install, launch, runtime, and regression checks.

## Test matrix

The blocking and busy behavior are part of the same pending-operation contract:
while an action is pending, navigation, confirmation, and close are blocked.

| ID | Scenario | Expected result | Evidence |
|---|---|---|---|
| MENU-01 | Enter GameGo menu from running content using MENU | Menu opens once; gameplay is paused or pause is pending | `device-evidence.md` |
| MENU-02 | Menu item order and focus | Continue, Save, Load, Reset, Advanced, Exit are present and focusable according to capability | `runtime-evidence.md` |
| BLOCK-01 | Action while runtime is busy | Input is ignored or reports busy; no duplicate request is submitted | `runtime-evidence.md`, `request-evidence.md` |
| CONT-01 | Continue | Menu closes only after resume/continue ownership is resolved; gameplay resumes | `device-evidence.md` |
| SAVE-01 | Submit Save | One Save request emits `ACCEPTED`; UI remains pending/busy | `request-evidence.md` |
| SAVE-02 | Save completion success | UI reports success only after `HH_EVENT_STATE_SAVE_COMPLETED` is received with `HH_OK` | `runtime-evidence.md` |
| SAVE-03 | Save completion error | UI reports Save error with no success flash or false slot metadata | `runtime-evidence.md` |
| LOAD-01 | Submit Load from an occupied slot | One Load request emits `ACCEPTED`; UI remains pending/busy | `request-evidence.md` |
| LOAD-02 | Load completion success | UI reports success only after `HH_EVENT_STATE_LOAD_COMPLETED` and `HH_OK` | `runtime-evidence.md` |
| LOAD-03 | Load completion error/empty slot | UI reports Load error and remains recoverable | `runtime-evidence.md` |
| RESET-01 | Confirm Reset | Reset is submitted once; content returns to a running state after completion | `device-evidence.md` |
| ADV-01 | Advanced menu | Quick Menu pause ownership is released before native RetroArch menu opens; Back returns to content | `device-evidence.md` |
| EXIT-01 | Confirm Exit | Content closes once and the Quick Menu cannot submit another action | `device-evidence.md` |
| PAUSE-01 | Already-paused content | Opening the menu does not claim or resume a pause owned by the user | `runtime-evidence.md` |
| PAUSE-02 | Pause pending, then close menu | Close waits for the matching pause/resume lifecycle and does not leave a stuck pause | `request-evidence.md` |
| PEND-01 | Pending action and repeated confirm/back | Navigation, confirmation, and close are blocked until result; no duplicate command | `runtime-evidence.md` |
| BUSY-01 | Save → Save | Second Save is rejected as busy or ignored; first request remains correlated | `request-evidence.md` |
| BUSY-02 | Save → Load | Load cannot overtake an active Save; no cross-action completion | `request-evidence.md` |
| BUSY-03 | Load → Save | Save cannot overtake an active Load; no cross-action completion | `request-evidence.md` |
| REQ-01 | Matching `request_id` | Accepted and completion events route to the originating action | `request-evidence.md` |
| REQ-02 | Mismatched `request_id` | Event is ignored; UI remains pending and no success/error is shown | `request-evidence.md` |
| REQ-03 | Late event after close/deinit | Late event cannot complete a newer request or mutate a closed menu | `request-evidence.md` |
| CRASH-01 | Launch and menu open | No fatal crash, force-close, or ANR during launch/menu entry | `crash-evidence.md` |
| CRASH-02 | Save/Load/Reset/Advanced/Exit | No fatal crash or ANR in each action path, including pending action plus close | `crash-evidence.md` |
| P1-01 | P1 runtime regression | Existing P1-B Android/environment and runtime checks remain green; feature-off build remains usable | `build-evidence.md`, `device-evidence.md` |

## Execution order

1. Run `scripts/check-evidence.sh` to validate this plan and its contract gate.
2. Run `scripts/run-validation.sh [output-directory]` for host tests and the
   feature-off configuration check when the Android toolchain is available.
3. Fill the build and device identity fields before installing an APK.
4. Execute MENU and pause ownership checks before action checks.
5. Execute Save/Load accepted and completion checks with log timestamps and
   `request_id` values.
6. Execute Reset, Advanced, Exit, crash/ANR, and P1 regression checks.

## Acceptance rules

- A test is `PASS` only when its expected behavior and linked evidence are
  both present.
- `NOT_RUN` means the test was not attempted. `BLOCKED` means a stated
  dependency prevented execution. Neither counts as `PASS`.
- Accepted Save/Load without verified completion is not a successful Save/Load.
- Any crash, ANR, duplicate request, mismatched completion, or premature
  success fails the affected case.
