#!/usr/bin/env bash
set -euo pipefail

failed=0
require_command() {
  local label="$1"
  local command_name="$2"
  if command -v "$command_name" >/dev/null 2>&1; then
    echo "PASS $label: $(command -v "$command_name")"
  else
    echo "MISSING $label: $command_name"
    failed=1
  fi
}

if command -v java >/dev/null 2>&1 && java -version >/dev/null 2>&1; then
  echo "PASS Java Runtime: $(command -v java)"
else
  echo "MISSING Java Runtime: java is absent or cannot start a JVM"
  failed=1
fi

require_command "adb" adb
require_command "Gradle Wrapper" "${1:-./gradlew}"

sdk_root="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
if [ -z "$sdk_root" ] || [ ! -d "$sdk_root" ]; then
  echo "MISSING Android SDK: set ANDROID_SDK_ROOT or ANDROID_HOME"
  failed=1
else
  echo "PASS Android SDK: $sdk_root"
  if [ -x "$sdk_root/platform-tools/adb" ]; then
    echo "PASS Android platform-tools"
  else
    echo "MISSING Android platform-tools"
    failed=1
  fi
  if find "$sdk_root/build-tools" -mindepth 1 -maxdepth 1 -type d -print -quit 2>/dev/null | grep -q .; then
    echo "PASS Android Build Tools"
  else
    echo "MISSING Android Build Tools"
    failed=1
  fi
  if find "$sdk_root/ndk" -mindepth 1 -maxdepth 1 -type d -print -quit 2>/dev/null | grep -q .; then
    echo "PASS Android NDK"
  else
    echo "MISSING Android NDK"
    failed=1
  fi
fi

exit "$failed"
