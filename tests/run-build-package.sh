#!/bin/sh

set -eu

fail()
{
    echo "run-build-package.sh: $*" >&2
    exit 1
}

assert_equal()
{
    expected=$1
    actual=$2
    message=$3

    test "$expected" = "$actual" \
        || fail "$message (expected '$expected', got '$actual')"
}

assert_contains()
{
    target_path=$1
    expected_text=$2

    grep -F -- "$expected_text" "$target_path" >/dev/null 2>&1 \
        || fail "$target_path does not contain: $expected_text"
}

test -n "${XPILOT_BUILD_PACKAGE:-}" \
    || fail "XPILOT_BUILD_PACKAGE is not set"
test -f "$XPILOT_BUILD_PACKAGE" \
    || fail "package build script is unavailable: $XPILOT_BUILD_PACKAGE"

package_script_dir=$(CDPATH= cd -- "$(dirname -- "$XPILOT_BUILD_PACKAGE")" \
    && pwd)
package_script="$package_script_dir/$(basename -- "$XPILOT_BUILD_PACKAGE")"
package_all_script="$package_script_dir/build_package_all.sh"
prereq_script="$package_script_dir/prereq.sh"

test -x "$package_all_script" \
    || fail "complete package build script is unavailable: $package_all_script"
test -x "$prereq_script" \
    || fail "prerequisite image script is unavailable: $prereq_script"

BUILD_PACKAGE_PROJECT_ROOT=$package_script_dir
BUILD_PACKAGE_SOURCE_ONLY=1
export BUILD_PACKAGE_PROJECT_ROOT BUILD_PACKAGE_SOURCE_ONLY
. "$XPILOT_BUILD_PACKAGE"
unset BUILD_PACKAGE_SOURCE_ONLY BUILD_PACKAGE_PROJECT_ROOT

assert_equal x86_64 "$(canonical_arch amd64)" \
    "amd64 architecture alias was not normalized"
assert_equal i686 "$(canonical_arch i386)" \
    "i386 architecture alias was not normalized"
assert_equal arm64 "$(canonical_arch aarch64)" \
    "aarch64 architecture alias was not normalized"
assert_equal armv7l "$(canonical_arch armhf)" \
    "armhf architecture alias was not normalized"
assert_equal 24.04 "$(canonical_release noble)" \
    "Ubuntu noble release alias was not normalized"

DISTRO_FILTER=
RELEASE_FILTER=
ARCH_FILTER=
assert_equal 15 "$(count_deb_builds)" \
    "the complete Linux package matrix was not selected"

DISTRO_FILTER=$(normalize_filter_list distro debian)
RELEASE_FILTER=$(normalize_filter_list release trixie)
ARCH_FILTER=$(normalize_filter_list arch riscv64)
assert_equal 1 "$(count_deb_builds)" \
    "a single Linux package target was not selected"

VERSION=4.7.99
DEB_ARTIFACT_ROOT=/tmp/xpilot-artifacts
assert_equal \
    /tmp/xpilot-artifacts/xpilot-ng-4.7.99-ubuntu-24.04-amd64.deb \
    "$(deb_artifact_path ubuntu 24.04 x86_64)" \
    "the Debian artifact name was incorrect"
assert_equal \
    localhost/xpilot-ng-pack-deb-debian-bookworm-x86_64:latest \
    "$(prereq_image_for_target debian bookworm x86_64)" \
    "the prerequisite image name was incorrect"

test_root=$(mktemp -d "${TMPDIR:-/tmp}/xpilot-package-test.XXXXXX")

cleanup()
{
    case "$test_root" in
        "${TMPDIR:-/tmp}"/xpilot-package-test.*)
            rm -rf -- "$test_root"
            ;;
    esac
}

trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

fixture_project="$test_root/project"
fixture_tools="$test_root/tools"
fixture_log="$test_root/container.log"
fixture_dpkg_log="$test_root/dpkg.log"
mkdir -p "$fixture_project/scripts" "$fixture_tools"

cat > "$fixture_tools/container-engine" <<'EOF'
#!/bin/sh
set -eu

if test "${1:-}" = image && test "${2:-}" = exists; then
    printf 'exists=%s\n' "${3:-}" >> "$XPILOT_PACKAGE_CONTAINER_LOG"
    exit 0
fi

