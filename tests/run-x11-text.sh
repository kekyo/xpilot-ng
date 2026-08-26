#!/bin/sh

set -eu

if test "${1:-}" != --inside-xvfb; then
    for command_name in xvfb-run xauth; do
        if ! command -v "$command_name" >/dev/null 2>&1; then
            echo "Missing X11 text test dependency: $command_name" >&2
            exit 1
        fi
    done
    exec xvfb-run -a \
        -s "-screen 0 640x480x24 +render -noreset" \
        /bin/sh "$0" --inside-xvfb
fi

test_binary=${XPILOT_X11_TEXT_TEST_BINARY:?
XPILOT_X11_TEXT_TEST_BINARY is required}
if test ! -x "$test_binary"; then
    echo "X11 text test binary is missing: $test_binary" >&2
    exit 1
fi

exec "$test_binary"
