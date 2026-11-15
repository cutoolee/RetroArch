#!/usr/bin/env python3
"""Check that the renderer, Android installer and release assets agree."""

from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
widgets = (root / "gfx/gfx_widgets.c").read_text()
names = widgets.split('static const char *hh_widget_quick_menu_icon_name', 1)[1]
names = names.split('static const char *names[] = {', 1)[1].split('};', 1)[0]
icons = re.findall(r'"([^"]+\.png)"', names)
installer = (root / "frontend/drivers/platform_unix.c").read_text()
installer = installer.split('static void android_seed_quick_menu_icons', 1)[1].split('};', 1)[0]
defaults = root / "pkg/android/phoenix/gamego-assets/gamego-defaults"
for icon in icons:
    assert (defaults / "icons" / icon).is_file(), "Missing bundled icon: " + icon
    assert '"' + icon + '"' in installer, "Missing first-launch installation: " + icon
render = (root / "handheld/ui/hh_quick_menu_render.c").read_text()
mapping = render.split('static const hh_quick_menu_icon_t icons[] = {', 1)[1].split('};', 1)[0]
main_icons = re.findall(r'HH_QUICK_MENU_ICON_[A-Z_]+', mapping)
assert len(main_icons) == 9 and len(set(main_icons)) == 9, "Main menu icons must be distinct"
for shader in (defaults / "shaders").rglob("*.glsl*"):
    for reference in re.findall(r'^shader\d+\s*=\s*"?([^"\n]+)', shader.read_text(), re.MULTILINE):
        assert (shader.parent / reference.strip()).is_file(), "Missing shader: " + reference
print("PASS: all renderer icons are packaged and seeded; nine distinct main icons; GLSL references exist")
