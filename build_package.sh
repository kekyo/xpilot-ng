#!/bin/sh

set -eu

PROJECT_ROOT=${BUILD_PACKAGE_PROJECT_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)}
ARTIFACT_ROOT="$PROJECT_ROOT/artifacts"
DEB_ARTIFACT_ROOT="$ARTIFACT_ROOT/deb"
PACKAGE_NAME=xpilot-ng
PACKAGE_DESCRIPTION="Multi-player tactical game with SDL and X11 clients."
DEFAULT_MAINTAINER="XPilot NG packager <packager@localhost>"
DEFAULT_PARALLEL_JOB_CAP=14

LINUX_MATRIX=$(cat <<'EOF'
debian bookworm x86_64 linux/amd64
debian bookworm i686 linux/386
debian bookworm arm64 linux/arm64
debian bookworm armv7l linux/arm/v7
debian trixie x86_64 linux/amd64
debian trixie i686 linux/386
debian trixie arm64 linux/arm64
debian trixie armv7l linux/arm/v7
debian trixie riscv64 linux/riscv64
ubuntu 22.04 x86_64 linux/amd64
ubuntu 22.04 arm64 linux/arm64
ubuntu 24.04 x86_64 linux/amd64
ubuntu 24.04 arm64 linux/arm64
ubuntu 26.04 x86_64 linux/amd64
ubuntu 26.04 arm64 linux/arm64
EOF
)

print_usage()
{
    cat <<'EOF'
Usage: ./build_package.sh [OPTIONS]

Build and validate distributable XPilot NG packages.

Options:
  --version VERSION  Package version (default: screw-up-derived version)
  --target TARGET    all or deb (default: all)
  --distro LIST      Comma-separated distribution filter
  --release LIST     Comma-separated release filter
  --arch LIST        Comma-separated Linux architecture filter
  --jobs NUMBER      Concurrent package jobs (default: auto, up to 14)
  --debug            Build packages without release optimization
  --print-version    Print the resolved package version and exit
  --help             Show this help

Run ./prereq.sh before this command to prepare the target container images.
EOF
}

fail()
{
    echo "build_package.sh: $*" >&2
    exit 1
}

assert_file()
{
    test -f "$1" || fail "missing expected file: $1"
}

assert_contains()
{
    target_path=$1
    expected_text=$2

    grep -F -- "$expected_text" "$target_path" >/dev/null 2>&1 \
        || fail "missing expected text in $target_path: $expected_text"
}

require_command()
{
    command -v "$1" >/dev/null 2>&1 \
        || fail "required command was not found: $1"
}

validate_positive_integer()
{
    value_name=$1
    value=$2

    case $value in
        ''|*[!0-9]*) fail "$value_name must be a positive integer: $value" ;;
    esac
    test "$value" -gt 0 \
        || fail "$value_name must be a positive integer: $value"
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
    if test -z "$detected_count" && command -v sysctl >/dev/null 2>&1; then
        detected_count=$(sysctl -n hw.ncpu 2>/dev/null || true)
    fi

    case $detected_count in
        ''|*[!0-9]*) detected_count=1 ;;
    esac
    test "$detected_count" -gt 0 || detected_count=1
    printf '%s\n' "$detected_count"
}

min_int()
{
    if test "$1" -le "$2"; then
        printf '%s\n' "$1"
    else
        printf '%s\n' "$2"
    fi
}

detect_version()
{
    require_command screw-up
    detected_version=$(
        cd "$PROJECT_ROOT"
        printf '%s\n' '{version}' | screw-up format | tr -d '\r' | head -n 1
    )
    test -n "$detected_version" || fail "screw-up did not return a version"
    printf '%s\n' "$detected_version"
}

validate_version()
{
    case $1 in
        ''|*[!0-9A-Za-z.+:~\-]*) fail "invalid package version: $1" ;;
    esac
}

canonical_distro()
{
    value=$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]' | tr -d '[:space:]')
    case $value in
        debian|ubuntu) printf '%s\n' "$value" ;;
        *) fail "unsupported distribution filter: $1" ;;
    esac
}

