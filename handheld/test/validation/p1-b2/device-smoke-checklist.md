# P1-B2 Device Runtime Smoke Checklist

This checklist must be executed only after P1-B1 produces the enabled APK.
Every row must reference the same enabled APK SHA256.

## Locked artifact

```text
APK filename:
APK SHA256:
Commit SHA:
Device model:
Android version:
ABI:
```

## Test order

| ID | Test | Result | Evidence |
|---|---|---|---|
| B2-01 | APK install | NOT_RUN | |
| B2-02 | App launch without crash | NOT_RUN | |
| B2-03 | Chinese / font / assets | NOT_RUN | |
| B2-04 | Load verified Core + ROM | NOT_RUN | |
| B2-05 | Video / gameplay | NOT_RUN | |
| B2-06 | Runtime snapshot | NOT_RUN | |
| B2-07 | Pause | NOT_RUN | |
| B2-08 | Resume | NOT_RUN | |
| B2-09 | Save state slot 0 | NOT_RUN | |
| B2-10 | Load state slot 0 | NOT_RUN | |
| B2-11 | Screenshot | NOT_RUN | |
| B2-12 | Reset | NOT_RUN | |
| B2-13 | Open native RetroArch menu | NOT_RUN | |
| B2-14 | Exit native menu and return to game | NOT_RUN | |
| B2-15 | Close content | NOT_RUN | |
| B2-16 | logcat fatal crash / ANR check | NOT_RUN | |
| B2-17 | Save → Save queue/BUSY | NOT_RUN | |
| B2-18 | Save → Load queue/BUSY | NOT_RUN | |
| B2-19 | Load → Save queue/BUSY | NOT_RUN | |
| B2-20 | request_id and completion/event correlation | NOT_RUN | |

## Native menu blocker

```text
Game → hh_runtime_open_ra_menu() → Ozone → Back → Game
```

This must have direct device evidence. It cannot be inferred from the
existence of `CMD_EVENT_MENU_TOGGLE` or from a successful APK build.

## B4 regression on the same APK

```text
Build: NOT_RUN
Install: NOT_RUN
Launch: NOT_RUN
Chinese/Menu/Font: NOT_RUN
Core Load: NOT_RUN
ROM Launch: NOT_RUN
Video: NOT_RUN
Fatal Crash: NOT_RUN
Clean Exit: NOT_RUN
B4-v3 Regression: NOT_RUN
```
