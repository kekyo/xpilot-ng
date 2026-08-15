#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)

if test ! -x "$source_dir/configure"; then
    echo "Missing generated configure script; run autoreconf -fi first" >&2
    exit 1
fi

suite_dir=$(mktemp -d "${TMPDIR:-/tmp}/xpilot-sdl2-suite.XXXXXX")
build_source_dir=$source_dir

cleanup()
{
    case "$suite_dir" in
        "${TMPDIR:-/tmp}"/xpilot-sdl2-suite.*)
            rm -rf -- "$suite_dir"
            ;;
    esac
}

trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

# Autoconf rejects an out-of-tree configure when the source checkout already
# has an in-tree config.status.  A generated source-tree version.h can also
# hide a missing build-tree include path.  Preserve the checkout and exercise
# a temporary source snapshot without either artifact in both cases.
if test -f "$source_dir/config.status" \
    || test -f "$source_dir/src/common/version.h"; then
    build_source_dir="$suite_dir/source"
    mkdir -p "$build_source_dir"
    cp -a "$source_dir/." "$build_source_dir/"
    # A copied in-tree Makefile retains the original absolute source path, so
    # invoking its distclean target can reconfigure the original checkout.
    # Remove copied build products directly inside the guarded temporary tree;
    # otherwise VPATH can mistake stale objects for out-of-tree prerequisites.
    rm -f -- "$build_source_dir/config.status" \
        "$build_source_dir/config.cache" "$build_source_dir/config.log" \
        "$build_source_dir/config.h" "$build_source_dir/stamp-h1" \
        "$build_source_dir/src/common/version.h" \
        "$build_source_dir/src/client/sdl/xpilot-ng-sdl" \
        "$build_source_dir/src/server/xpilot-ng-server"
    find "$build_source_dir" -type f \
        \( -name Makefile -o -name '*.o' -o -name '*.a' \) -delete
    find "$build_source_dir" -depth -type d -name .deps -exec rm -rf -- {} \;
fi

if test -n "${XPILOT_TEST_JOBS:-}"; then
    test_jobs=$XPILOT_TEST_JOBS
else
    test_jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
fi

assert_removed_programs_absent()
{
    build_dir=$1

    for removed_program in \
        src/client/x11/xpilot-ng-x11 \
        src/replay/xpilot-ng-replay \
        src/mapedit/xpilot-ng-xp-mapedit
    do
        if test -e "$build_dir/$removed_program"; then
            echo "Unexpected legacy program: $removed_program" >&2
            return 1
        fi
    done
}

assert_removed_install_entries_absent()
{
    install_prefix=$1

    for removed_entry in \
        bin/xpilot-ng-x11 \
        bin/xpilot-ng-replay \
        bin/xpilot-ng-xp-mapedit \
        share/man/man6/xpilot-ng-x11.6 \
        share/man/man6/xpilot-ng-replay.6 \
        share/man/man6/xpilot-ng-xp-mapedit.6 \
        share/xpilot-ng/textures/metabtndown.png \
        share/xpilot-ng/textures/metabtnup.png \
        share/xpilot-ng/textures/sdlmetabg.png
    do
        if test -e "$install_prefix/$removed_entry"; then
            echo "Unexpected legacy install entry: $removed_entry" >&2
            return 1
        fi
    done
}

