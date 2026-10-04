#!/usr/bin/env sh
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
build=$(mktemp -d "${TMPDIR:-/tmp}/hh-reset.XXXXXX")
trap 'rm -rf "$build"' EXIT
cc -std=c89 -Wall -Wextra -Werror -I"$root" -I"$root/handheld/ui" \
  "$root/handheld/ui/hh_quick_menu.c" \
  "$root/handheld/ui/hh_quick_menu_actions.c" \
  "$root/handheld/ui/hh_quick_menu_layout.c" \
  "$root/handheld/ui/hh_quick_menu_render.c" \
  "$root/handheld/ui/hh_quick_menu_state.c" \
  "$root/handheld/ui/hh_quick_menu_controls.c" \
  "$root/handheld/bridge/hh_bridge.c" "$root/validation/p2.1/tests/test_reset.c" \
  -o "$build/test_reset"
"$build/test_reset"
printf '%s\n' 'PASS: reset resumes before resetting and handles pending pause and command errors'