canonical_release()
{
    value=$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]' | tr -d '[:space:]')
    case $value in
        bookworm|trixie|22.04|24.04|26.04) printf '%s\n' "$value" ;;
        jammy) printf '%s\n' 22.04 ;;
        noble) printf '%s\n' 24.04 ;;
        *) fail "unsupported release filter: $1" ;;
    esac
}

canonical_arch()
{
    value=$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]' | tr -d '[:space:]')
    case $value in
        x86_64|amd64) printf '%s\n' x86_64 ;;
        i686|i386|i486|i586|x86) printf '%s\n' i686 ;;
        arm64|aarch64) printf '%s\n' arm64 ;;
        armv7l|armv7|armhf) printf '%s\n' armv7l ;;
        riscv64) printf '%s\n' riscv64 ;;
        *) fail "unsupported architecture filter: $1" ;;
    esac
}

normalize_filter_list()
{
    filter_kind=$1
    filter_value=$2

    if test -z "$filter_value"; then
        printf '\n'
        return 0
    fi

    previous_ifs=$IFS
    IFS=,
    normalized=
    for filter_item in $filter_value; do
        case $filter_kind in
            distro) resolved_filter=$(canonical_distro "$filter_item") ;;
            release) resolved_filter=$(canonical_release "$filter_item") ;;
            arch) resolved_filter=$(canonical_arch "$filter_item") ;;
            *)
                IFS=$previous_ifs
                fail "unsupported filter kind: $filter_kind"
                ;;
        esac
        normalized="${normalized}${normalized:+,}$resolved_filter"
    done
    IFS=$previous_ifs
    printf '%s\n' "$normalized"
}

matches_filter()
{
    filter_value=$1
    actual_value=$2
    test -n "$filter_value" || return 0

    previous_ifs=$IFS
    IFS=,
    for allowed_value in $filter_value; do
        if test "$allowed_value" = "$actual_value"; then
            IFS=$previous_ifs
            return 0
        fi
    done
    IFS=$previous_ifs
    return 1
}

count_deb_builds()
{
    build_count=0
    while IFS=' ' read -r distro release arch platform; do
        test -n "$distro" || continue
        matches_filter "$DISTRO_FILTER" "$distro" || continue
        matches_filter "$RELEASE_FILTER" "$release" || continue
        matches_filter "$ARCH_FILTER" "$arch" || continue
        build_count=$((build_count + 1))
    done <<EOF
$LINUX_MATRIX
EOF
    printf '%s\n' "$build_count"
}

count_matching_files()
{
    target_directory=$1
    target_pattern=$2
    if test ! -d "$target_directory"; then
        printf '0\n'
        return 0
    fi
    find "$target_directory" -maxdepth 1 -type f -name "$target_pattern" \
        -print | wc -l | tr -d '[:space:]'
}

expected_elf_class()
{
    case $1 in
        x86_64|arm64|riscv64) printf '%s\n' "Class:                             ELF64" ;;
        i686|armv7l) printf '%s\n' "Class:                             ELF32" ;;
        *) fail "unsupported ELF class lookup: $1" ;;
    esac
}

expected_elf_machine()
{
    case $1 in
        x86_64) printf '%s\n' "Machine:                           Advanced Micro Devices X86-64" ;;
        i686) printf '%s\n' "Machine:                           Intel 80386" ;;
        arm64) printf '%s\n' "Machine:                           AArch64" ;;
        armv7l) printf '%s\n' "Machine:                           ARM" ;;
        riscv64) printf '%s\n' "Machine:                           RISC-V" ;;
        *) fail "unsupported ELF machine lookup: $1" ;;
    esac
}

deb_arch_name()
{
    case $1 in
        x86_64) printf '%s\n' amd64 ;;
        i686) printf '%s\n' i386 ;;
        arm64) printf '%s\n' arm64 ;;
        armv7l) printf '%s\n' armhf ;;
        riscv64) printf '%s\n' riscv64 ;;
        *) fail "unsupported Debian architecture lookup: $1" ;;
    esac
}

