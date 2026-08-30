#!/bin/sh

set -eu

fail()
{
    echo "run-windows-archive.sh: $*" >&2
    exit 1
}

test -n "${XPILOT_WINDOWS_ARCHIVER:-}" \
    || fail "XPILOT_WINDOWS_ARCHIVER is not set"
test -f "$XPILOT_WINDOWS_ARCHIVER" \
    || fail "Windows archiver is unavailable: $XPILOT_WINDOWS_ARCHIVER"
command -v node >/dev/null 2>&1 \
    || fail "Node.js is required to test Windows archive generation"
command -v zip >/dev/null 2>&1 \
    || fail "zip is required to test Windows archive generation"

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec node "$script_dir/test-windows-archive.mjs" "$XPILOT_WINDOWS_ARCHIVER"
