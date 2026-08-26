#!/bin/sh

set -eu

fail()
{
	echo "resolve-version.sh: $*" >&2
	exit 1
}

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_root=$(CDPATH= cd -- "$script_dir/.." && pwd)
resolved_version=${XPILOT_VERSION:-}

if test -z "$resolved_version"; then
	screw_up=${SCREW_UP:-screw-up}
	command -v "$screw_up" >/dev/null 2>&1 \
		|| fail "screw-up was not found: $screw_up"
	resolved_version=$(
		cd "$project_root"
		printf '%s\n' '{version}' \
			| "$screw_up" format \
			| tr -d '\r' \
			| sed -n '1p'
	)
fi

case "$resolved_version" in
	''|*[!0-9A-Za-z.+:~-]*) fail "invalid version: $resolved_version" ;;
esac

printf '%s\n' "$resolved_version"