deb_artifact_path()
{
    distro=$1
    release=$2
    arch=$3
    deb_arch=$(deb_arch_name "$arch")
    printf '%s\n' \
        "$DEB_ARTIFACT_ROOT/$PACKAGE_NAME-$VERSION-$distro-$release-$deb_arch.deb"
}

choose_container_engine()
{
    if test -n "${CONTAINER_ENGINE:-}"; then
        require_command "$CONTAINER_ENGINE"
        printf '%s\n' "$CONTAINER_ENGINE"
        return 0
    fi
    require_command podman
    printf '%s\n' podman
}

container_image_for_target()
{
    distro=$1
    release=$2
    arch=$3

    if test "$arch" = riscv64; then
        printf 'docker.io/library/%s:%s\n' "$distro" "$release"
        return 0
    fi

    case $arch in
        x86_64) repository_prefix=amd64 ;;
        i686) repository_prefix=i386 ;;
        arm64) repository_prefix=arm64v8 ;;
        armv7l) repository_prefix=arm32v7 ;;
        *) fail "unsupported package architecture: $arch" ;;
    esac
    printf 'docker.io/%s/%s:%s\n' "$repository_prefix" "$distro" "$release"
}

prereq_image_for_target()
{
    printf 'localhost/xpilot-ng-pack-deb-%s-%s-%s:latest\n' "$1" "$2" "$3"
}

assert_prereq_image()
{
    image=$1
    "$CONTAINER_ENGINE_BIN" image exists "$image" >/dev/null 2>&1 \
        || fail "missing prerequisite image: $image. Run ./prereq.sh first."
}

build_deb_package()
{
    distro=$1
    release=$2
    arch=$3
    platform=$4
    image=$(prereq_image_for_target "$distro" "$release" "$arch")
    work_root="$TMP_ROOT/deb/$distro/$release/$arch"
    container_root="/workspace/artifacts/.tmp/$RUN_ID/deb/$distro/$release/$arch"
    artifact_path=$(deb_artifact_path "$distro" "$release" "$arch")

    assert_prereq_image "$image"
    printf '%s\n' "[deb] $distro $release $arch ($platform, $image)"
    rm -rf "$work_root"
    mkdir -p "$DEB_ARTIFACT_ROOT"

    "$CONTAINER_ENGINE_BIN" run --rm \
        --platform "$platform" \
        -v "$PROJECT_ROOT:/workspace" \
        -w /workspace \
        -e XPILOT_WORK_DIR="$container_root/work" \
        -e XPILOT_META_DIR="$container_root/meta" \
        -e XPILOT_PACKAGE_VERSION="$VERSION" \
        -e XPILOT_PACKAGE_NAME="$PACKAGE_NAME" \
        -e XPILOT_PACKAGE_DESCRIPTION="$PACKAGE_DESCRIPTION" \
        -e XPILOT_PACKAGE_MAINTAINER="${DEB_MAINTAINER:-$DEFAULT_MAINTAINER}" \
        -e XPILOT_BUILD_TYPE="$BUILD_TYPE" \
        -e XPILOT_MAKE_JOBS="$MAKE_JOBS" \
        "$image" \
        ./scripts/build_linux_dist_container.sh

    dpkg-deb --root-owner-group --build \
        "$work_root/work/stage/$PACKAGE_NAME" "$artifact_path" >/dev/null
}