test "${1:-}" = run || exit 2
workspace=
work_dir=
previous=
for argument do
    if test "$previous" = volume; then
        workspace=${argument%%:/workspace*}
        previous=
        continue
    fi
    if test "$previous" = environment; then
        case "$argument" in
            XPILOT_WORK_DIR=*) work_dir=${argument#*=} ;;
        esac
        previous=
        continue
    fi
    if test "$previous" = platform; then
        printf 'platform=%s\n' "$argument" \
            >> "$XPILOT_PACKAGE_CONTAINER_LOG"
        previous=
        continue
    fi
    case "$argument" in
        -v) previous=volume ;;
        -e) previous=environment ;;
        --platform) previous=platform ;;
        localhost/*) printf 'run-image=%s\n' "$argument" \
            >> "$XPILOT_PACKAGE_CONTAINER_LOG" ;;
    esac
done

test -n "$workspace"
test -n "$work_dir"
host_work_dir="$workspace/${work_dir#/workspace/}"
mkdir -p "$host_work_dir/stage/xpilot-ng/DEBIAN"
printf 'Package: xpilot-ng\n' \
    > "$host_work_dir/stage/xpilot-ng/DEBIAN/control"
EOF

cat > "$fixture_tools/dpkg-deb" <<'EOF'
#!/bin/sh
set -eu

output_path=
for argument do
    output_path=$argument
done
mkdir -p "$(dirname -- "$output_path")"
printf 'fixture deb\n' > "$output_path"
printf '%s\n' "$*" >> "$XPILOT_PACKAGE_DPKG_LOG"
EOF

chmod +x "$fixture_tools/container-engine" "$fixture_tools/dpkg-deb"

PROJECT_ROOT=$fixture_project
ARTIFACT_ROOT="$fixture_project/artifacts"
DEB_ARTIFACT_ROOT="$ARTIFACT_ROOT/deb"
RUN_ID=test-run
TMP_ROOT="$ARTIFACT_ROOT/.tmp/$RUN_ID"
VERSION=4.7.99
CONTAINER_ENGINE_BIN="$fixture_tools/container-engine"
MAKE_JOBS=3
BUILD_TYPE=Release
export XPILOT_PACKAGE_CONTAINER_LOG=$fixture_log
export XPILOT_PACKAGE_DPKG_LOG=$fixture_dpkg_log
PATH="$fixture_tools:$PATH"
export PATH

build_deb_package debian bookworm x86_64 linux/amd64

assert_contains "$fixture_log" \
    "exists=localhost/xpilot-ng-pack-deb-debian-bookworm-x86_64:latest"
assert_contains "$fixture_log" "platform=linux/amd64"
assert_contains "$fixture_log" \
    "run-image=localhost/xpilot-ng-pack-deb-debian-bookworm-x86_64:latest"
assert_contains "$fixture_dpkg_log" \
    "$DEB_ARTIFACT_ROOT/xpilot-ng-4.7.99-debian-bookworm-amd64.deb"
test -f "$DEB_ARTIFACT_ROOT/xpilot-ng-4.7.99-debian-bookworm-amd64.deb" \
    || fail "the Debian artifact was not created"

all_args="$test_root/build-all.args"
cat > "$fixture_tools/build-package-stub" <<'EOF'
#!/bin/sh
set -eu
printf '%s\n' "$@" > "$XPILOT_PACKAGE_ALL_ARGS"
EOF
chmod +x "$fixture_tools/build-package-stub"

XPILOT_PACKAGE_ALL_ARGS=$all_args \
BUILD_PACKAGE_SCRIPT="$fixture_tools/build-package-stub" \
    "$package_all_script" --arch amd64 --jobs 2

assert_equal "--target
all
--arch
amd64
--jobs
2" "$(cat "$all_args")" \
    "the complete package wrapper did not forward its arguments"

prereq_project="$test_root/prereq-project"
prereq_log="$test_root/prereq.log"
mkdir -p "$prereq_project"
ln -s "$package_script" "$prereq_project/build_package.sh"

cat > "$fixture_tools/prereq-container-engine" <<'EOF'
#!/bin/sh
set -eu

if test "${1:-}" = image && test "${2:-}" = exists; then
    exit 1
fi
test "${1:-}" = build || exit 2

containerfile=
previous=
for argument do
    if test "$previous" = containerfile; then
        containerfile=$argument
        previous=
        continue
    fi
    test "$argument" = -f && previous=containerfile
done

printf '%s\n' "$*" > "$XPILOT_PREREQ_TEST_LOG"
cat "$containerfile" >> "$XPILOT_PREREQ_TEST_LOG"
EOF
chmod +x "$fixture_tools/prereq-container-engine"

XPILOT_PREREQ_TEST_LOG=$prereq_log \
XPILOT_PREREQ_PROJECT_ROOT=$prereq_project \
CONTAINER_ENGINE="$fixture_tools/prereq-container-engine" \
    "$prereq_script" --distro debian --release bookworm \
    --arch amd64 --jobs 1

assert_contains "$prereq_log" "--platform linux/amd64"
assert_contains "$prereq_log" \
    "localhost/xpilot-ng-pack-deb-debian-bookworm-x86_64:latest"
assert_contains "$prereq_log" "dpkg-dev"
assert_contains "$prereq_log" "libfontconfig1-dev"
assert_contains "$prereq_log" "libgl-dev"
assert_contains "$prereq_log" "libxrender-dev"
assert_contains "$prereq_log" "libxtst-dev"

echo "Package build orchestration smoke passed"
