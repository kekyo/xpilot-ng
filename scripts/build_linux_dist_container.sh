#!/bin/sh

set -eu

fail()
{
    echo "build_linux_dist_container.sh: $*" >&2
    exit 1
}

require_env()
{
    variable_name=$1
    eval "variable_value=\${$variable_name:-}"
    test -n "$variable_value" \
        || fail "required environment variable is missing: $variable_name"
}

require_command()
{
    command -v "$1" >/dev/null 2>&1 \
        || fail "required command was not found: $1"
}

assert_file()
{
    test -f "$1" || fail "missing expected file: $1"
}

validate_positive_integer()
{
    case $2 in
        ''|*[!0-9]*) fail "$1 must be a positive integer: $2" ;;
    esac
    test "$2" -gt 0 || fail "$1 must be a positive integer: $2"
}

calculate_runtime_dependencies()
{
    temporary_dir=$(mktemp -d)
    mkdir -p "$temporary_dir/debian"
    cat > "$temporary_dir/debian/control" <<EOF
Source: $XPILOT_PACKAGE_NAME
Section: games
Priority: optional
Maintainer: $XPILOT_PACKAGE_MAINTAINER
Standards-Version: 4.6.2

Package: $XPILOT_PACKAGE_NAME
Architecture: $deb_arch
Description: temporary metadata for dependency calculation
EOF

    dependencies=$(
        cd "$temporary_dir"
        dpkg-shlibdeps -O \
            "$stage_dir/usr/games/xpilot-ng-sdl" \
            "$stage_dir/usr/games/xpilot-ng-x11" \
            "$stage_dir/usr/games/xpilot-ng-server" \
            "$stage_dir/usr/games/xpilot-ng-replay" \
            "$stage_dir/usr/games/xpilot-ng-xp-mapedit" \
            | sed -n 's/^shlibs:Depends=//p'
    )
    rm -rf "$temporary_dir"
    test -n "$dependencies" \
        || fail "dpkg-shlibdeps did not calculate runtime dependencies"
    printf '%s\n' "$dependencies"
}

write_control_file()
{
    dependencies=$1
    control_dir="$stage_dir/DEBIAN"
    mkdir -p "$control_dir"
    cat > "$control_dir/control" <<EOF
Package: $XPILOT_PACKAGE_NAME
Version: $XPILOT_PACKAGE_VERSION
Section: games
Priority: optional
Architecture: $deb_arch
Maintainer: $XPILOT_PACKAGE_MAINTAINER
Depends: $dependencies
Description: $XPILOT_PACKAGE_DESCRIPTION
 XPilot NG is a multiplayer tactical game. This package includes the SDL
 and X11 clients, the dedicated server, utilities, game data, and manuals.
EOF
}

for variable_name in \
    XPILOT_WORK_DIR XPILOT_META_DIR \
    XPILOT_PACKAGE_VERSION XPILOT_PACKAGE_NAME XPILOT_PACKAGE_DESCRIPTION \
    XPILOT_PACKAGE_MAINTAINER XPILOT_BUILD_TYPE XPILOT_MAKE_JOBS
do
    require_env "$variable_name"
done

validate_positive_integer XPILOT_MAKE_JOBS "$XPILOT_MAKE_JOBS"
case $XPILOT_WORK_DIR in
    /workspace/artifacts/.tmp/*/work) ;;
    *) fail "unsafe work directory: $XPILOT_WORK_DIR" ;;
esac
case $XPILOT_META_DIR in
    /workspace/artifacts/.tmp/*/meta) ;;
    *) fail "unsafe metadata directory: $XPILOT_META_DIR" ;;
esac

require_command cmake
require_command dpkg-architecture
require_command dpkg-shlibdeps
require_command make
require_command pkg-config

test -x ./configure || fail "configure is unavailable; run ./bootstrap first"
test -x ./vendor/sdl3/build.sh \
    || fail "the vendored SDL3 build script is unavailable"
test ! -f ./config.status \
    || fail "the source tree must not be configured in place"

work_dir=$XPILOT_WORK_DIR
meta_dir=$XPILOT_META_DIR
dependency_build_dir="$work_dir/vendor-sdl3"
dependency_prefix="$work_dir/vendor-sdl3-prefix"
build_dir="$work_dir/xpilot-ng"
stage_dir="$work_dir/stage/$XPILOT_PACKAGE_NAME"

rm -rf "$work_dir" "$meta_dir"
mkdir -p "$build_dir" "$meta_dir" "$stage_dir"

./vendor/sdl3/build.sh \
    --build-dir "$dependency_build_dir" \
    --prefix "$dependency_prefix" \
    --jobs "$XPILOT_MAKE_JOBS" \
    --build-type "$XPILOT_BUILD_TYPE" \
    --disable-libusb

(
    cd "$build_dir"
    /workspace/configure \
        --prefix=/usr \
        --bindir=/usr/games \
        --datadir=/usr/share/games \
        --mandir=/usr/share/man \
        --disable-sound \
        --with-sdl3=vendored \
        "--with-sdl3-prefix=$dependency_prefix"
    make -j"$XPILOT_MAKE_JOBS"
    make install DESTDIR="$stage_dir"
)

for executable_name in \
    xpilot-ng-sdl xpilot-ng-x11 xpilot-ng-server \
    xpilot-ng-replay xpilot-ng-xp-mapedit
do
    assert_file "$stage_dir/usr/games/$executable_name"
done
assert_file "$stage_dir/usr/share/games/xpilot-ng/defaults.txt"
assert_file "$stage_dir/usr/share/games/xpilot-ng/maps/ndh.xp2"

doc_dir="$stage_dir/usr/share/doc/$XPILOT_PACKAGE_NAME"
mkdir -p "$doc_dir"
cp COPYING README INSTALL ChangeLog "$doc_dir/"

deb_arch=$(dpkg-architecture -qDEB_HOST_ARCH)
runtime_dependencies=$(calculate_runtime_dependencies)
write_control_file "$runtime_dependencies"

printf '%s\n' "$deb_arch" > "$meta_dir/deb_arch"
