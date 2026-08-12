#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)

if test ! -x "$source_dir/configure"; then
    echo "Missing generated configure script; run autoreconf -fi first" >&2
    exit 1
fi

suite_dir=$(mktemp -d "${TMPDIR:-/tmp}/xpilot-sdl2-suite.XXXXXX")
build_source_dir=$source_dir

cleanup()
{
    case "$suite_dir" in
        "${TMPDIR:-/tmp}"/xpilot-sdl2-suite.*)
            rm -rf -- "$suite_dir"
            ;;
    esac
}

trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

# Autoconf rejects an out-of-tree configure when the source checkout already
# has an in-tree config.status.  Preserve that checkout and clean only a
# temporary source snapshot in this case.
if test -f "$source_dir/config.status"; then
    build_source_dir="$suite_dir/source"
    mkdir -p "$build_source_dir"
    cp -a "$source_dir/." "$build_source_dir/"
    # A copied in-tree Makefile retains the original absolute source path, so
    # invoking its distclean target can reconfigure the original checkout.
    # Remove copied build products directly inside the guarded temporary tree;
    # otherwise VPATH can mistake stale objects for out-of-tree prerequisites.
    rm -f -- "$build_source_dir/config.status" \
        "$build_source_dir/config.cache" "$build_source_dir/config.log" \
        "$build_source_dir/config.h" "$build_source_dir/stamp-h1" \
        "$build_source_dir/src/client/sdl/xpilot-ng-sdl" \
        "$build_source_dir/src/client/x11/xpilot-ng-x11" \
        "$build_source_dir/src/server/xpilot-ng-server" \
        "$build_source_dir/src/replay/xpilot-ng-replay" \
        "$build_source_dir/src/mapedit/xpilot-ng-xp-mapedit"
    find "$build_source_dir" -type f \
        \( -name Makefile -o -name '*.o' -o -name '*.a' \) -delete
    find "$build_source_dir" -depth -type d -name .deps -exec rm -rf -- {} \;
fi

if test -n "${XPILOT_TEST_JOBS:-}"; then
    test_jobs=$XPILOT_TEST_JOBS
else
    test_jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
fi

run_configuration()
{
    configuration_name=$1
    shift
    build_dir="$suite_dir/build-$configuration_name"
    install_prefix="$suite_dir/prefix-$configuration_name"
    mkdir -p "$build_dir" "$install_prefix"

    echo "===== configure: $configuration_name ====="
    (
        cd "$build_dir"
        "$build_source_dir/configure" --prefix="$install_prefix" "$@"
        make -j"$test_jobs"
        if ! make check; then
            for test_log in tests/*.log; do
                if test -f "$test_log"; then
                    echo "===== $test_log =====" >&2
                    sed -n '1,320p' "$test_log" >&2
                fi
            done
            exit 1
        fi
    )
}

# Every test is run through make check; the runner never selects an individual
# test binary.  Separate build and install trees also catch source-tree leaks.
run_configuration default
run_configuration sdl-only \
    --enable-sdl-client --disable-x11-client --disable-replay \
    --disable-xp-mapedit

echo "Both out-of-tree SDL2 configurations passed"
