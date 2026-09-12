# P2.0.5 Quick Menu validation

Run from the repository root:

```sh
validation/p2/tests/test_quick_menu.sh
SANITIZE=1 validation/p2/tests/test_quick_menu.sh validation/p2/preview
```

The second command also generates `preview/index.html` and 33 SVGs from the
production C renderer (11 mock states at 640×480, 1280×720, 1920×1080).
The HTML is a static review gallery, not a second implementation of the UI.
Mock game/slot data lives only in `tests/mock_quick_menu.h`.

The test compiler preserves P2.0's `stdbool.h` ABI. Clang checks C89 syntax with
only its inherited C99-extension diagnostic suppressed for `bool`; other
compilers use C99 with declaration-after-statement warnings treated as errors.

## UI contract

- Inject copied game/slot data with `hh_quick_menu_set_game` / `set_slot`, and
  UI capabilities with `set_capabilities`. Individual item/slot `disabled`
  flags also apply. Main items wrap; slots stop at 0 and 9.
- Feed semantic input to `hh_quick_menu_input`. Save/Load first enter a slot
  page. On an emitted action, read `view.pending_action` and `view.pending_slot`
  before completing it. Slot is -1 for actions without a slot. Pending locks
  navigation, confirmation and closing until a result is supplied.
- Supply a UI-only result via `hh_quick_menu_action_result`, or generic success
  via the existing `action_complete`. This does not change slot metadata.
  Feedback remains until cleared, another action starts, a page is left, or
  the menu is reopened. The caller controls `busy` independently.
- `hh_quick_menu_render` emits synchronous rectangle/text/optional-preview
  callbacks. Colors are RRGGBBAA. Text callbacks must clip or ellipsize UTF-8
  text to the supplied bounds. No font loading, disk access or graphics backend
  is embedded. A missing/failed/disabled preview uses `No Preview`.
- Open/close transitions use the original explicit finish calls; no animation
  thread or timed transition was added. `CLOSED` emits no drawing commands.

## Regression boundary

Discovery used `find validation handheld -type f | sort` and searched the
repository and workflows. No standalone P1 host Runtime test suite is present.
The existing Android environment check was executed; its output is in
`p1-environment.txt`. Device Harness and CI were not invoked or modified.
`results.yaml` distinguishes UI validation from the blocked Android regression.
