#!/usr/bin/env bash
set -euo pipefail

runtime_value="$1"
apk_path="$2"
manifest_path="$3"
link_evidence_path="$4"
build_log_path="$5"

case "$runtime_value" in
  0|1) ;;
  *) echo "runtime flag must be 0 or 1" >&2; exit 2 ;;
esac

test -f "$apk_path"
test -f "$link_evidence_path"
test -f "$build_log_path"

sha256="$(sha256sum "$apk_path" | awk '{print $1}')"
size="$(wc -c < "$apk_path" | tr -d ' ')"
commit="$(git rev-parse HEAD)"
timestamp="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

python3 - "$runtime_value" "$apk_path" "$manifest_path" "$sha256" "$size" "$commit" "$timestamp" "$link_evidence_path" <<'PY'
import json
import sys

runtime, apk, output, sha256, size, commit, timestamp, evidence = sys.argv[1:]
data = {
    "commit": commit,
    "runtime": runtime == "1",
    "feature_flags": {"HAVE_HANDHELD_RUNTIME": int(runtime)},
    "apk": apk,
    "size_bytes": int(size),
    "sha256": sha256,
    "build_timestamp_utc": timestamp,
    "runtime_link_evidence": evidence,
    "upstream_base": "RetroArch Android ndk-build via pkg/android/phoenix-common/jni/Android.mk",
}
with open(output, "w") as stream:
    json.dump(data, stream, indent=2, sort_keys=True)
    stream.write("\n")
PY

echo "APK=$apk_path" >> "$link_evidence_path"
echo "SHA256=$sha256" >> "$link_evidence_path"
echo "COMMIT=$commit" >> "$link_evidence_path"
