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
test -n "${XPILOT_WINDOWS_ARCHIVER:-}" \
    || fail "XPILOT_WINDOWS_ARCHIVER is not set"
test -f "$XPILOT_WINDOWS_ARCHIVER" \
    || fail "Windows archiver is unavailable: $XPILOT_WINDOWS_ARCHIVER"

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
fixture_artifacts="$wrapper_test_dir/artifacts"

mkdir -p "$fixture_source/vendor/sdl3/SDL/build-scripts" \
    "$fixture_source/vendor/mingw" "$fixture_source/config" \
    "$fixture_source/tests" \
    "$fixture_source/lib/maps" "$fixture_tools"
cp "$XPILOT_BUILD_WRAPPER" "$fixture_source/build.sh"
cp "$XPILOT_WINDOWS_ARCHIVER" "$fixture_source/config/package-windows.mjs"
chmod +x "$fixture_source/build.sh"
: > "$fixture_toolchain"
: > "$fixture_source/vendor/sdl3/SDL/build-scripts/cmake-toolchain-mingw64-i686.cmake"
: > "$fixture_source/vendor/sdl3/SDL/build-scripts/cmake-toolchain-mingw64-x86_64.cmake"
for data_file in defaults.txt password.txt robots.txt shipshapes.txt; do
    printf 'fixture data\n' > "$fixture_source/lib/$data_file"
done
printf 'fixture map\n' > "$fixture_source/lib/maps/ndh.xp2"
printf 'fixture license\n' > "$fixture_source/COPYING"

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

cat > "$fixture_source/vendor/mingw/build.sh" <<'EOF'
#!/bin/sh
set -eu

vendor_build_dir=
vendor_prefix=
for argument do
    printf 'mingw.arg=%s\n' "$argument" >> "$XPILOT_BUILD_TEST_LOG"
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
mkdir -p "$vendor_build_dir" "$vendor_prefix/include" \
    "$vendor_prefix/lib/pkgconfig"
: > "$vendor_prefix/lib/libz.a"
: > "$vendor_prefix/lib/libexpat.a"
: > "$vendor_prefix/lib/pkgconfig/sdl3.pc"
: > "$vendor_prefix/lib/pkgconfig/sdl3-image.pc"
: > "$vendor_prefix/lib/pkgconfig/sdl3-ttf.pc"
EOF

cat > "$fixture_source/configure" <<'EOF'
#!/bin/sh
set -eu

printf 'configure.cwd=%s\n' "$(pwd)" >> "$XPILOT_BUILD_TEST_LOG"
printf 'configure.cc=%s\n' "${CC:-}" >> "$XPILOT_BUILD_TEST_LOG"
printf 'configure.cppflags=%s\n' "${CPPFLAGS:-}" >> "$XPILOT_BUILD_TEST_LOG"
printf 'configure.ldflags=%s\n' "${LDFLAGS:-}" >> "$XPILOT_BUILD_TEST_LOG"
printf 'configure.pkg_config_libdir=%s\n' "${PKG_CONFIG_LIBDIR:-}" \
    >> "$XPILOT_BUILD_TEST_LOG"
printf 'configure.gl_cflags=%s\n' "${GL_CFLAGS:-}" \
    >> "$XPILOT_BUILD_TEST_LOG"
printf 'configure.gl_libs=%s\n' "${GL_LIBS:-}" \
    >> "$XPILOT_BUILD_TEST_LOG"
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
case "$(pwd)" in
    */output/windows/*/xpilot-infinity)
        mkdir -p src/server src/client/sdl tests
        : > src/server/xpilot-infinity-server.exe
        : > src/client/sdl/xpilot-infinity-sdl.exe
        : > tests/test-framed-stream.exe
        : > tests/test-game-transport.exe
        : > tests/test-socket-io.exe
        : > tests/test-sdl-versions.exe
        ;;
    */windows/x86|*/windows/x86_64)
        case " $* " in
            *" windows-package "*)
                mkdir -p package/lib/maps
                : > package/xpilot-infinity-server.exe
                : > package/xpilot-infinity-sdl.exe
                : > package/lib/maps/ndh.xp2
                ;;
        esac
        ;;
esac
EOF

cat > "$fixture_source/tests/run-wine-suite.sh" <<'EOF'
#!/bin/sh
set -eu

for argument do
    printf 'wine.arg=%s\n' "$argument" >> "$XPILOT_BUILD_TEST_LOG"
done
EOF

chmod +x "$fixture_source/vendor/sdl3/build.sh" \
    "$fixture_source/vendor/mingw/build.sh" \
    "$fixture_source/tests/run-wine-suite.sh" \
    "$fixture_source/configure" "$fixture_tools/make"

XPILOT_BUILD_TEST_LOG=$fixture_log \
PATH="$fixture_tools:$PATH" \
    "$fixture_source/build.sh" \
    --target native \
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

