#!/bin/sh

set -eu

fail()
{
    echo "run-source-encoding.sh: $*" >&2
    exit 1
}

test -n "${XPILOT_SOURCE_ENCODING_ROOT:-}" \
    || fail "XPILOT_SOURCE_ENCODING_ROOT is not set"
test -d "$XPILOT_SOURCE_ENCODING_ROOT" \
    || fail "source root is unavailable: $XPILOT_SOURCE_ENCODING_ROOT"
command -v node >/dev/null 2>&1 \
    || fail "Node.js is required to test source encodings"

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec node "$script_dir/test-source-encoding.mjs" \
    "$XPILOT_SOURCE_ENCODING_ROOT"
