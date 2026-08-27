#!/bin/sh

set -eu

usage()
{
    cat <<'EOF'
Usage: generate-deps-makefile.sh --prefix PATH --output PATH [OPTIONS]

Generate a GNU Make fragment containing MinGW SDL3 and sound dependency flags
from an isolated vendored dependency prefix.

Options:
  --prefix PATH       Installed MinGW dependency prefix (required)
  --output PATH       Make fragment to replace atomically (required)
  --pkg-config PATH   pkg-config program (default: pkg-config)
  --help              Show this help
EOF
}

fail()
{
    echo "generate-deps-makefile.sh: $*" >&2
    exit 1
}

dependency_prefix=
output_file=
pkg_config_program=pkg-config

while test "$#" -gt 0; do
    case "$1" in
        --prefix)
            test "$#" -ge 2 || fail "--prefix requires a path"
            dependency_prefix=$2
            shift 2
            ;;
        --prefix=*)
            dependency_prefix=${1#*=}
            shift
            ;;
        --output)
            test "$#" -ge 2 || fail "--output requires a path"
            output_file=$2
            shift 2
            ;;
        --output=*)
            output_file=${1#*=}
            shift
            ;;
        --pkg-config)
            test "$#" -ge 2 || fail "--pkg-config requires a path"
            pkg_config_program=$2
            shift 2
            ;;
        --pkg-config=*)
            pkg_config_program=${1#*=}
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        *)
            fail "unknown option: $1"
            ;;
    esac
done

test -n "$dependency_prefix" || fail "--prefix is required"
test -n "$output_file" || fail "--output is required"
case "$dependency_prefix" in
    /*) ;;
    *) fail "--prefix must be an absolute path" ;;
esac
case "$output_file" in
    /*) ;;
    *) fail "--output must be an absolute path" ;;
esac
command -v "$pkg_config_program" >/dev/null 2>&1 \
    || fail "pkg-config program was not found: $pkg_config_program"

pkg_config_dir="$dependency_prefix/lib/pkgconfig"
test -d "$pkg_config_dir" \
    || fail "pkg-config directory was not found: $pkg_config_dir"

PKG_CONFIG_PATH=
PKG_CONFIG_LIBDIR=$pkg_config_dir
export PKG_CONFIG_PATH PKG_CONFIG_LIBDIR

"$pkg_config_program" --atleast-version=3.2.0 sdl3 \
    || fail "vendored SDL3 metadata is unavailable"
"$pkg_config_program" --atleast-version=3.2.0 sdl3-image \
    || fail "vendored SDL3_image metadata is unavailable"
"$pkg_config_program" --atleast-version=3.2.0 sdl3-ttf \
    || fail "vendored SDL3_ttf metadata is unavailable"
"$pkg_config_program" --atleast-version=1.1.0 freealut \
    || fail "vendored freealut metadata is unavailable"
"$pkg_config_program" --atleast-version=1.25.2 openal \
    || fail "vendored OpenAL Soft metadata is unavailable"

sdl3_cflags=$("$pkg_config_program" --cflags sdl3)
sdl3_libs=$("$pkg_config_program" --static --libs sdl3)
sdl3_image_cflags=$("$pkg_config_program" --cflags sdl3-image)
sdl3_image_libs=$("$pkg_config_program" --static --libs sdl3-image)
sdl3_ttf_cflags=$("$pkg_config_program" --cflags sdl3-ttf)
sdl3_ttf_libs=$("$pkg_config_program" --static --libs sdl3-ttf)
# The upstream freealut CMake metadata marks shared consumers as ALUT_STATIC.
# OpenAL's include flags cover both installed AL and ALUT headers, while the
# freealut library metadata still supplies the correct shared-library order.
sound_cflags=$("$pkg_config_program" --cflags openal)
sound_libs=$("$pkg_config_program" --libs freealut)

output_tmp="$output_file.tmp.$$"
cleanup()
{
    test ! -f "$output_tmp" || rm -f -- "$output_tmp"
}
trap cleanup EXIT HUP INT TERM

{
    printf '%s\n' '# Generated from the target dependency pkg-config files.'
    printf 'MINGW_SDL3_CFLAGS := %s\n' "$sdl3_cflags"
    printf 'MINGW_SDL3_LIBS := %s\n' "$sdl3_libs"
    printf 'MINGW_SDL3_IMAGE_CFLAGS := %s\n' "$sdl3_image_cflags"
    printf 'MINGW_SDL3_IMAGE_LIBS := %s\n' "$sdl3_image_libs"
    printf 'MINGW_SDL3_TTF_CFLAGS := %s\n' "$sdl3_ttf_cflags"
    printf 'MINGW_SDL3_TTF_LIBS := %s\n' "$sdl3_ttf_libs"
    printf 'MINGW_SOUND_CFLAGS := %s\n' "$sound_cflags"
    printf 'MINGW_SOUND_LIBS := %s\n' "$sound_libs"
} >"$output_tmp"
mv -f -- "$output_tmp" "$output_file"
trap - EXIT HUP INT TERM
