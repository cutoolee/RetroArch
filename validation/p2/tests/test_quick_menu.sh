#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../../.." && pwd)"
build="$(mktemp -d "${TMPDIR:-/tmp}/hh-ui-test.XXXXXX")"
trap 'rm -rf "$build"' EXIT
cd "$root"
python3 validation/p2/tests/check_boundary.py
compiler="${CC:-cc}"
flags=(-std=c99 -pedantic-errors -Wall -Wextra -Werror -Wdeclaration-after-statement)
# P2.0 uses stdbool.h; retain its ABI while checking all other C89 syntax.
if "$compiler" --version | grep -qi clang; then
  flags=(-std=c89 -pedantic-errors -Wno-c99-extensions -Wall -Wextra -Werror -Wdeclaration-after-statement)
fi
if [ "${SANITIZE:-0}" = 1 ]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
"$compiler" "${flags[@]}" -Ihandheld/ui handheld/ui/*.c \
  validation/p2/tests/test_quick_menu.c -o "$build/test_quick_menu"
"$build/test_quick_menu"
if [ "$#" -gt 0 ]; then
  mkdir -p "$1"
  "$compiler" "${flags[@]}" -Ihandheld/ui handheld/ui/*.c \
    validation/p2/tests/demo_quick_menu.c -o "$build/demo_quick_menu"
  "$build/demo_quick_menu" "$1"
fi
