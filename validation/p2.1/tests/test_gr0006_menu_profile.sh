#!/usr/bin/env sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
builtin="$root/input/input_autodetect_builtin.c"

grep -Fq '#define GR0006_DEFAULT_BINDS' "$builtin"
grep -Fq 'DECL_MENU(110)' "$builtin"
grep -Fq 'DECL_AUTOCONF_PID(2809, 1133, "android", GR0006_DEFAULT_BINDS)' "$builtin"
! grep -Fq 'DECL_MENU(316)' "$builtin"

grep -Fq 'BIT256_SET(current_bits, RARCH_MENU_TOGGLE)' "$root/runloop.c"
grep -Fq 'hh_bridge_global' "$root/runloop.c"
grep -Fq 'input_state_internal' "$root/input/input_driver.c"

printf '%s\n' 'PASS: GR0006 BUTTON_MODE profile maps Android keycode 110 to menu toggle'
