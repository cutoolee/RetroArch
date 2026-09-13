#!/usr/bin/env sh
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
build=$(mktemp -d "${TMPDIR:-/tmp}/hh-bridge.XXXXXX")
trap 'rm -rf "$build"' EXIT
cc -std=c99 -Wall -Wextra -Werror -I"$root" -I"$root/handheld/ui" \
  "$root/handheld/ui/hh_quick_menu.c" \
  "$root/handheld/ui/hh_quick_menu_actions.c" \
  "$root/handheld/ui/hh_quick_menu_layout.c" \
  "$root/handheld/ui/hh_quick_menu_render.c" \
  "$root/handheld/ui/hh_quick_menu_state.c" \
  "$root/handheld/bridge/hh_bridge.c" "$root/validation/p2.1/tests/test_bridge.c" \
  -o "$build/test_bridge"
"$build/test_bridge"
printf '%s\n' 'PASS: bridge accepted/completed mapping and success feedback'