grep -Fx "configure.cwd=$fixture_output/xpilot-infinity" \
    "$fixture_log" >/dev/null \
    || fail "XPilot Infinity was not configured out of tree"
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
grep -Fx "make.cwd=$fixture_output/xpilot-infinity" "$fixture_log" >/dev/null \
    || fail "XPilot Infinity was not built out of tree"
grep -Fx "make.arg=-j3" "$fixture_log" >/dev/null \
    || fail "parallel build count was not passed to make"

: > "$fixture_log"
default_output="$wrapper_test_dir/default-output"
XPILOT_BUILD_TEST_LOG=$fixture_log \
PATH="$fixture_tools:$PATH" \
    "$fixture_source/build.sh" \
    --build-root "$default_output" \
    --package-version 4.7.99 \
    --jobs 1

for target_build_dir in \
    "$default_output/xpilot-infinity" \
    "$default_output/windows/x86" \
    "$default_output/windows/x86_64"
do
    test "$(grep -Fxc "configure.cwd=$target_build_dir" "$fixture_log")" -eq 1 \
        || fail "default build did not configure every target exactly once: $target_build_dir"
done
for architecture in x86 x86_64; do
    test -f "$fixture_source/artifacts/windows/xpilot-infinity-4.7.99-windows-$architecture.zip" \
        || fail "default build did not create the $architecture archive"
done

: > "$fixture_log"
XPILOT_BUILD_TEST_LOG=$fixture_log \
PATH="$fixture_tools:$PATH" \
    "$fixture_source/build.sh" \
    --target windows \
    --arch all \
    --test \
    --build-root "$fixture_output" \
    --artifact-root "$fixture_artifacts" \
    --package-version 4.7.99 \
    --jobs 2

for architecture in x86 x86_64; do
    case "$architecture" in
        x86)
            triplet=i686-w64-mingw32
            ;;
        x86_64)
            triplet=x86_64-w64-mingw32
            ;;
    esac
    architecture_root="$fixture_output/windows/$architecture"

    if grep -q '^mingw\.arg=' "$fixture_log"; then
        fail "$architecture dependencies were built outside make"
    fi
    grep -Fx "configure.cwd=$architecture_root" \
        "$fixture_log" >/dev/null \
        || fail "$architecture build was not configured out of tree"
    grep -Fx 'configure.cc=' "$fixture_log" >/dev/null \
        || fail "$architecture compiler leaked from the wrapper"
    grep -Fx 'configure.cppflags=' "$fixture_log" >/dev/null \
        || fail "$architecture include flags leaked from the wrapper"
    grep -Fx 'configure.ldflags=' "$fixture_log" >/dev/null \
        || fail "$architecture linker flags leaked from the wrapper"
    grep -Fx 'configure.pkg_config_libdir=' "$fixture_log" >/dev/null \
        || fail "$architecture pkg-config path leaked from the wrapper"
    grep -Fx 'configure.gl_cflags=' "$fixture_log" >/dev/null \
        || fail "$architecture OpenGL flags leaked from the wrapper"
    grep -Fx 'configure.gl_libs=' "$fixture_log" >/dev/null \
        || fail "$architecture OpenGL libraries leaked from the wrapper"
    grep -Fx "configure.arg=--host=$triplet" "$fixture_log" >/dev/null \
        || fail "$architecture host triplet was not configured"
    grep -Fx 'configure.arg=--enable-mingw-vendored-deps' \
        "$fixture_log" >/dev/null \
        || fail "$architecture vendored dependencies were not configured"
    grep -Fx 'configure.arg=--with-mingw-deps-build-type=Release' \
        "$fixture_log" >/dev/null \
        || fail "$architecture dependency build type was not configured"

    grep -Fx "make.cwd=$architecture_root" "$fixture_log" >/dev/null \
        || fail "$architecture was not built through its configured tree"
    grep -Fx 'make.arg=-j2' "$fixture_log" >/dev/null \
        || fail "$architecture parallel build was not requested"
    grep -Fx 'make.arg=windows-package' "$fixture_log" >/dev/null \
        || fail "$architecture package target was not requested"
    grep -Fx 'make.arg=check' "$fixture_log" >/dev/null \
        || fail "$architecture check target was not requested"

    test -f "$architecture_root/package/xpilot-infinity-server.exe" \
        || fail "$architecture server was not packaged"
    test -f "$architecture_root/package/xpilot-infinity-sdl.exe" \
        || fail "$architecture SDL client was not packaged"
    test -f "$architecture_root/package/lib/maps/ndh.xp2" \
        || fail "$architecture game data was not packaged"
    test -f "$fixture_artifacts/xpilot-infinity-4.7.99-windows-$architecture.zip" \
        || fail "$architecture distribution archive was not created"
done

if grep -q '^wine\.arg=' "$fixture_log"; then
    fail "Wine tests were run outside make check"
fi

if "$fixture_source/build.sh" --jobs 0 \
    --build-root "$wrapper_test_dir/invalid" >/dev/null 2>&1; then
    fail "zero parallel jobs were accepted"
fi

echo "Vendored SDL3 build wrapper smoke passed"
