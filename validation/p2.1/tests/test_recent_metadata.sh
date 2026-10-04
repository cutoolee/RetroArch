#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../../.." && pwd)"
build="$(mktemp -d "${TMPDIR:-/tmp}/hh-metadata.XXXXXX")"
trap 'rm -rf "$build"' EXIT
cd "$root"
flags=(-std=c99 -Wall -Wextra -Werror -Wdeclaration-after-statement -ffunction-sections)
compiler="${CC:-cc}"
if "$compiler" --version | rg -qi clang; then
  flags=(-std=c89 -pedantic-errors -Wno-c99-extensions -Wall -Wextra -Werror -Wdeclaration-after-statement -ffunction-sections)
fi
link_flags=()
if [[ "$(uname -s)" == Darwin ]]; then
  link_flags=(-Wl,-dead_strip -framework CoreFoundation)
else
  link_flags=(-Wl,--gc-sections -lm)
fi
if [[ "${SANITIZE:-0}" == 1 ]]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  link_flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
# Existing libretro-common sources have platform-specific warning policies.
"$compiler" "${flags[@]}" -I. -Ilibretro-common/include -c \
  handheld/runtime/hh_runtime_metadata.c -o "$build/metadata.o"
"$compiler" -std=c99 -ffunction-sections -I. -Ilibretro-common/include \
  "$build/metadata.o" \
  validation/p2.1/tests/test_recent_metadata.c \
  libretro-common/file/file_path.c libretro-common/file/file_path_io.c \
  libretro-common/lists/dir_list.c libretro-common/lists/string_list.c \
  libretro-common/file/retro_dirent.c \
  libretro-common/streams/file_stream.c libretro-common/vfs/vfs_implementation.c \
  libretro-common/string/stdstring.c libretro-common/compat/compat_strl.c \
  libretro-common/compat/compat_posix_string.c libretro-common/encodings/encoding_utf.c \
  libretro-common/time/rtime.c "${link_flags[@]}" -o "$build/test_metadata"
mkdir -p "$build/roms/media/拳皇94" "$build/roms/subdir" "$build/roms/media/mslug"
touch "$build/roms/media/拳皇94/boxfront.png" "$build/roms/custom.png"
touch "$build/roms/media/mslug/boxFront.jpg"
mkdir -p "$build/roms/media/orphan" "$build/roms/states"
touch "$build/roms/media/mslug/video.mp4" "$build/roms/media/orphan/video.mp4"
python3 - "$build/roms/states" <<'PY'
import os
import sys
directory = sys.argv[1]
for name, stamp in [('game.state', 100), ('game.state.auto', 200),
                    ('game.state12', 300), ('other.state', 400),
                    ('game.state.backup', 500)]:
    for suffix in ['', '.png']:
        path = os.path.join(directory, name + suffix)
        open(path, 'wb').close()
        os.utime(path, (stamp, stamp))
open(os.path.join(directory, 'game.state13.png'), 'wb').close()
PY
printf '\357\273\277collection: Arcade\r\ngame: 拳皇94\r\nfiles:\r\n  kof94.zip\r\n  kof94br.zip\r\n# comment\r\ngame: 恐龙快打\r\nfile: subdir/dino.zip\r\nassets.box_front: custom.png\r\ngame: 合金弹头\r\nfiles:\r\n  mslug.zip\r\n  msluga.zip' \
  > "$build/roms/metadata.pegasus.txt"
"$build/test_metadata" "$build/roms"
