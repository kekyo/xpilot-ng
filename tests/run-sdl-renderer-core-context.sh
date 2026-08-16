#!/bin/sh

set -eu

if test "${1:-}" != --inside-xvfb; then
    for command_name in xvfb-run xauth; do
        if ! command -v "$command_name" >/dev/null 2>&1; then
            echo "Missing OpenGL test dependency: $command_name" >&2
            exit 1
        fi
    done

    export LIBGL_ALWAYS_SOFTWARE=1
    export SDL_VIDEODRIVER=x11
    exec xvfb-run -a \
        -s "-screen 0 1280x1024x24 +extension GLX +render -noreset" \
        /bin/sh "$0" --inside-xvfb
fi

test_binary=${XPILOT_SDL_CORE_CONTEXT_TEST_BINARY:?
XPILOT_SDL_CORE_CONTEXT_TEST_BINARY is required}
if test ! -x "$test_binary"; then
    echo "SDL core context test binary is missing: $test_binary" >&2
    exit 1
fi

exec "$test_binary"
