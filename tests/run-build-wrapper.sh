#!/bin/sh

set -eu

fail()
{
    echo "run-build-wrapper.sh: $*" >&2
    exit 1
}

test -n "${XPILOT_BUILD_WRAPPER:-}" \
    || fail "XPILOT_BUILD_WRAPPER is not set"
test -x "$XPILOT_BUILD_WRAPPER" \
    || fail "build wrapper is unavailable: $XPILOT_BUILD_WRAPPER"

wrapper_test_dir=$(mktemp -d \
    "${TMPDIR:-/tmp}/xpilot-build-wrapper-test.XXXXXX")

cleanup()
{
    case "$wrapper_test_dir" in
        "${TMPDIR:-/tmp}"/xpilot-build-wrapper-test.*)
            rm -rf -- "$wrapper_test_dir"
            ;;
    esac
}

trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

fixture_source="$wrapper_test_dir/source"
fixture_tools="$wrapper_test_dir/tools"
fixture_output="$wrapper_test_dir/output"
fixture_log="$wrapper_test_dir/invocations.log"
fixture_toolchain="$wrapper_test_dir/toolchain.cmake"
fixture_install="$wrapper_test_dir/install"

mkdir -p "$fixture_source/vendor/sdl3" "$fixture_tools"
cp "$XPILOT_BUILD_WRAPPER" "$fixture_source/build.sh"
chmod +x "$fixture_source/build.sh"
: > "$fixture_toolchain"

cat > "$fixture_source/vendor/sdl3/build.sh" <<'EOF'
#!/bin/sh
set -eu

vendor_build_dir=
vendor_prefix=
for argument do
    printf 'vendor.arg=%s\n' "$argument" >> "$XPILOT_BUILD_TEST_LOG"
done
while test "$#" -gt 0; do
    case "$1" in
        --build-dir)
            vendor_build_dir=$2
            shift 2
            ;;
        --prefix)
            vendor_prefix=$2
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done
mkdir -p "$vendor_build_dir" "$vendor_prefix/lib/pkgconfig"
EOF

cat > "$fixture_source/configure" <<'EOF'
#!/bin/sh
set -eu

printf 'configure.cwd=%s\n' "$(pwd)" >> "$XPILOT_BUILD_TEST_LOG"
for argument do
    printf 'configure.arg=%s\n' "$argument" >> "$XPILOT_BUILD_TEST_LOG"
done
EOF

cat > "$fixture_tools/make" <<'EOF'
#!/bin/sh
set -eu

printf 'make.cwd=%s\n' "$(pwd)" >> "$XPILOT_BUILD_TEST_LOG"
for argument do
    printf 'make.arg=%s\n' "$argument" >> "$XPILOT_BUILD_TEST_LOG"
done
EOF

chmod +x "$fixture_source/vendor/sdl3/build.sh" \
    "$fixture_source/configure" "$fixture_tools/make"

XPILOT_BUILD_TEST_LOG=$fixture_log \
PATH="$fixture_tools:$PATH" \
    "$fixture_source/build.sh" \
    --build-root "$fixture_output" \
    --jobs 3 \
    --build-type Debug \
    --toolchain-file "$fixture_toolchain" \
    -- --disable-x11-client "--prefix=$fixture_install"

grep -Fx "vendor.arg=--build-dir" "$fixture_log" >/dev/null \
    || fail "vendored SDL3 build directory option was not passed"
grep -Fx "vendor.arg=$fixture_output/vendor-sdl3" \
    "$fixture_log" >/dev/null \
    || fail "vendored SDL3 build directory was incorrect"
grep -Fx "vendor.arg=$fixture_output/vendor-sdl3-prefix" \
    "$fixture_log" >/dev/null \
    || fail "vendored SDL3 prefix was incorrect"
grep -Fx "vendor.arg=3" "$fixture_log" >/dev/null \
    || fail "parallel build count was not passed"
grep -Fx "vendor.arg=Debug" "$fixture_log" >/dev/null \
    || fail "CMake build type was not passed"
grep -Fx "vendor.arg=$fixture_toolchain" "$fixture_log" >/dev/null \
    || fail "CMake toolchain was not passed"

grep -Fx "configure.cwd=$fixture_output/xpilot-ng" \
    "$fixture_log" >/dev/null \
    || fail "XPilot NG was not configured out of tree"
grep -Fx "configure.arg=--disable-x11-client" \
    "$fixture_log" >/dev/null \
    || fail "configure option was not forwarded"
grep -Fx "configure.arg=--prefix=$fixture_install" \
    "$fixture_log" >/dev/null \
    || fail "configure prefix was not forwarded"
grep -Fx "configure.arg=--with-sdl3=vendored" \
    "$fixture_log" >/dev/null \
    || fail "vendored SDL3 provider was not selected"
grep -Fx "configure.arg=--with-sdl3-prefix=$fixture_output/vendor-sdl3-prefix" \
    "$fixture_log" >/dev/null \
    || fail "vendored SDL3 prefix was not selected"
grep -Fx "make.cwd=$fixture_output/xpilot-ng" "$fixture_log" >/dev/null \
    || fail "XPilot NG was not built out of tree"
grep -Fx "make.arg=-j3" "$fixture_log" >/dev/null \
    || fail "parallel build count was not passed to make"

if "$fixture_source/build.sh" --jobs 0 \
    --build-root "$wrapper_test_dir/invalid" >/dev/null 2>&1; then
    fail "zero parallel jobs were accepted"
fi

echo "Vendored SDL3 build wrapper smoke passed"
