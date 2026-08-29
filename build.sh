#!/bin/sh

set -eu

usage()
{
    cat <<'EOF'
Usage: ./build.sh [OPTIONS] [-- CONFIGURE_OPTIONS...]

Configure and build XPilot Infinity and its pinned dependencies in one command.
Build products are kept out of the source tree under ./build by default.

Options:
  --target TARGET          all, native, or windows (default: all)
  --arch ARCH              Windows x86, x86_64, or all (default: all)
  --test                   Run the target's complete supported test suite
  --build-root PATH        Root for all build output (default: ./build)
  --artifact-root PATH     Windows ZIP/installer output (default: ./artifacts/windows)
  --package-version VALUE  Build/archive version override (default: resolved at make time)
  --jobs NUMBER            Parallel build jobs (default: detected CPU count)
  --build-type TYPE        Dependency CMake build type (default: Release)
  --toolchain-file PATH    CMake toolchain file for one target architecture
  --help                   Show this help

Arguments after -- are forwarded to XPilot Infinity's configure script.  Native
builds use the vendored SDL3 provider.  Windows builds run on Linux with
MinGW and delegate dependency builds, packaging, and Wine tests to the
configured Makefiles for x86 and/or x86_64.  The default builds native and
both Windows architectures.
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
artifact_root="$source_dir/artifacts/windows"
if test "${XPILOT_PACKAGE_VERSION+x}" = x; then
    package_version=$XPILOT_PACKAGE_VERSION
    package_version_set=true
else
    package_version=
    package_version_set=false
fi
package_commit_id=${XPILOT_COMMIT_ID:-}
jobs=
build_type=Release
toolchain_file=
target=all
architecture=
run_tests=false

while test "$#" -gt 0; do
    case "$1" in
        --target)
            test "$#" -ge 2 || fail "--target requires a value"
            target=$2
            shift 2
            ;;
        --target=*)
            target=${1#*=}
            shift
            ;;
        --arch)
            test "$#" -ge 2 || fail "--arch requires a value"
            architecture=$2
            shift 2
            ;;
        --arch=*)
            architecture=${1#*=}
            shift
            ;;
        --test)
            run_tests=true
            shift
            ;;
        --build-root)
            test "$#" -ge 2 || fail "--build-root requires a path"
            build_root=$2
            shift 2
            ;;
        --build-root=*)
            build_root=${1#*=}
            shift
            ;;
        --artifact-root)
            test "$#" -ge 2 || fail "--artifact-root requires a path"
            artifact_root=$2
            shift 2
            ;;
        --artifact-root=*)
            artifact_root=${1#*=}
            shift
            ;;
        --package-version)
            test "$#" -ge 2 || fail "--package-version requires a value"
            package_version=$2
            package_version_set=true
            shift 2
            ;;
        --package-version=*)
            package_version=${1#*=}
            package_version_set=true
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

case "$target" in
    all)
        test -z "$architecture" \
            || fail "--arch is only valid with --target windows"
        test -z "$toolchain_file" \
            || fail "--toolchain-file requires a single target"
        architecture=all
        ;;
    native)
        test -z "$architecture" \
            || fail "--arch is only valid with --target windows"
        ;;
    windows)
        if test -z "$architecture"; then
            architecture=all
        fi
        case "$architecture" in
            x86|x86_64|all) ;;
            *) fail "--arch must be x86, x86_64, or all" ;;
        esac
        if test "$architecture" = all && test -n "$toolchain_file"; then
            fail "--toolchain-file requires a single Windows architecture"
        fi
        ;;
    *) fail "--target must be all, native, or windows" ;;
esac

test -n "$build_root" || fail "--build-root requires a nonempty path"
test -n "$build_type" || fail "--build-type requires a nonempty value"

if test "$package_version_set" = true; then
    case "$package_version" in
        ''|*[!0-9A-Za-z.+:~-]*) \
            fail "invalid package version: $package_version" \
            ;;
    esac
fi

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

case "$target" in
    all|windows)
        test -n "$artifact_root" \
            || fail "--artifact-root requires a nonempty path"
        case "$artifact_root" in
            /*) ;;
            *) artifact_root="$invocation_dir/$artifact_root" ;;
        esac
        test "$artifact_root" != / \
            || fail "refusing to use / as the artifact root"
        mkdir -p "$artifact_root"
        artifact_root=$(CDPATH= cd -- "$artifact_root" && pwd)

        windows_archiver="$source_dir/config/package-windows.mjs"
        test -f "$windows_archiver" \
            || fail "Windows archiver is unavailable: $windows_archiver"
        node_program=${NODE:-node}
        command -v "$node_program" >/dev/null 2>&1 \
            || fail "Node.js was not found: $node_program"
        zip_program=${ZIP:-zip}
        case "$zip_program" in
            /*) ;;
            */*) zip_program="$invocation_dir/$zip_program" ;;
        esac
        command -v "$zip_program" >/dev/null 2>&1 \
            || fail "zip was not found: $zip_program"

        windows_installer_builder="$source_dir/config/build-windows-installer.sh"
        windows_icon="$source_dir/images/icon.ico"
        test -x "$windows_installer_builder" \
            || fail "Windows installer builder is unavailable: $windows_installer_builder"
        test -f "$windows_icon" \
            || fail "Windows icon is unavailable: $windows_icon"
        makensis_program=${MAKENSIS:-makensis}
        command -v "$makensis_program" >/dev/null 2>&1 \
            || fail "makensis was not found: $makensis_program"

        ;;
