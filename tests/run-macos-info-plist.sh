#!/bin/sh

set -eu

fail()
{
    echo "run-macos-info-plist.sh: $*" >&2
    exit 1
}

test -n "${XPILOT_MACOS_INFO_PLIST:-}" \
    || fail "XPILOT_MACOS_INFO_PLIST is not set"
test -f "$XPILOT_MACOS_INFO_PLIST" \
    || fail "macOS InfoPlist.strings is unavailable: $XPILOT_MACOS_INFO_PLIST"
command -v node >/dev/null 2>&1 \
    || fail "Node.js is required to test the macOS bundle name"

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec node "$script_dir/test-macos-info-plist.mjs" "$XPILOT_MACOS_INFO_PLIST"
