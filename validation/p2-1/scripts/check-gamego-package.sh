#!/usr/bin/env sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
fail=0

check_text() {
   text=$1
   file=$2
   if grep -F "$text" "$file" >/dev/null 2>&1; then
      printf 'PASS package contract: %s\n' "$text"
   else
      printf 'FAIL package contract: %s (%s)\n' "$text" "${file#"$root"/}"
      fail=1
   fi
}

check_text 'applicationId "com.cutoolee.gamego"' \
   "$root/pkg/android/phoenix/build.gradle"
check_text 'namespace "com.retroarch"' \
   "$root/pkg/android/phoenix/build.gradle"
check_text 'ANDROID_PRODUCT_DATA_ROOT "GameGo"' \
   "$root/frontend/drivers/platform_unix.c"
check_text '${applicationId}.documents' \
   "$root/pkg/android/phoenix/AndroidManifest.xml"
check_text 'BuildConfig.APPLICATION_ID + ".QUERY_INSTALLED_CORES"' \
   "$root/pkg/android/phoenix-common/src/com/retroarch/browser/receiver/InstalledCoresReceiver.java"
check_text 'BuildConfig.APPLICATION_ID + ".USB_PERMISSION"' \
   "$root/pkg/android/phoenix-common/src/com/retroarch/browser/retroactivity/RetroActivityCommon.java"

if grep -F 'GAMEGO_PRODUCT cannot use P1B2_VALIDATION_APK' \
      "$root/pkg/android/phoenix/build.gradle" >/dev/null; then
   printf 'PASS package contract: P1B2 product guard\n'
else
   printf 'FAIL package contract: P1B2 product guard\n'
   fail=1
fi

printf 'PACKAGE_ID_GATE=%s\n' "$(test "$fail" -eq 0 && echo PASS || echo FAIL)"
printf 'DATA_ROOT_GATE=%s\n' "$(grep -F 'ANDROID_PRODUCT_DATA_ROOT "GameGo"' "$root/frontend/drivers/platform_unix.c" >/dev/null && echo PASS || echo FAIL)"
printf 'AUTHORITY_GATE=%s\n' "$(grep -F '${applicationId}.documents' "$root/pkg/android/phoenix/AndroidManifest.xml" >/dev/null && echo PASS || echo FAIL)"
exit "$fail"
