#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../../.." && pwd)"
build="$(mktemp -d "${TMPDIR:-/tmp}/hh-resume.XXXXXX")"
trap 'rm -rf "$build"' EXIT
cd "$root"
link_flags=(-Wl,--gc-sections)
if [[ "$(uname -s)" == Darwin ]]; then
  link_flags=(-Wl,-dead_strip)
fi
cc -std=c89 -Wall -Wextra -ffunction-sections -I. -Ilibretro-common/include \
  validation/p2.1/tests/test_resume.c handheld/runtime/hh_runtime_command.c \
  "${link_flags[@]}" -o "$build/test_resume"
"$build/test_resume"