esac

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

test -x "$source_dir/configure" \
    || fail "configure is unavailable; run ./bootstrap first"
test ! -f "$source_dir/config.status" \
    || fail "source tree is configured in place; run make distclean first"

metadata_resolver="$source_dir/config/resolve-build-metadata.sh"
test -x "$metadata_resolver" \
    || fail "build metadata resolver is unavailable: $metadata_resolver"
resolved_metadata=$(XPILOT_VERSION=$package_version \
    XPILOT_COMMIT_ID=$package_commit_id "$metadata_resolver")
build_version=$(printf '%s\n' "$resolved_metadata" | sed -n '1p')
build_commit_id=$(printf '%s\n' "$resolved_metadata" | sed -n '2p')

make_program=${MAKE:-make}
command -v "$make_program" >/dev/null 2>&1 \
    || fail "make was not found: $make_program"

build_native()
(
    vendor_builder="$source_dir/vendor/sdl3/build.sh"
    test -x "$vendor_builder" \
        || fail "vendored SDL3 builder is unavailable: $vendor_builder"

    vendor_build_dir="$build_root/vendor-sdl3"
    vendor_prefix="$build_root/vendor-sdl3-prefix"
    xpilot_build_dir="$build_root/xpilot-infinity"

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
    echo "===== configure: XPilot Infinity with vendored SDL3 ====="
    cd "$xpilot_build_dir"
    "$source_dir/configure" "$@" \
        --with-sdl3=vendored \
        "--with-sdl3-prefix=$vendor_prefix"
    "$make_program" "-j$jobs" \
        "XPILOT_VERSION=$build_version" \
        "XPILOT_COMMIT_ID=$build_commit_id"
    if test "$run_tests" = true; then
        "$make_program" "XPILOT_VERSION=$build_version" \
            "XPILOT_COMMIT_ID=$build_commit_id" check
    fi

    echo "XPilot Infinity build completed in $xpilot_build_dir"
)

build_windows_architecture()
(
    windows_architecture=$1
    shift

    case "$windows_architecture" in
        x86)
            triplet=i686-w64-mingw32
            ;;
        x86_64)
            triplet=x86_64-w64-mingw32
            ;;
        *)
            fail "unsupported Windows architecture: $windows_architecture"
            ;;
    esac

    architecture_root="$build_root/windows/$windows_architecture"
    install_prefix="$architecture_root/install"
    mkdir -p "$architecture_root"
    echo "===== configure: XPilot Infinity for $triplet ====="
    cd "$architecture_root"
    if test -n "$toolchain_file"; then
        "$source_dir/configure" "$@" \
            "--host=$triplet" \
            "--prefix=$install_prefix" \
            --enable-mingw-vendored-deps \
            "--with-mingw-deps-build-type=$build_type" \
            "--with-mingw-toolchain-file=$toolchain_file"
    else
        "$source_dir/configure" "$@" \
            "--host=$triplet" \
            "--prefix=$install_prefix" \
            --enable-mingw-vendored-deps \
            "--with-mingw-deps-build-type=$build_type"
    fi

    "$make_program" "-j$jobs" "MINGW_DEPS_JOBS=$jobs" \
        "XPILOT_VERSION=$build_version" \
        "XPILOT_COMMIT_ID=$build_commit_id"
    "$make_program" "MINGW_DEPS_JOBS=$jobs" \
        "XPILOT_VERSION=$build_version" \
        "XPILOT_COMMIT_ID=$build_commit_id" windows-package

    installer_build_path="$architecture_root/xpilot-infinity-setup.exe"
    MAKENSIS="$makensis_program" "$windows_installer_builder" \
        --package-dir "$architecture_root/package" \
        --output "$installer_build_path" \
        --version "$build_version" \
        --arch "$windows_architecture" \
        --icon "$windows_icon"
    if test "$run_tests" = true; then
        "$make_program" "MINGW_DEPS_JOBS=$jobs" \
            "MINGW_TEST_JOBS=$jobs" \
            "XPILOT_VERSION=$build_version" \
            "XPILOT_COMMIT_ID=$build_commit_id" check
    fi

    archive_path="$artifact_root/xpilot-infinity-$build_version-windows-$windows_architecture.zip"
    ZIP="$zip_program" "$node_program" "$windows_archiver" \
        --input "$architecture_root/package" \
        --output "$archive_path"
    test -f "$archive_path" \
        || fail "Windows archive was not created: $archive_path"
    installer_path="$artifact_root/xpilot-infinity-$build_version-windows-$windows_architecture-setup.exe"
    cp -- "$installer_build_path" "$installer_path"
    test -f "$installer_path" \
        || fail "Windows installer was not created: $installer_path"
    echo "XPilot Infinity Windows package completed in $architecture_root/package"
    echo "XPilot Infinity Windows archive completed in $archive_path"
    echo "XPilot Infinity Windows installer completed in $installer_path"
)

build_windows()
{
    case "$architecture" in
        all)
            build_windows_architecture x86 "$@"
            build_windows_architecture x86_64 "$@"
            ;;
        *)
            build_windows_architecture "$architecture" "$@"
            ;;
    esac
}

case "$target" in
    all)
        build_native "$@"
        build_windows "$@"
        ;;
    native)
        build_native "$@"
        ;;
    windows)
        build_windows "$@"
        ;;
esac
