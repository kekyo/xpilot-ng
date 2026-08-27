#!/bin/sh

set -eu

if test -z "${XPILOT_VENDOR_ZLIB_SOURCE:-}"; then
    echo "XPILOT_VENDOR_ZLIB_SOURCE is not set" >&2
    exit 1
fi

if test ! -x "$XPILOT_VENDOR_ZLIB_SOURCE/configure"; then
    echo "Vendored zlib source is unavailable: $XPILOT_VENDOR_ZLIB_SOURCE" >&2
    exit 1
fi

zlib_test_dir=$(mktemp -d "${TMPDIR:-/tmp}/xpilot-vendor-zlib-test.XXXXXX")

cleanup()
{
    case "$zlib_test_dir" in
        "${TMPDIR:-/tmp}"/xpilot-vendor-zlib-test.*)
            rm -rf -- "$zlib_test_dir"
            ;;
    esac
}

trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

mkdir -p "$zlib_test_dir/source"
cp -a "$XPILOT_VENDOR_ZLIB_SOURCE/." "$zlib_test_dir/source/"

if test -n "${XPILOT_TEST_JOBS:-}"; then
    test_jobs=$XPILOT_TEST_JOBS
else
    test_jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
fi

cd "$zlib_test_dir/source"
./configure --static
make -j"$test_jobs" teststatic