assert_configuration_programs()
{
    configuration_name=$1
    build_dir=$2
    install_prefix=$3

    for required_program in \
        src/server/xpilot-ng-server \
        "$install_prefix/bin/xpilot-ng-server"
    do
        case "$required_program" in
            /*) program_path=$required_program ;;
            *) program_path="$build_dir/$required_program" ;;
        esac
        if test ! -x "$program_path"; then
            echo "Missing required program: $program_path" >&2
            return 1
        fi
    done

    for sdl_program in \
        "$build_dir/src/client/sdl/xpilot-ng-sdl" \
        "$install_prefix/bin/xpilot-ng-sdl"
    do
        if test "$configuration_name" = default; then
            if test ! -x "$sdl_program"; then
                echo "Missing SDL client: $sdl_program" >&2
                return 1
            fi
        elif test -e "$sdl_program"; then
            echo "Unexpected SDL client in server-only build: $sdl_program" >&2
            return 1
        fi
    done

    server_man="$install_prefix/share/man/man6/xpilot-ng-server.6"
    if test ! -f "$server_man"; then
        echo "Missing server manual: $server_man" >&2
        return 1
    fi

    sdl_man="$install_prefix/share/man/man6/xpilot-ng-sdl.6"
    if test "$configuration_name" = default; then
        if test ! -f "$sdl_man"; then
            echo "Missing SDL client manual: $sdl_man" >&2
            return 1
        fi
    elif test -e "$sdl_man"; then
        echo "Unexpected SDL client manual in server-only install: $sdl_man" >&2
        return 1
    fi
}

assert_runtime_help_surface()
{
    build_dir=$1

    server_help=$("$build_dir/src/server/xpilot-ng-server" -help 2>&1 || true)
    case "$server_help" in
        *xpilot-ng-x11*)
            echo "Server help still references the removed X11 client" >&2
            return 1
            ;;
    esac

    client_help=$("$build_dir/src/client/sdl/xpilot-ng-sdl" -help 2>&1 || true)
    for removed_option in keyToggleRecord keyToggleRadarScore -join -list
    do
        case "$client_help" in
            *"$removed_option"*)
                echo "SDL client help still exposes $removed_option" >&2
                return 1
                ;;
        esac
    done

    case "$client_help" in
        *"welcome screen where you can select a server"*)
            echo "SDL client help still describes the removed server selector" >&2
            return 1
            ;;
        *"connects directly to 127.0.0.1"*)
            ;;
        *)
            echo "SDL client help does not describe its default direct connection" >&2
            return 1
            ;;
    esac

    server_help=$($build_dir/src/server/xpilot-ng-server -help 2>&1 || true)
    for removed_option in \
        contactPort reportToMetaServer metaUpdateMaxSize \
        searchDomainForXPilot clientPortStart clientPortEnd
    do
        case "$server_help" in
            *"$removed_option"*)
                echo "Server help still exposes $removed_option" >&2
                return 1
                ;;
        esac
    done
}

assert_distribution_surface()
{
    build_dir=$1

    (
        cd "$build_dir"
        make dist

        set -- xpilot-ng-*.tar.gz
        if test "$#" -ne 1 || test ! -f "$1"; then
            echo "Expected one source distribution archive" >&2
            return 1
        fi

        distribution_listing=$(tar -tzf "$1")
        for legacy_path in \
            XPilot.dsw \
            src/client/NT/ \
            src/client/items/ \
            src/common/NT/XPilotFiles \
            src/common/NT/bindist/ \
            src/common/NT/winNet.c \
            src/common/NT/winX \
            src/common/NT/wsockerrs.c \
            src/server/NT/ \
            src/client/sdl/res/ \
            src/client/sdl/xpilot.rc \
            src/client/sdl/xpilot_sdl.dsp \
            src/client/sdl/xpilot_sdl.dsw \
            contrib/xpngcc/ \
            src/client/datagram.c \
            src/client/datagram.h \
            src/client/meta.c \
            src/client/meta.h \
            src/client/meta_test_support.h \
            src/client/query.c \
            src/client/textinterface.c \
            src/common/metaserver.h \
            src/client/sdl/sdlmeta.c \
            src/client/sdl/sdlmeta.h \
            src/client/sdl/sdlmetaframe.c \
            src/client/sdl/sdlmetaframe.h \
            src/server/metaserver.c \
            tests/test_meta_environment.c \
            tests/test_sdl_meta_frame.c \
            tests/test_sdlmeta_primitives.c \
            lib/textures/metabtndown.png \
            lib/textures/metabtnup.png \
            lib/textures/sdlmetabg.png
        do
            case "$distribution_listing" in
                *"/$legacy_path"*)
                    echo "Unexpected legacy distribution path: $legacy_path" >&2
                    return 1
                    ;;
            esac
        done
    )
}

assert_configure_surface()
{
    configure_help=$("$build_source_dir/configure" --help)

    for removed_option in \
        --enable-dbe \
        --enable-mbx \
        --disable-x11-client \
        --disable-replay \
        --disable-xp-mapedit
    do
        case "$configure_help" in
            *"$removed_option"*)
                echo "Unexpected legacy configure option: $removed_option" >&2
                return 1
                ;;
        esac
    done
}

assert_source_platform_surface()
{
    legacy_x11_includes=$(grep -R -n -E \
        --include='*.c' --include='*.h' --include='*.m' \
        '#[[:space:]]*include[[:space:]]*[<"]X11/' \
        "$build_source_dir/src" || true)

    if test -n "$legacy_x11_includes"; then
        echo "XPilot source still includes X11 directly:" >&2
        echo "$legacy_x11_includes" >&2
        return 1
    fi
}

run_configuration()
{
    configuration_name=$1
    shift
    build_dir="$suite_dir/build-$configuration_name"
    install_prefix="$suite_dir/prefix-$configuration_name"
    mkdir -p "$build_dir" "$install_prefix"

    echo "===== configure: $configuration_name ====="
    (
        cd "$build_dir"
        "$build_source_dir/configure" --prefix="$install_prefix" "$@"
        make -j"$test_jobs"
        assert_removed_programs_absent "$build_dir"
        if test "$configuration_name" = default; then
            assert_runtime_help_surface "$build_dir"
            assert_distribution_surface "$build_dir"
        fi
        if ! make check; then
            for test_log in tests/*.log; do
                if test -f "$test_log"; then
                    echo "===== $test_log =====" >&2
                    sed -n '1,320p' "$test_log" >&2
                fi
            done
            exit 1
        fi
        make install
        assert_removed_install_entries_absent "$install_prefix"
        assert_configuration_programs \
            "$configuration_name" "$build_dir" "$install_prefix"
    )
}

# Every test is run through make check; the runner never selects an individual
# test binary.  Separate build and install trees also catch source-tree leaks.
assert_source_platform_surface
run_configuration default
run_configuration server-only --disable-sdl-client
assert_configure_surface

echo "SDL2 client and server-only configurations passed"