validate_deb_package()
{
    package_path=$1
    expected_arch=$2
    expected_deb_arch=$(deb_arch_name "$expected_arch")
    extract_dir=$(mktemp -d "${TMPDIR:-/tmp}/xpilot-deb-validate.XXXXXX")

    assert_file "$package_path"
    test "$(dpkg-deb -f "$package_path" Package)" = "$PACKAGE_NAME" \
        || fail "unexpected Package field in $package_path"
    test "$(dpkg-deb -f "$package_path" Architecture)" = "$expected_deb_arch" \
        || fail "unexpected Architecture field in $package_path"
    test "$(dpkg-deb -f "$package_path" Version)" = "$VERSION" \
        || fail "unexpected Version field in $package_path"
    test -n "$(dpkg-deb -f "$package_path" Depends)" \
        || fail "missing Depends field in $package_path"

    dpkg-deb -x "$package_path" "$extract_dir"
    for executable_name in \
        xpilot-ng-sdl xpilot-ng-x11 xpilot-ng-server \
        xpilot-ng-replay xpilot-ng-xp-mapedit
    do
        executable_path="$extract_dir/usr/games/$executable_name"
        assert_file "$executable_path"
        readelf -h "$executable_path" > "$extract_dir/$executable_name.readelf"
        assert_contains "$extract_dir/$executable_name.readelf" \
            "$(expected_elf_class "$expected_arch")"
        assert_contains "$extract_dir/$executable_name.readelf" \
            "$(expected_elf_machine "$expected_arch")"
    done

    assert_file "$extract_dir/usr/share/games/xpilot-ng/defaults.txt"
    assert_file "$extract_dir/usr/share/games/xpilot-ng/maps/ndh.xp2"
    assert_file "$extract_dir/usr/share/games/xpilot-ng/textures/ship.ppm"
    assert_file "$extract_dir/usr/share/man/man6/xpilot-ng-sdl.6"
    assert_file "$extract_dir/usr/share/doc/xpilot-ng/README"
    assert_file "$extract_dir/usr/share/doc/xpilot-ng/COPYING"

    rm -rf "$extract_dir"
}

validate_deb_artifacts()
{
    expected_count=$(count_deb_builds)
    actual_count=$(count_matching_files "$DEB_ARTIFACT_ROOT" '*.deb')
    test "$actual_count" = "$expected_count" \
        || fail "unexpected deb artifact count: $actual_count (expected $expected_count)"

    while IFS=' ' read -r distro release arch platform; do
        test -n "$distro" || continue
        matches_filter "$DISTRO_FILTER" "$distro" || continue
        matches_filter "$RELEASE_FILTER" "$release" || continue
        matches_filter "$ARCH_FILTER" "$arch" || continue
        validate_deb_package \
            "$(deb_artifact_path "$distro" "$release" "$arch")" "$arch"
    done <<EOF
$LINUX_MATRIX
EOF
}

wait_for_oldest_job()
{
    test "$ACTIVE_JOB_COUNT" -gt 0 || return 0
    set -- $ACTIVE_JOB_PIDS
    wait_pid=$1
    shift
    if wait "$wait_pid"; then
        :
    else
        JOB_FAILURE=1
    fi
    ACTIVE_JOB_PIDS=$*
    ACTIVE_JOB_COUNT=$((ACTIVE_JOB_COUNT - 1))
}

run_parallel_job()
{
    while test "$ACTIVE_JOB_COUNT" -ge "$PARALLEL_JOBS"; do
        wait_for_oldest_job
    done
    test "$JOB_FAILURE" -eq 0 || fail "one or more package builds failed"
    "$@" &
    ACTIVE_JOB_PIDS="${ACTIVE_JOB_PIDS}${ACTIVE_JOB_PIDS:+ }$!"
    ACTIVE_JOB_COUNT=$((ACTIVE_JOB_COUNT + 1))
}

wait_for_all_jobs()
{
    while test "$ACTIVE_JOB_COUNT" -gt 0; do
        wait_for_oldest_job
    done
    test "$JOB_FAILURE" -eq 0 || fail "one or more package builds failed"
}

schedule_deb_builds()
{
    while IFS=' ' read -r distro release arch platform; do
        test -n "$distro" || continue
        matches_filter "$DISTRO_FILTER" "$distro" || continue
        matches_filter "$RELEASE_FILTER" "$release" || continue
        matches_filter "$ARCH_FILTER" "$arch" || continue
        run_parallel_job build_deb_package "$distro" "$release" "$arch" "$platform"
    done <<EOF
$LINUX_MATRIX
EOF
}

