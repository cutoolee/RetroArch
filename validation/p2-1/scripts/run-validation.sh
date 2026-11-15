#!/usr/bin/env sh
set -u

root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
if [ "$#" -gt 0 ]; then
   output=$1
else
   output="$root/validation/p2-1/results/latest"
fi
mkdir -p "$output"

status=0
contract_status=NOT_RUN
ui_status=NOT_RUN
bridge_status=NOT_RUN
feature_off_status=NOT_RUN

run_case() {
   name=$1
   shift
   log="$output/$name.log"
   if "$@" >"$log" 2>&1; then
      printf 'PASS %s (%s)\n' "$name" "$log"
      return 0
   fi
   printf 'FAIL %s (%s)\n' "$name" "$log"
   status=1
   return 1
}

if run_case contract "$root/validation/p2-1/scripts/check-evidence.sh"; then
   contract_status=PASS
else
   contract_status=FAIL
fi

if run_case quick-menu "$root/validation/p2/tests/test_quick_menu.sh"; then
   ui_status=PASS
else
   ui_status=FAIL
fi

if run_case bridge "$root/validation/p2.1/tests/test_bridge.sh"; then
   bridge_status=PASS
else
   bridge_status=FAIL
fi

environment_log="$output/android-environment.log"
{
   printf 'java: '
   command -v java 2>/dev/null || printf 'MISSING\n'
   printf 'adb: '
   command -v adb 2>/dev/null || printf 'MISSING\n'
   printf 'ANDROID_SDK_ROOT/ANDROID_HOME: '
   if [ -n "${ANDROID_SDK_ROOT:-}" ]; then
      printf '%s\n' "$ANDROID_SDK_ROOT"
   elif [ -n "${ANDROID_HOME:-}" ]; then
      printf '%s\n' "$ANDROID_HOME"
   else
      printf 'MISSING\n'
   fi
} >"$environment_log"

if command -v java >/dev/null 2>&1 \
   && java -version >/dev/null 2>&1 \
   && [ -x "$root/pkg/android/phoenix/gradlew" ]; then
   if run_case feature-off \
      sh -c "cd '$root/pkg/android/phoenix' && ./gradlew tasks --no-daemon -PHAVE_HANDHELD_RUNTIME=0 -PHAVE_HANDHELD_QUICK_MENU=0"; then
      feature_off_status=PASS
   else
      feature_off_status=FAIL
   fi
else
   printf 'NOT_RUN feature-off: Java/Gradle prerequisites unavailable\n'
   feature_off_status=NOT_RUN
fi

summary="$output/summary.yaml"
{
   printf 'schema: gamego-p2.1-validation-run/v1\n'
   printf 'completion_contract:\n'
   printf '  save: HH_EVENT_STATE_SAVE_COMPLETED\n'
   printf '  load: HH_EVENT_STATE_LOAD_COMPLETED\n'
   printf 'contract_gate: %s\n' "$contract_status"
   printf 'quick_menu_regression: %s\n' "$ui_status"
   printf 'bridge_lifecycle: %s\n' "$bridge_status"
   printf 'feature_off_android_configuration: %s\n' "$feature_off_status"
   printf 'device: NOT_RUN\n'
} >"$summary"
printf 'Summary: %s\n' "$summary"
exit "$status"
