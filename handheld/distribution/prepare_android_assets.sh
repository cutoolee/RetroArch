#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
lock_file="${RESOURCE_LOCK_FILE:-$script_dir/resources.lock}"
assets_dir="${ANDROID_ASSETS_DIR:-$repo_root/pkg/android/phoenix/assets}"
temp_root="${RUNNER_TEMP:-${TMPDIR:-/tmp}}"
work_dir=$(mktemp -d "$temp_root/retroarch-assets.XXXXXX")
archive="$work_dir/frontend-assets.tar.gz"
extract_dir="$work_dir/extracted"

read -r resource_name source_url expected_sha256 resource_version <<EOF
$(python3 - "$lock_file" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as lock:
    data = json.load(lock)

required = ("resource_name", "source_url", "expected_sha256", "version")
missing = [key for key in required if not data.get(key)]
if missing:
    raise SystemExit("resource lock missing: " + ", ".join(missing))

print(*(data[key] for key in required))
PY
)
EOF

if [ -e "$assets_dir" ]; then
   echo "Refusing to overwrite existing Android assets directory: $assets_dir" >&2
   echo "Set ANDROID_ASSETS_DIR to an empty staging path for local verification." >&2
   exit 1
fi

mkdir -p "$extract_dir"
curl --fail --location --retry 3 --proto '=https' --tlsv1.2 \
   --output "$archive" "$source_url"

printf '%s  %s\n' "$expected_sha256" "$archive" | shasum -a 256 -c -

tar -xzf "$archive" -C "$extract_dir"
root_count=$(find "$extract_dir" -mindepth 1 -maxdepth 1 -type d | wc -l | tr -d ' ')
if [ "$root_count" -ne 1 ]; then
   echo "Expected one frontend archive root, found $root_count" >&2
   exit 1
fi
frontend_root=$(find "$extract_dir" -mindepth 1 -maxdepth 1 -type d -print -quit)

required_files=(
   "ozone/regular.ttf"
   "ozone/bold.ttf"
   "pkg/chinese-fallback-font.ttf"
)
for relative_path in "${required_files[@]}"; do
   if [ ! -f "$frontend_root/$relative_path" ]; then
      echo "Missing required frontend asset: $relative_path" >&2
      exit 1
   fi
done
if [ ! -d "$frontend_root/ozone/png/icons" ]; then
   echo "Missing required Ozone icon directory" >&2
   exit 1
fi

mkdir -p "$assets_dir/assets"
for entry in COPYING glui nxrgui ozone pkg rgui sounds switch xmb; do
   if [ -e "$frontend_root/$entry" ]; then
      cp -R "$frontend_root/$entry" "$assets_dir/assets/"
   fi
done

test -f "$assets_dir/assets/pkg/chinese-fallback-font.ttf"
test -f "$assets_dir/assets/ozone/regular.ttf"
test -f "$assets_dir/assets/ozone/bold.ttf"
test -d "$assets_dir/assets/ozone/png/icons"

lock_sha256=$(shasum -a 256 "$lock_file" | awk '{print $1}')
printf 'RESOURCE_NAME=%s\n' "$resource_name"
printf 'RESOURCE_VERSION=%s\n' "$resource_version"
printf 'RESOURCE_URL=%s\n' "$source_url"
printf 'RESOURCE_LOCK_SHA256=%s\n' "$lock_sha256"
printf 'FRONTEND_BUNDLE_SHA256=%s\n' "$expected_sha256"
printf 'ANDROID_ASSETS_DIR=%s\n' "$assets_dir"
printf 'CHINESE_FALLBACK_FONT=%s\n' "$assets_dir/assets/pkg/chinese-fallback-font.ttf"
printf 'OZONE_REGULAR_FONT=%s\n' "$assets_dir/assets/ozone/regular.ttf"
printf 'OZONE_BOLD_FONT=%s\n' "$assets_dir/assets/ozone/bold.ttf"
printf 'OZONE_ASSETS=%s\n' "$assets_dir/assets/ozone"
printf 'ANDROID_ASSETS_PREPARE=PASS\n'