main()
{
    VERSION=
    TARGET=all
    DISTRO_FILTER=
    RELEASE_FILTER=
    ARCH_FILTER=
    PARALLEL_JOBS=
    PRINT_VERSION=false
    BUILD_TYPE=Release

    while test "$#" -gt 0; do
        case $1 in
            --version)
                test "$#" -ge 2 || fail "--version requires a value"
                VERSION=$2
                shift 2
                ;;
            --target)
                test "$#" -ge 2 || fail "--target requires a value"
                TARGET=$2
                shift 2
                ;;
            --distro)
                test "$#" -ge 2 || fail "--distro requires a value"
                DISTRO_FILTER=$(normalize_filter_list distro "$2")
                shift 2
                ;;
            --release)
                test "$#" -ge 2 || fail "--release requires a value"
                RELEASE_FILTER=$(normalize_filter_list release "$2")
                shift 2
                ;;
            --arch)
                test "$#" -ge 2 || fail "--arch requires a value"
                ARCH_FILTER=$(normalize_filter_list arch "$2")
                shift 2
                ;;
            --jobs)
                test "$#" -ge 2 || fail "--jobs requires a value"
                PARALLEL_JOBS=$2
                shift 2
                ;;
            --debug)
                BUILD_TYPE=Debug
                shift
                ;;
            --print-version)
                PRINT_VERSION=true
                shift
                ;;
            --help)
                print_usage
                exit 0
                ;;
            *) fail "unknown argument: $1" ;;
        esac
    done

    test -n "$VERSION" || VERSION=$(detect_version)
    validate_version "$VERSION"
    if test -n "$PARALLEL_JOBS"; then
        validate_positive_integer "parallel job count" "$PARALLEL_JOBS"
    fi
    if test "$PRINT_VERSION" = true; then
        printf '%s\n' "$VERSION"
        exit 0
    fi

    case $TARGET in
        all|deb) ;;
        *) fail "unsupported target: $TARGET" ;;
    esac

    CPU_COUNT=$(detect_processor_count)
    test -n "$PARALLEL_JOBS" \
        || PARALLEL_JOBS=$(min_int "$CPU_COUNT" "$DEFAULT_PARALLEL_JOB_CAP")
    BUILD_TASK_COUNT=$(count_deb_builds)
    test "$BUILD_TASK_COUNT" -gt 0 || fail "no Debian package targets matched"
    EFFECTIVE_BUILD_JOBS=$(min_int "$PARALLEL_JOBS" "$BUILD_TASK_COUNT")
    MAKE_JOBS=$((CPU_COUNT / EFFECTIVE_BUILD_JOBS))
    test "$MAKE_JOBS" -gt 0 || MAKE_JOBS=1

    RUN_ID="run-$(date +%Y%m%d%H%M%S)-$$"
    TMP_ROOT="$ARTIFACT_ROOT/.tmp/$RUN_ID"
    ACTIVE_JOB_PIDS=
    ACTIVE_JOB_COUNT=0
    JOB_FAILURE=0

    mkdir -p "$ARTIFACT_ROOT"
    rm -rf "$DEB_ARTIFACT_ROOT"
    mkdir -p "$TMP_ROOT"

    require_command dpkg-deb
    require_command readelf
    CONTAINER_ENGINE_BIN=$(choose_container_engine)
    printf '%s\n' \
        "Using up to $PARALLEL_JOBS package jobs with $MAKE_JOBS make jobs each"
    schedule_deb_builds
    wait_for_all_jobs
    printf '%s\n' "Validating generated Debian packages"
    validate_deb_artifacts

    rm -rf "$TMP_ROOT"
    printf '%s\n' "Artifacts generated in $ARTIFACT_ROOT"
}

if test "${BUILD_PACKAGE_SOURCE_ONLY:-0}" = 1; then
    return 0 2>/dev/null || exit 0
fi

main "$@"
