#!/bin/sh

set -eu

usage()
{
    cat <<'EOF'
Usage: package-windows.sh --source-dir PATH --build-dir PATH --output PATH

Assemble statically linked MinGW executables and XPilot NG game data into a
relocatable Windows package directory.
EOF
}

fail()
{
    echo "package-windows.sh: $*" >&2
    exit 1
}

source_dir=
build_dir=
package_dir=

while test "$#" -gt 0; do
    case "$1" in
        --source-dir)
            test "$#" -ge 2 || fail "--source-dir requires a path"
            source_dir=$2
            shift 2
            ;;
        --source-dir=*)
            source_dir=${1#*=}
            shift
            ;;
        --build-dir)
            test "$#" -ge 2 || fail "--build-dir requires a path"
            build_dir=$2
            shift 2
            ;;
        --build-dir=*)
            build_dir=${1#*=}
            shift
            ;;
        --output)
            test "$#" -ge 2 || fail "--output requires a path"
            package_dir=$2
            shift 2
            ;;
        --output=*)
            package_dir=${1#*=}
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

for required_path in "$source_dir" "$build_dir" "$package_dir"; do
    test -n "$required_path" || fail "all path options are required"
    case "$required_path" in
        /*) ;;
        *) fail "all paths must be absolute" ;;
    esac
done
test -d "$source_dir" || fail "source directory was not found: $source_dir"
test -d "$build_dir" || fail "build directory was not found: $build_dir"
case "$package_dir" in
    "$build_dir"/*) ;;
    *) fail "package output must be inside the build directory" ;;
esac

server_executable="$build_dir/src/server/xpilot-ng-server.exe"
client_executable="$build_dir/src/client/sdl/xpilot-ng-sdl.exe"
test -f "$server_executable" \
    || fail "Windows server executable is missing: $server_executable"
test -f "$client_executable" \
    || fail "Windows SDL client executable is missing: $client_executable"

package_work_dir="$package_dir.tmp"
for output_dir in "$package_dir" "$package_work_dir"; do
    case "$output_dir" in
        "$build_dir"/*) rm -rf -- "$output_dir" ;;
        *) fail "refusing to replace unexpected package path" ;;
    esac
done
mkdir -p "$package_work_dir/lib"
cp "$server_executable" "$client_executable" "$package_work_dir/"

for data_directory in fonts maps textures sound; do
    if test -d "$source_dir/lib/$data_directory"; then
        cp -R "$source_dir/lib/$data_directory" \
            "$package_work_dir/lib/$data_directory"
    fi
done
for data_file in defaults.txt password.txt robots.txt shipshapes.txt; do
    test -f "$source_dir/lib/$data_file" \
        || fail "required game data is missing: lib/$data_file"
    cp "$source_dir/lib/$data_file" "$package_work_dir/lib/"
done
cp "$source_dir/COPYING" "$package_work_dir/"

mv "$package_work_dir" "$package_dir"
echo "Windows package assembled in $package_dir"
