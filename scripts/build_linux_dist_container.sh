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

assert_debian_dependency()
{
    dependency_list=$1
    expected_package=$2
    saved_ifs=$IFS
    IFS=,
    for dependency in $dependency_list; do
        dependency=${dependency# }
        case $dependency in
            "$expected_package"|"$expected_package "*)
                IFS=$saved_ifs
                return 0
                ;;
        esac
    done
    IFS=$saved_ifs
    fail "missing runtime dependency: $expected_package"
}

validate_positive_integer()
{
    case $2 in
        ''|*[!0-9]*) fail "$1 must be a positive integer: $2" ;;
    esac
    test "$2" -gt 0 || fail "$1 must be a positive integer: $2"
}

validate_copyright_file()
{
    copyright_path=$1
    assert_file "$copyright_path"
    if grep -F TODO "$copyright_path" >/dev/null 2>&1; then
        fail "copyright metadata still contains TODO entries: $copyright_path"
    fi
}

validate_debian_changelog()
{
    changelog_path=$1
    assert_file "$changelog_path"
    changelog_version=$(dpkg-parsechangelog \
        -l "$changelog_path" -S Version)
    test "$changelog_version" = "$XPILOT_PACKAGE_VERSION" \
        || fail "Debian changelog version $changelog_version does not match package version $XPILOT_PACKAGE_VERSION"
}

render_debian_changelog()
{
    changelog_template_path=$1
    changelog_output_path=$2
    version_placeholder=@XPILOT_PACKAGE_VERSION@

    assert_file "$changelog_template_path"
    grep -F "$version_placeholder" "$changelog_template_path" \
        >/dev/null 2>&1 \
        || fail "Debian changelog template has no version placeholder: $changelog_template_path"
    sed "s/$version_placeholder/$XPILOT_PACKAGE_VERSION/g" \
        "$changelog_template_path" > "$changelog_output_path"
    if grep -F "$version_placeholder" "$changelog_output_path" \
        >/dev/null 2>&1
    then
        fail "Debian changelog still contains a version placeholder: $changelog_output_path"
    fi
    validate_debian_changelog "$changelog_output_path"
}

strip_staged_executables()
{
    strip_command=${STRIP:-strip}
    for executable_name in \
        xpilot-infinity-sdl xpilot-infinity-x11 xpilot-infinity-server \
        xpilot-infinity-replay xpilot-infinity-xp-mapedit
    do
        executable_path="$stage_dir/usr/games/$executable_name"
        assert_file "$executable_path"
        "$strip_command" --strip-unneeded \
            --remove-section=.comment --remove-section=.note \
            "$executable_path"
    done
}

compress_manual_pages()
{
    manual_count=0
    for manual_path in "$stage_dir"/usr/share/man/man[1-9]/*.[1-9]; do
        test -f "$manual_path" || continue
        gzip -9n "$manual_path"
        manual_count=$((manual_count + 1))
    done
    test "$manual_count" -gt 0 \
        || fail "no manual pages were available to compress"
}

install_package_documentation()
{
    copyright_path="$source_dir/debian/copyright"
    debian_changelog_template_path="$source_dir/debian/changelog.in"
    debian_changelog_path="$meta_dir/changelog.Debian"
    validate_copyright_file "$copyright_path"

    package_doc_dir="$stage_dir/usr/share/doc/$XPILOT_PACKAGE_NAME"
    mkdir -p "$package_doc_dir" "$meta_dir"
    render_debian_changelog \
        "$debian_changelog_template_path" "$debian_changelog_path"
    cp "$source_dir/README.md" "$package_doc_dir/"
    cp "$copyright_path" "$package_doc_dir/copyright"
    gzip -9n -c "$source_dir/ChangeLog" \
        > "$package_doc_dir/changelog.gz"
    gzip -9n -c "$debian_changelog_path" \
        > "$package_doc_dir/changelog.Debian.gz"
}

prepare_debian_package_files()
{
    strip_staged_executables
    compress_manual_pages
    install_package_documentation
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
Standards-Version: 4.7.4

Package: $XPILOT_PACKAGE_NAME
Architecture: $deb_arch
Description: temporary metadata for dependency calculation
EOF

    dependencies=$(
        cd "$temporary_dir"
        dpkg-shlibdeps -O \
            "$stage_dir/usr/games/xpilot-infinity-sdl" \
            "$stage_dir/usr/games/xpilot-infinity-x11" \
            "$stage_dir/usr/games/xpilot-infinity-server" \
            "$stage_dir/usr/games/xpilot-infinity-replay" \
            "$stage_dir/usr/games/xpilot-infinity-xp-mapedit" \
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
 XPilot Infinity is a multiplayer tactical game. This package includes the SDL
 and X11 clients, the dedicated server, utilities, game data, and manuals.
EOF
}

if test "${BUILD_LINUX_DIST_SOURCE_ONLY:-0}" = 1; then
    return 0 2>/dev/null || exit 0
fi

for variable_name in \
    XPILOT_WORK_DIR XPILOT_META_DIR \
    XPILOT_VERSION \
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
require_command dpkg-parsechangelog
require_command dpkg-shlibdeps
require_command gzip
require_command make
require_command pkg-config
require_command "${STRIP:-strip}"

test -x ./configure || fail "configure is unavailable; run ./bootstrap first"
test -x ./vendor/sdl3/build.sh \
    || fail "the vendored SDL3 build script is unavailable"
test ! -f ./config.status \
    || fail "the source tree must not be configured in place"

work_dir=$XPILOT_WORK_DIR
meta_dir=$XPILOT_META_DIR
source_dir=$(pwd)
dependency_build_dir="$work_dir/vendor-sdl3"
dependency_prefix="$work_dir/vendor-sdl3-prefix"
build_dir="$work_dir/xpilot-infinity"
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
        --with-sdl3=vendored \
        "--with-sdl3-prefix=$dependency_prefix"
    make -j"$XPILOT_MAKE_JOBS" \
        "XPILOT_VERSION=$XPILOT_VERSION"
    make "XPILOT_VERSION=$XPILOT_VERSION" \
        install DESTDIR="$stage_dir"
)

for executable_name in \
    xpilot-infinity-sdl xpilot-infinity-x11 xpilot-infinity-server \
    xpilot-infinity-replay xpilot-infinity-xp-mapedit
do
    assert_file "$stage_dir/usr/games/$executable_name"
done
assert_file "$stage_dir/usr/share/games/xpilot-infinity/defaults.txt"
assert_file "$stage_dir/usr/share/games/xpilot-infinity/maps/ndh.xp2"
assert_file "$stage_dir/usr/share/games/xpilot-infinity/sound/sounds.txt"
assert_file "$stage_dir/usr/share/games/xpilot-infinity/sound/bfire.wav"
prepare_debian_package_files
assert_file "$stage_dir/usr/share/man/man6/xpilot-infinity-sdl.6.gz"
assert_file "$stage_dir/usr/share/doc/$XPILOT_PACKAGE_NAME/copyright"
assert_file "$stage_dir/usr/share/doc/$XPILOT_PACKAGE_NAME/changelog.gz"
assert_file \
    "$stage_dir/usr/share/doc/$XPILOT_PACKAGE_NAME/changelog.Debian.gz"

deb_arch=$(dpkg-architecture -qDEB_HOST_ARCH)
runtime_dependencies=$(calculate_runtime_dependencies)
assert_debian_dependency "$runtime_dependencies" libalut0
assert_debian_dependency "$runtime_dependencies" libopenal1
write_control_file "$runtime_dependencies"

printf '%s\n' "$deb_arch" > "$meta_dir/deb_arch"
