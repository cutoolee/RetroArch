#!/usr/bin/env sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
plan="$root/validation/p2-1/test-plan.md"
results="$root/validation/p2-1/results.yaml"

fail=0
require_file() {
   file=$1
   if [ -f "$file" ]; then
      printf 'PASS file: %s\n' "${file#"$root"/}"
   else
      printf 'FAIL missing file: %s\n' "${file#"$root"/}"
      fail=1
   fi
}

require_text() {
   text=$1
   file=$2
   if grep -F "$text" "$file" >/dev/null 2>&1; then
      printf 'PASS contract: %s\n' "$text"
   else
      printf 'FAIL contract: %s (%s)\n' "$text" "${file#"$root"/}"
      fail=1
   fi
}

require_file "$plan"
require_file "$results"
for file in \
   "$root/validation/p2-1/build-evidence.md" \
   "$root/validation/p2-1/device-evidence.md" \
   "$root/validation/p2-1/runtime-evidence.md" \
   "$root/validation/p2-1/request-evidence.md" \
   "$root/validation/p2-1/crash-evidence.md"; do
   require_file "$file"
done

if [ -f "$plan" ]; then
   require_text 'SAVE_COMPLETION_EVENT=HH_EVENT_STATE_SAVE_COMPLETED' "$plan"
   require_text 'LOAD_COMPLETION_EVENT=HH_EVENT_STATE_LOAD_COMPLETED' "$plan"
   for text in MENU blocking Continue Save Load Reset Advanced Exit pause pending busy request_id crash ANR P1; do
      require_text "$text" "$plan"
   done
fi
if [ -f "$results" ]; then
   require_text 'HH_EVENT_STATE_SAVE_COMPLETED' "$results"
   require_text 'HH_EVENT_STATE_LOAD_COMPLETED' "$results"
fi

exit "$fail"
