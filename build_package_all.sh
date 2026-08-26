#!/bin/sh

set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
package_script=${BUILD_PACKAGE_SCRIPT:-$project_root/build_package.sh}

exec "$package_script" --target all "$@"
