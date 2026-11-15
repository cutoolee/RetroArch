#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../../.." && pwd)"
build="$(mktemp -d "${TMPDIR:-/tmp}/hh-recent-playlist.XXXXXX")"
trap 'rm -rf "$build"' EXIT
cd "$root"
platform_flags=(-D_GNU_SOURCE)
if [[ "$(uname -s)" == Darwin ]]; then platform_flags=(-D_DARWIN_C_SOURCE); fi
link_flags=(-Wl,--gc-sections)
if [[ "$(uname -s)" == Darwin ]]; then
  link_flags=(-Wl,-dead_strip)
fi
cc -std=c99 "${platform_flags[@]}" -ffunction-sections -I. -Ilibretro-common/include \
  validation/p2.1/tests/test_recent_playlist.c handheld/runtime/hh_runtime_query.c \
  libretro-common/string/stdstring.c libretro-common/compat/compat_strl.c \
  libretro-common/compat/compat_posix_string.c "${link_flags[@]}" -o "$build/test_recent"
"$build/test_recent"
cc -std=c99 "${platform_flags[@]}" -DHAVE_SCREENSHOTS -ffunction-sections -I. -Ilibretro-common/include \
  validation/p2.1/tests/test_recent_capture.c handheld/runtime/hh_runtime_recent.c \
  libretro-common/file/file_path.c libretro-common/string/stdstring.c \
  libretro-common/compat/compat_strl.c libretro-common/compat/compat_posix_string.c \
  "${link_flags[@]}" -o "$build/test_capture"
"$build/test_capture"
