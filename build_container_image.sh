#!/bin/sh

set -eu

PROJECT_ROOT=${BUILD_CONTAINER_PROJECT_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)}
VERSION_RESOLVER=${XPILOT_VERSION_RESOLVER:-"$PROJECT_ROOT/config/resolve-version.sh"}
DEFAULT_IMAGE_NAME=localhost/xpilot-infinity-server
DEFAULT_PARALLEL_JOB_CAP=14
DEFAULT_SOURCE_URL=https://github.com/kekyo/xpilot-infinity

print_usage()
{
    cat <<'EOF'
Usage: ./build_container_image.sh [OPTIONS]

Build the dedicated XPilot Infinity server image with Podman. This command
never pushes the resulting image.

Options:
  --version VERSION   Product version override (default: screw-up-derived)
  --revision REVISION Source revision label override (default: current Git HEAD)
  --tag IMAGE         Local image or manifest name
                      (default: localhost/xpilot-infinity-server:<version>)
  --platform LIST     Comma-separated target platforms, for example
                      linux/amd64,linux/arm64 (default: native platform)
  --jobs NUMBER       Concurrent make jobs (default: auto, up to 14)
  --base-image IMAGE  Override the pinned Debian builder/runtime image
  --source-url URL    Override the OCI source label
  --print-version     Print the resolved product version and exit
  --help              Show this help

For multiple platforms, IMAGE names a local manifest list. Use
`podman manifest push --all` separately after validating the result.
EOF
}

fail()
{
    echo "build_container_image.sh: $*" >&2
    exit 1
}

require_command()
{
    command -v "$1" >/dev/null 2>&1 \
        || fail "required command was not found: $1"
}

validate_version()
{
    case $1 in
        ''|*[!0-9A-Za-z.+:~-]*) fail "invalid product version: $1" ;;
    esac
}

validate_positive_integer()
{
    case $2 in
        ''|*[!0-9]*) fail "$1 must be a positive integer: $2" ;;
    esac
    test "$2" -gt 0 || fail "$1 must be a positive integer: $2"
}

validate_platforms()
{
    saved_ifs=$IFS
    IFS=,
    for platform in $1; do
        case $platform in
            linux/amd64|linux/arm64) ;;
            *)
                IFS=$saved_ifs
                fail "unsupported container platform: $platform"
                ;;
        esac
    done
    IFS=$saved_ifs
}

detect_processor_count()
{
    detected_count=
    if command -v getconf >/dev/null 2>&1; then
        detected_count=$(getconf _NPROCESSORS_ONLN 2>/dev/null || true)
    fi
    if test -z "$detected_count" && command -v nproc >/dev/null 2>&1; then
        detected_count=$(nproc 2>/dev/null || true)
    fi

    case $detected_count in
        ''|*[!0-9]*) detected_count=1 ;;
    esac
    test "$detected_count" -gt 0 || detected_count=1
    if test "$detected_count" -gt "$DEFAULT_PARALLEL_JOB_CAP"; then
        detected_count=$DEFAULT_PARALLEL_JOB_CAP
    fi
    printf '%s\n' "$detected_count"
}

detect_native_platform()
{
    require_command uname
    case $(uname -m) in
        x86_64|amd64) printf '%s\n' linux/amd64 ;;
        aarch64|arm64) printf '%s\n' linux/arm64 ;;
        *) fail "unsupported native container architecture: $(uname -m)" ;;
    esac
}

detect_version()
{
    test -x "$VERSION_RESOLVER" \
        || fail "version resolver is unavailable: $VERSION_RESOLVER"
    XPILOT_VERSION= "$VERSION_RESOLVER"
}

detect_revision()
{
    if command -v git >/dev/null 2>&1 \
        && git -C "$PROJECT_ROOT" rev-parse --verify HEAD >/dev/null 2>&1
    then
        resolved_revision=$(git -C "$PROJECT_ROOT" rev-parse HEAD)
        if ! git -C "$PROJECT_ROOT" diff --quiet --ignore-submodules -- \
            || test -n "$(git -C "$PROJECT_ROOT" ls-files \
                --others --exclude-standard | sed -n '1p')"
        then
            resolved_revision="$resolved_revision-dirty"
        fi
        printf '%s\n' "$resolved_revision"
    else
        printf '%s\n' unknown
    fi
}

version=
revision=
image_name=
platforms=
jobs=
base_image=
source_url=$DEFAULT_SOURCE_URL
print_version=false

while test $# -gt 0; do
    case $1 in
        --version|--revision|--tag|--platform|--jobs|--base-image|--source-url)
            test $# -ge 2 || fail "$1 requires an argument"
            option_name=$1
            option_value=$2
            shift 2
            case $option_name in
                --version) version=$option_value ;;
                --revision) revision=$option_value ;;
                --tag) image_name=$option_value ;;
                --platform) platforms=$option_value ;;
                --jobs) jobs=$option_value ;;
                --base-image) base_image=$option_value ;;
                --source-url) source_url=$option_value ;;
            esac
            ;;
        --print-version)
            print_version=true
            shift
            ;;
        --help|-h)
            print_usage
            exit 0
            ;;
        --)
            shift
            test $# -eq 0 || fail "unexpected positional arguments: $*"
            ;;
        *) fail "unknown option: $1" ;;
    esac
done

if test -z "$version"; then
    version=$(detect_version)
fi
validate_version "$version"
if $print_version; then
    printf '%s\n' "$version"
    exit 0
fi

test -f "$PROJECT_ROOT/Dockerfile" \
    || fail "Dockerfile is unavailable: $PROJECT_ROOT/Dockerfile"
test -n "$source_url" || fail "source URL must not be empty"
if test -z "$revision"; then
    revision=$(detect_revision)
fi
test -n "$revision" || fail "revision must not be empty"
if test -z "$image_name"; then
    image_name="$DEFAULT_IMAGE_NAME:$version"
fi
if test -z "$jobs"; then
    jobs=$(detect_processor_count)
fi
validate_positive_integer jobs "$jobs"
if test -z "$platforms"; then
    platforms=$(detect_native_platform)
fi
validate_platforms "$platforms"

container_engine=${CONTAINER_ENGINE:-podman}
require_command "$container_engine"

set -- \
    --file "$PROJECT_ROOT/Dockerfile" \
    --build-arg "XPILOT_BUILD_JOBS=$jobs" \
    --build-arg "XPILOT_REVISION=$revision" \
    --build-arg "XPILOT_SOURCE=$source_url" \
    --build-arg "XPILOT_VERSION=$version"
if test -n "$base_image"; then
    set -- "$@" --build-arg "DEBIAN_IMAGE=$base_image"
fi

case $platforms in
    *,*)
        "$container_engine" build "$@" \
            --platform "$platforms" \
            --manifest "$image_name" \
            "$PROJECT_ROOT"
        echo "Built local multi-platform manifest: $image_name"
        ;;
    *)
        "$container_engine" build "$@" \
            --platform "$platforms" \
            --tag "$image_name" \
            "$PROJECT_ROOT"
        echo "Built local image: $image_name ($platforms)"
        ;;
esac

echo "No image was pushed."
