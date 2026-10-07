#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../../.." && pwd)"
build="$(mktemp -d "${TMPDIR:-/tmp}/hh-controls-test.XXXXXX")"
trap 'rm -rf "$build"' EXIT
cd "$root"
cc -std=c89 -pedantic-errors -Wall -Wextra -Werror \
  -fsanitize=address,undefined -I. -Ilibretro-common/include \
  validation/controllers/tests/test_input_action.c -o "$build/test_input_action"
"$build/test_input_action"
link_flags=(-Wl,--gc-sections)
if [[ "$(uname -s)" == Darwin ]]; then
  link_flags=(-Wl,-dead_strip)
fi
cc -std=c89 -Wno-c99-extensions -ffunction-sections \
  -DHAVE_MENU -DHAVE_HANDHELD_RUNTIME=1 -DHAVE_HANDHELD_QUICK_MENU=1 \
  -fsanitize=address,undefined -I. -Ilibretro-common/include \
  validation/controllers/tests/test_menu_input.c input/input_driver.c \
  "${link_flags[@]}" -o "$build/test_menu_input"
"$build/test_menu_input"
cc -std=c89 -Wno-c99-extensions -DHAVE_CONFIGFILE -ffunction-sections \
  -fsanitize=address,undefined -I. -Ilibretro-common/include \
  validation/controllers/tests/test_controls_runtime.c handheld/runtime/hh_runtime_controls.c \
  libretro-common/file/file_path.c libretro-common/string/stdstring.c \
  libretro-common/compat/compat_strl.c libretro-common/compat/compat_posix_string.c \
  "${link_flags[@]}" -o "$build/test_controls_runtime"
"$build/test_controls_runtime"
for product in 0 1; do
  product_flags=(-UHAVE_GAMEGO_PRODUCT)
  if [[ "$product" == 1 ]]; then product_flags=(-DHAVE_GAMEGO_PRODUCT); fi
  cc -std=c89 -Wno-c99-extensions -ffunction-sections "${product_flags[@]}" \
    -fsanitize=address,undefined -I. -Ilibretro-common/include \
    validation/controllers/tests/test_core_defaults.c core_option_manager.c \
    libretro-common/lists/string_list.c libretro-common/lists/nested_list.c \
    libretro-common/string/stdstring.c libretro-common/file/config_file.c \
    libretro-common/file/file_path.c libretro-common/compat/compat_strl.c \
    libretro-common/compat/compat_posix_string.c "${link_flags[@]}" \
    -o "$build/test_core_defaults_$product"
  (cd "$build" && "./test_core_defaults_$product")
done
