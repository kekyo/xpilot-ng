#!/bin/sh

set -eu

usage()
{
    cat <<'EOF'
Usage: ./build.sh [OPTIONS] [-- CONFIGURE_OPTIONS...]

Build the pinned vendored SDL3 dependencies and XPilot NG in one command.
Build products are kept out of the source tree under ./build by default.

Options:
  --build-root PATH        Root for all build output (default: ./build)
  --jobs NUMBER            Parallel build jobs (default: detected CPU count)
  --build-type TYPE        SDL3 CMake build type (default: Release)
  --toolchain-file PATH    CMake toolchain file for cross-compilation
  --help                   Show this help

Arguments after -- are forwarded to XPilot NG's configure script.  The
vendored SDL3 provider and its private prefix are always selected by this
wrapper.  The script builds but does not install XPilot NG.
EOF
}

fail()
{
    echo "build.sh: $*" >&2
    exit 1
}

source_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
invocation_dir=$(pwd)
build_root="$source_dir/build"
jobs=
build_type=Release
toolchain_file=

while test "$#" -gt 0; do
    case "$1" in
        --build-root)
            test "$#" -ge 2 || fail "--build-root requires a path"
            build_root=$2
            shift 2
            ;;
        --build-root=*)
            build_root=${1#*=}
            shift
            ;;
        --jobs)
            test "$#" -ge 2 || fail "--jobs requires a number"
            jobs=$2
            shift 2
            ;;
        --jobs=*)
            jobs=${1#*=}
            shift
            ;;
        --build-type)
            test "$#" -ge 2 || fail "--build-type requires a value"
            build_type=$2
            shift 2
            ;;
        --build-type=*)
            build_type=${1#*=}
            shift
            ;;
        --toolchain-file)
            test "$#" -ge 2 || fail "--toolchain-file requires a path"
            toolchain_file=$2
            shift 2
            ;;
        --toolchain-file=*)
            toolchain_file=${1#*=}
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        --)
            shift
            break
            ;;
        *)
            fail "unknown option: $1 (put configure options after --)"
            ;;
    esac
done

test -n "$build_root" || fail "--build-root requires a nonempty path"
test -n "$build_type" || fail "--build-type requires a nonempty value"

if test -z "$jobs"; then
    jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
fi
case "$jobs" in
    ''|0|*[!0-9]*) fail "--jobs must be a positive integer" ;;
esac

case "$build_root" in
    /*) ;;
    *) build_root="$invocation_dir/$build_root" ;;
esac
test "$build_root" != / || fail "refusing to use / as the build root"
mkdir -p "$build_root"
build_root=$(CDPATH= cd -- "$build_root" && pwd)

if test -n "$toolchain_file"; then
    case "$toolchain_file" in
        /*) ;;
        *) toolchain_file="$invocation_dir/$toolchain_file" ;;
    esac
    test -f "$toolchain_file" \
        || fail "toolchain file not found: $toolchain_file"
    toolchain_dir=$(CDPATH= cd -- "$(dirname -- "$toolchain_file")" && pwd)
    toolchain_file="$toolchain_dir/$(basename -- "$toolchain_file")"
fi

vendor_builder="$source_dir/vendor/sdl3/build.sh"
test -x "$vendor_builder" \
    || fail "vendored SDL3 builder is unavailable: $vendor_builder"
test -x "$source_dir/configure" \
    || fail "configure is unavailable; run ./bootstrap first"
test ! -f "$source_dir/config.status" \
    || fail "source tree is configured in place; run make distclean first"

make_program=${MAKE:-make}
command -v "$make_program" >/dev/null 2>&1 \
    || fail "make was not found: $make_program"

vendor_build_dir="$build_root/vendor-sdl3"
vendor_prefix="$build_root/vendor-sdl3-prefix"
xpilot_build_dir="$build_root/xpilot-ng"

echo "===== build: vendored SDL3 dependencies ====="
if test -n "$toolchain_file"; then
    "$vendor_builder" \
        --build-dir "$vendor_build_dir" \
        --prefix "$vendor_prefix" \
        --jobs "$jobs" \
        --build-type "$build_type" \
        --toolchain-file "$toolchain_file"
else
    "$vendor_builder" \
        --build-dir "$vendor_build_dir" \
        --prefix "$vendor_prefix" \
        --jobs "$jobs" \
        --build-type "$build_type"
fi

mkdir -p "$xpilot_build_dir"
echo "===== configure: XPilot NG with vendored SDL3 ====="
(
    cd "$xpilot_build_dir"
    "$source_dir/configure" "$@" \
        --with-sdl3=vendored \
        "--with-sdl3-prefix=$vendor_prefix"
    "$make_program" "-j$jobs"
)

echo "XPilot NG build completed in $xpilot_build_dir"
