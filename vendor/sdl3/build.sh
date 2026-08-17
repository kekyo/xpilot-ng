#!/bin/sh

set -eu

usage()
{
    cat <<'EOF'
Usage: vendor/sdl3/build.sh --build-dir PATH --prefix PATH [OPTIONS]

Build the pinned SDL3, SDL3_image, and SDL3_ttf submodules as static
libraries and install them into a private prefix.

Options:
  --build-dir PATH          Out-of-tree CMake build directory (required)
  --prefix PATH             Private installation prefix (required)
  --jobs NUMBER             Parallel build jobs (default: detected CPU count)
  --build-type TYPE         CMake build type (default: Release)
  --toolchain-file PATH     CMake toolchain file for cross-compilation
  --help                    Show this help

The build uses system codec, FreeType, and HarfBuzz development packages.
It never downloads dependencies; initialize the git submodules first.
EOF
}

fail()
{
    echo "vendor/sdl3/build.sh: $*" >&2
    exit 1
}

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
build_dir=
install_prefix=
jobs=
build_type=Release
toolchain_file=

while test "$#" -gt 0; do
    case "$1" in
        --build-dir)
            test "$#" -ge 2 || fail "--build-dir requires a path"
            build_dir=$2
            shift 2
            ;;
        --build-dir=*)
            build_dir=${1#*=}
            shift
            ;;
        --prefix)
            test "$#" -ge 2 || fail "--prefix requires a path"
            install_prefix=$2
            shift 2
            ;;
        --prefix=*)
            install_prefix=${1#*=}
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
        *)
            fail "unknown option: $1"
            ;;
    esac
done

test -n "$build_dir" || fail "--build-dir is required"
test -n "$install_prefix" || fail "--prefix is required"

case "$build_dir" in
    /*) ;;
    *) fail "--build-dir must be an absolute path" ;;
esac
case "$install_prefix" in
    /*) ;;
    *) fail "--prefix must be an absolute path" ;;
esac
test "$build_dir" != / || fail "refusing to use / as the build directory"
test "$install_prefix" != / || fail "refusing to install into /"

if test -z "$jobs"; then
    jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
fi
case "$jobs" in
    ''|0|*[!0-9]*) fail "--jobs must be a positive integer" ;;
esac

if test -n "$toolchain_file"; then
    case "$toolchain_file" in
        /*) ;;
        *) fail "--toolchain-file must be an absolute path" ;;
    esac
    test -f "$toolchain_file" \
        || fail "toolchain file not found: $toolchain_file"
fi

for project in SDL SDL_image SDL_ttf; do
    test -f "$script_dir/$project/CMakeLists.txt" \
        || fail "missing $project submodule; run git submodule update --init"
done

command -v cmake >/dev/null 2>&1 || fail "cmake was not found"
pkg_config_program=${PKG_CONFIG:-pkg-config}
command -v "$pkg_config_program" >/dev/null 2>&1 \
    || fail "pkg-config was not found"

mkdir -p "$build_dir" "$install_prefix"
build_dir=$(CDPATH= cd -- "$build_dir" && pwd)
install_prefix=$(CDPATH= cd -- "$install_prefix" && pwd)
test "$build_dir" != "$install_prefix" \
    || fail "build directory and installation prefix must differ"

cmake_prefix_path=$install_prefix
if test -n "${CMAKE_PREFIX_PATH:-}"; then
    cmake_prefix_path="$cmake_prefix_path;$CMAKE_PREFIX_PATH"
fi

configure_project()
{
    project_name=$1
    shift
    project_build_dir="$build_dir/$project_name"

    set -- \
        -S "$script_dir/$project_name" \
        -B "$project_build_dir" \
        "-DCMAKE_BUILD_TYPE=$build_type" \
        "-DCMAKE_INSTALL_PREFIX=$install_prefix" \
        -DCMAKE_INSTALL_LIBDIR=lib \
        "$@"
    if test -n "$toolchain_file"; then
        set -- "$@" "-DCMAKE_TOOLCHAIN_FILE=$toolchain_file"
    fi

    cmake "$@"
    cmake --build "$project_build_dir" --config "$build_type" \
        --parallel "$jobs"
    cmake --install "$project_build_dir" --config "$build_type"
}

configure_project SDL \
    -DSDL_SHARED=OFF \
    -DSDL_STATIC=ON \
    -DSDL_TEST_LIBRARY=OFF \
    -DSDL_TESTS=OFF \
    -DSDL_EXAMPLES=OFF \
    -DSDL_INSTALL=ON \
    -DSDL_INSTALL_DOCS=OFF \
    -DSDL_RPATH=OFF

vendor_pkg_config_path="$install_prefix/lib/pkgconfig"
if test -n "${PKG_CONFIG_PATH:-}"; then
    PKG_CONFIG_PATH="$vendor_pkg_config_path:$PKG_CONFIG_PATH"
else
    PKG_CONFIG_PATH=$vendor_pkg_config_path
fi
export PKG_CONFIG_PATH

configure_project SDL_image \
    "-DCMAKE_PREFIX_PATH=$cmake_prefix_path" \
    -DBUILD_SHARED_LIBS=OFF \
    -DSDLIMAGE_INSTALL=ON \
    -DSDLIMAGE_INSTALL_MAN=OFF \
    -DSDLIMAGE_VENDORED=OFF \
    -DSDLIMAGE_DEPS_SHARED=ON \
    -DSDLIMAGE_SAMPLES=OFF \
    -DSDLIMAGE_TESTS=OFF

configure_project SDL_ttf \
    "-DCMAKE_PREFIX_PATH=$cmake_prefix_path" \
    -DBUILD_SHARED_LIBS=OFF \
    -DSDLTTF_INSTALL=ON \
    -DSDLTTF_INSTALL_MAN=OFF \
    -DSDLTTF_VENDORED=OFF \
    -DSDLTTF_SAMPLES=OFF \
    -DSDLTTF_HARFBUZZ=ON \
    -DSDLTTF_PLUTOSVG=OFF

for pc_file in sdl3.pc sdl3-image.pc sdl3-ttf.pc; do
    test -f "$vendor_pkg_config_path/$pc_file" \
        || fail "expected pkg-config metadata was not installed: $pc_file"
done

"$pkg_config_program" --static --exists \
    'sdl3 = 3.4.14' 'sdl3-image = 3.4.4' 'sdl3-ttf = 3.2.2' \
    || fail "the installed SDL3 dependency set is incomplete"

echo "Vendored SDL3 dependencies installed in $install_prefix"
