#!/usr/bin/env python3
"""Allow only UI-local headers and the standard headers actually needed."""
import pathlib
import re

root = pathlib.Path(__file__).resolve().parents[3]
ui = root / "handheld/ui"
standard = {"stdbool.h", "stddef.h", "stdio.h", "string.h"}
for path in sorted(ui.iterdir()):
    if path.suffix not in {".h", ".c"}:
        continue
    text = path.read_text()
    for line in text.splitlines():
        if not re.match(r"\s*#\s*include\b", line):
            continue
        match = re.fullmatch(r'\s*#\s*include\s*[<"]([^>"]+)[>"]\s*', line)
        assert match, (path, "unrecognized include", line)
        header = match[1]
        assert header in standard or (
            header.startswith("hh_quick_menu") and "/" not in header
            and (ui / header).is_file()
        ), (path, "non-UI include", header)
    assert not re.search(
        r"\b(?:hh_runtime_\w*|command_event\w*|runloop_\w*|retroarch_\w*|"
        r"menu_driver_\w*|input_driver_\w*|video_driver_\w*|gfx_widgets?\w*|"
        r"fopen|freopen|opendir|system|popen)\b", text
    ), (path, "runtime/control or filesystem access")
print("PASS: UI header allowlist and runtime/control/filesystem boundary")
