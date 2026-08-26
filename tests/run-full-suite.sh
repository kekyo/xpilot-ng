#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)

if test ! -x "$source_dir/configure"; then
    echo "Missing generated configure script; run autoreconf -fi first" >&2
    exit 1
fi

suite_dir=$(mktemp -d "${TMPDIR:-/tmp}/xpilot-sdl3-suite.XXXXXX")
build_source_dir=$source_dir

cleanup()
{
    case "$suite_dir" in
        "${TMPDIR:-/tmp}"/xpilot-sdl3-suite.*)
            rm -rf -- "$suite_dir"
            ;;
    esac
}

trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

# Autoconf rejects an out-of-tree configure when the source checkout already
# has an in-tree config.status.  A generated source-tree version.h can also
# hide a missing build-tree include path.  Preserve the checkout and exercise
# a temporary source snapshot without either artifact in both cases.
if test -f "$source_dir/config.status" \
    || test -f "$source_dir/src/common/version.h"; then
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
        "$build_source_dir/src/common/version.h" \
        "$build_source_dir/src/client/sdl/xpilot-infinity-sdl" \
        "$build_source_dir/src/client/x11/xpilot-infinity-x11" \
        "$build_source_dir/src/server/xpilot-infinity-server" \
        "$build_source_dir/src/replay/xpilot-infinity-replay" \
        "$build_source_dir/src/mapedit/xpilot-infinity-xp-mapedit"
    find "$build_source_dir" -type f \
        \( -name Makefile -o -name '*.o' -o -name '*.a' \) -delete
    find "$build_source_dir" -depth -type d -name .deps -exec rm -rf -- {} \;
fi

if test -n "${XPILOT_TEST_JOBS:-}"; then
    test_jobs=$XPILOT_TEST_JOBS
else
    test_jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
fi

vendor_build_dir="$suite_dir/vendor-sdl3-build"
vendor_prefix="$suite_dir/vendor-sdl3-prefix"

echo "===== build: vendored SDL3 dependencies ====="
"$build_source_dir/vendor/sdl3/build.sh" \
    --build-dir "$vendor_build_dir" \
    --prefix "$vendor_prefix" \
    --jobs "$test_jobs"

system_pkg_config_path="$vendor_prefix/lib/pkgconfig"
if test -n "${PKG_CONFIG_PATH:-}"; then
    system_pkg_config_path="$system_pkg_config_path:$PKG_CONFIG_PATH"
fi
system_sdl3_libs=$(PKG_CONFIG_PATH="$system_pkg_config_path" \
    pkg-config --static --libs sdl3)
system_sdl3_image_libs=$(PKG_CONFIG_PATH="$system_pkg_config_path" \
    pkg-config --static --libs sdl3-image)
system_sdl3_ttf_libs=$(PKG_CONFIG_PATH="$system_pkg_config_path" \
    pkg-config --static --libs sdl3-ttf)

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
        client_help=$(src/client/sdl/xpilot-infinity-sdl -help 2>&1 || true)
        printf '%s\n' "$client_help" | grep -Fq 'soundFile' \
            || { echo "default client sound options are unavailable" >&2; exit 1; }
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
# Both dependency modes exercise the complete target set and the SDL-only set.
PKG_CONFIG_PATH="$system_pkg_config_path" SDL3_LIBS="$system_sdl3_libs" \
    SDL3_IMAGE_LIBS="$system_sdl3_image_libs" \
    SDL3_TTF_LIBS="$system_sdl3_ttf_libs" \
    run_configuration system-default --with-sdl3=system
PKG_CONFIG_PATH="$system_pkg_config_path" SDL3_LIBS="$system_sdl3_libs" \
    SDL3_IMAGE_LIBS="$system_sdl3_image_libs" \
    SDL3_TTF_LIBS="$system_sdl3_ttf_libs" \
    run_configuration system-sdl-only --with-sdl3=system \
    --enable-sdl-client --disable-x11-client --disable-replay \
    --disable-xp-mapedit
run_configuration vendored-default \
    --with-sdl3=vendored --with-sdl3-prefix="$vendor_prefix"
run_configuration vendored-sdl-only \
    --with-sdl3=vendored --with-sdl3-prefix="$vendor_prefix" \
    --enable-sdl-client --disable-x11-client --disable-replay \
    --disable-xp-mapedit

echo "===== build and test: MinGW Windows targets ====="
"$build_source_dir/build.sh" \
    --target windows --arch all --test \
    --build-root "$suite_dir/windows" --jobs "$test_jobs"

echo "All native and MinGW out-of-tree configurations passed"
