#!/bin/sh

set -eu

usage()
{
    cat <<'EOF'
Usage: build-windows-installer.sh --package-dir PATH --output PATH \
       --version VERSION --arch ARCH --icon PATH

Compile the XPilot Infinity NSIS installer for an assembled Windows package.
ARCH must be x86 or x86_64. Set MAKENSIS to override the compiler command.
EOF
}

fail()
{
    echo "build-windows-installer.sh: $*" >&2
    exit 1
}

package_dir=
output_path=
package_version=
architecture=
icon_path=

while test "$#" -gt 0; do
    case "$1" in
        --package-dir)
            test "$#" -ge 2 || fail "--package-dir requires a path"
            package_dir=$2
            shift 2
            ;;
        --package-dir=*)
            package_dir=${1#*=}
            shift
            ;;
        --output)
            test "$#" -ge 2 || fail "--output requires a path"
            output_path=$2
            shift 2
            ;;
        --output=*)
            output_path=${1#*=}
            shift
            ;;
        --version)
            test "$#" -ge 2 || fail "--version requires a value"
            package_version=$2
            shift 2
            ;;
        --version=*)
            package_version=${1#*=}
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
        --icon)
            test "$#" -ge 2 || fail "--icon requires a path"
            icon_path=$2
            shift 2
            ;;
        --icon=*)
            icon_path=${1#*=}
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

for required_path in "$package_dir" "$output_path" "$icon_path"; do
    test -n "$required_path" || fail "all path options are required"
    case "$required_path" in
        /*) ;;
        *) fail "all paths must be absolute" ;;
    esac
done
test -d "$package_dir" \
    || fail "Windows package directory was not found: $package_dir"
test -f "$package_dir/xpilot-infinity-server.exe" \
    || fail "Windows server executable is missing from the package"
test -f "$package_dir/xpilot-infinity-sdl.exe" \
    || fail "Windows SDL client executable is missing from the package"
test -f "$package_dir/COPYING" \
    || fail "XPilot Infinity license is missing from the package"
test -f "$icon_path" || fail "Windows icon was not found: $icon_path"

case "$architecture" in
    x86|x86_64) ;;
    *) fail "--arch must be x86 or x86_64" ;;
esac
case "$package_version" in
    ''|*[!0-9A-Za-z.+:~-]*) fail "invalid package version: $package_version" ;;
esac
case "$output_path" in
    "$package_dir"|"$package_dir"/*)
        fail "installer output must be outside the package directory"
        ;;
esac

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
installer_script="$script_dir/xpilot-infinity.nsi"
server_config="$script_dir/xpilot-infinity-server.conf"
test -f "$installer_script" \
    || fail "NSIS script was not found: $installer_script"
test -f "$server_config" \
    || fail "Windows server defaults were not found: $server_config"

file_version=$(printf '%s\n' "$package_version" \
    | sed -n 's/^\([0-9][0-9]*\)\.\([0-9][0-9]*\)\.\([0-9][0-9]*\).*$/\1.\2.\3.0/p')
test -n "$file_version" || file_version=0.0.0.0

makensis_program=${MAKENSIS:-makensis}
command -v "$makensis_program" >/dev/null 2>&1 \
    || fail "makensis was not found: $makensis_program"

mkdir -p "$(dirname -- "$output_path")"
rm -f -- "$output_path"
"$makensis_program" -V2 -WX \
    "-DXPILOT_PACKAGE_DIR=$package_dir" \
    "-DXPILOT_OUTPUT=$output_path" \
    "-DXPILOT_VERSION=$package_version" \
    "-DXPILOT_FILE_VERSION=$file_version" \
    "-DXPILOT_ARCH=$architecture" \
    "-DXPILOT_ICON=$icon_path" \
    "-DXPILOT_SERVER_CONFIG=$server_config" \
    "$installer_script"
test -f "$output_path" \
    || fail "NSIS did not create the installer: $output_path"
echo "Windows installer created in $output_path"
