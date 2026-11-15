#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 3 ]; then
   echo "usage: $0 APK RESOURCE_LOCK EVIDENCE" >&2
   exit 2
fi

apk=$1
lock_file=$2
evidence=$3

test -f "$apk"
test -f "$lock_file"

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

apk_entries=$(mktemp "${TMPDIR:-/tmp}/retroarch-apk-entries.XXXXXX")
unzip -Z1 "$apk" > "$apk_entries"

required_entries=(
   "assets/assets/pkg/chinese-fallback-font.ttf"
   "assets/assets/ozone/regular.ttf"
   "assets/assets/ozone/bold.ttf"
)
for entry in "${required_entries[@]}"; do
   if ! grep -Fqx "$entry" "$apk_entries"; then
      echo "Missing required APK asset: $entry" >&2
      exit 1
   fi
done

if ! grep -Eq '^assets/' "$apk_entries"; then
   echo "APK has no assets/ directory" >&2
   exit 1
fi
if ! grep -Eq '^assets/assets/ozone/png/icons/[^/]+$' "$apk_entries"; then
   echo "APK has no Ozone icon asset" >&2
   exit 1
fi

apk_sha256=$(shasum -a 256 "$apk" | awk '{print $1}')
lock_sha256=$(shasum -a 256 "$lock_file" | awk '{print $1}')

{
   printf 'APK_FILENAME=%s\n' "$(basename "$apk")"
   printf 'APK_SHA256=%s\n' "$apk_sha256"
   printf 'RESOURCE_NAME=%s\n' "$resource_name"
   printf 'RESOURCE_VERSION=%s\n' "$resource_version"
   printf 'RESOURCE_URL=%s\n' "$source_url"
   printf 'RESOURCE_LOCK_SHA256=%s\n' "$lock_sha256"
   printf 'FRONTEND_BUNDLE_SHA256=%s\n' "$expected_sha256"
   printf 'FRONTEND_ASSETS_PRESENT=PASS\n'
   printf 'CHINESE_FALLBACK_FONT=assets/assets/pkg/chinese-fallback-font.ttf\n'
   printf 'OZONE_REGULAR_FONT=assets/assets/ozone/regular.ttf\n'
   printf 'OZONE_BOLD_FONT=assets/assets/ozone/bold.ttf\n'
   printf 'OZONE_ASSETS=assets/assets/ozone\n'
   printf 'OZONE_ICONS=assets/assets/ozone/png/icons\n'
   printf 'ANDROID_DISTRIBUTION_PACKAGING_GATE=PASS\n'
} > "$evidence"

cat "$evidence"
