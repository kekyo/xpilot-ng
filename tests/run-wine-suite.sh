#!/bin/sh

set -eu

usage()
{
    cat <<'EOF'
Usage: tests/run-wine-suite.sh --build-dir PATH --package-dir PATH \
       --wine-prefix PATH --arch ARCH [--jobs NUMBER]

Build and run the supported Windows unit tests, then exercise UDP and TCP
gameplay with the packaged server and SDL client under Wine and Xvfb.
EOF
}

fail()
{
    echo "Wine test failure: $*" >&2
    if command -v dump_logs >/dev/null 2>&1; then
	dump_logs
    fi
    exit 1
}

build_dir=
package_dir=
wine_prefix=
architecture=
jobs=
inside_xvfb=false

while test "$#" -gt 0; do
    case "$1" in
        --build-dir)
            test "$#" -ge 2 || { usage >&2; exit 1; }
            build_dir=$2
            shift 2
            ;;
        --package-dir)
            test "$#" -ge 2 || { usage >&2; exit 1; }
            package_dir=$2
            shift 2
            ;;
        --wine-prefix)
            test "$#" -ge 2 || { usage >&2; exit 1; }
            wine_prefix=$2
            shift 2
            ;;
        --arch)
            test "$#" -ge 2 || { usage >&2; exit 1; }
            architecture=$2
            shift 2
            ;;
        --jobs)
            test "$#" -ge 2 || { usage >&2; exit 1; }
            jobs=$2
            shift 2
            ;;
        --inside-xvfb)
            inside_xvfb=true
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        *)
            usage >&2
            exit 1
            ;;
    esac
done

test -n "$build_dir" || { usage >&2; exit 1; }
test -n "$package_dir" || { usage >&2; exit 1; }
test -n "$wine_prefix" || { usage >&2; exit 1; }
case "$architecture" in
    x86)
        wine_architecture=win32
        triplet=i686-w64-mingw32
        ;;
    x86_64)
        wine_architecture=win64
        triplet=x86_64-w64-mingw32
        ;;
    *) fail "--arch must be x86 or x86_64" ;;
esac
if test -z "$jobs"; then
    jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
fi
case "$jobs" in
    ''|0|*[!0-9]*) fail "--jobs must be a positive integer" ;;
esac

case "$build_dir" in
    /*) ;;
    *) fail "--build-dir must be an absolute path" ;;
esac
case "$package_dir" in
    /*) ;;
    *) fail "--package-dir must be an absolute path" ;;
esac
case "$wine_prefix" in
    /*) ;;
    *) fail "--wine-prefix must be an absolute path" ;;
esac

make_program=${MAKE:-make}
wine_program=${WINE:-wine}
wineboot_program=${WINEBOOT:-wineboot}

if test "$inside_xvfb" = false; then
    for required_command in "$make_program" "$wine_program" \
        "$wineboot_program" wineserver xvfb-run xdotool node file timeout; do
        command -v "$required_command" >/dev/null 2>&1 \
            || fail "required command was not found: $required_command"
    done
    command -v "$triplet-gcc" >/dev/null 2>&1 \
        || fail "required command was not found: $triplet-gcc"

    echo "===== build: supported Windows tests for $architecture ====="
    "$make_program" -C "$build_dir/tests" "-j$jobs" \
        test-framed-stream.exe \
        test-game-transport.exe \
        test-socket-io.exe \
        test-sdl-versions.exe \
        test-native-socket-handle.exe

    mkdir -p "$(dirname -- "$wine_prefix")"
    export WINEPREFIX=$wine_prefix
    export WINEARCH=$wine_architecture
    export WINEDEBUG=-all
    export LIBGL_ALWAYS_SOFTWARE=1
    exec xvfb-run -a \
        -s "-screen 0 1280x1024x24 +extension GLX +render -noreset" \
        /bin/sh "$0" \
            --inside-xvfb \
            --build-dir "$build_dir" \
            --package-dir "$package_dir" \
            --wine-prefix "$wine_prefix" \
            --arch "$architecture" \
            --jobs "$jobs"
fi

export WINEPREFIX=$wine_prefix
export WINEARCH=$wine_architecture
export WINEDEBUG=-all
export LIBGL_ALWAYS_SOFTWARE=1

runtime_dir=$(mktemp -d \
    "${TMPDIR:-/tmp}/xpilot-wine-$architecture.XXXXXX")
runtime_package="$runtime_dir/package"
server_pid=
client_pid=
server_log=
client_log=
window_id=

dump_logs()
{
    if test -n "${runtime_dir:-}" && test -d "$runtime_dir"; then
        for log_file in "$runtime_dir"/*.log; do
            if test -f "$log_file"; then
                echo "===== $log_file =====" >&2
                sed -n '1,240p' "$log_file" >&2
            fi
        done
    fi
}

cleanup()
{
    cleanup_deadline=$(($(date +%s) + 10))
    for process_id in "$client_pid" "$server_pid"; do
        if test -n "$process_id" && kill -0 "$process_id" 2>/dev/null; then
            kill -TERM "$process_id" 2>/dev/null || true
        fi
    done
    while :; do
        cleanup_running=false
        for process_id in "$client_pid" "$server_pid"; do
            if test -n "$process_id" \
                && kill -0 "$process_id" 2>/dev/null; then
                cleanup_running=true
            fi
        done
        test "$cleanup_running" = false && break
        if test "$(date +%s)" -ge "$cleanup_deadline"; then
            for process_id in "$client_pid" "$server_pid"; do
                if test -n "$process_id" \
                    && kill -0 "$process_id" 2>/dev/null; then
                    kill -KILL "$process_id" 2>/dev/null || true
                fi
            done
            break
        fi
        sleep 0.1
    done
    for process_id in "$client_pid" "$server_pid"; do
        if test -n "$process_id"; then
            wait "$process_id" 2>/dev/null || true
        fi
    done
    case "$runtime_dir" in
        "${TMPDIR:-/tmp}"/xpilot-wine-*) rm -rf -- "$runtime_dir" ;;
    esac
}

trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

wait_until()
{
    wait_description=$1
    wait_seconds=$2
    shift 2
    wait_deadline=$(($(date +%s) + wait_seconds))
    while ! "$@"; do
        if test "$(date +%s)" -ge "$wait_deadline"; then
            fail "timed out waiting for $wait_description"
        fi
        sleep 0.1
    done
}

process_stopped()
{
    ! kill -0 "$1" 2>/dev/null
}

server_ready()
{
    if ! kill -0 "$server_pid" 2>/dev/null; then
        fail "server stopped before becoming ready"
    fi
    grep -q 'Server runs at' "$server_log" 2>/dev/null
}

client_joined()
{
    if ! kill -0 "$client_pid" 2>/dev/null; then
        fail "client stopped before joining the server"
    fi
    grep -q '\*\*\* Login allowed\.' "$client_log" 2>/dev/null \
        && grep -q "Welcome .*$client_name" "$server_log" 2>/dev/null
}

find_game_window()
{
    window_id=$(xdotool search --onlyvisible --name '^XPilot NG ' \
        2>/dev/null | tail -n 1 || true)
    test -n "$window_id"
}

client_departed()
{
    grep -q "Goodbye .*$client_name" "$server_log" 2>/dev/null
}

reserve_contact_port()
{
    node -e '
const socket = require("dgram").createSocket("udp4");
socket.bind(0, "127.0.0.1", () => {
  process.stdout.write(String(socket.address().port));
  socket.close();
});'
}

run_wine_unit_test()
{
    test_name=$1
    test_executable="$build_dir/tests/$test_name.exe"
    test_log="$runtime_dir/$test_name.log"
    test -f "$test_executable" \
        || fail "Windows test executable is missing: $test_executable"
    if ! timeout 30s "$wine_program" "$test_executable" \
        >"$test_log" 2>&1; then
        fail "$test_name failed under Wine"
    fi
}

stop_server()
{
    kill -TERM "$server_pid" 2>/dev/null || true
    wait_until "server shutdown" 15 process_stopped "$server_pid"
    wait "$server_pid" 2>/dev/null || true
    server_pid=
}

run_gameplay_case()
{
    gameplay_transport=$1
    contact_port=$(reserve_contact_port)
    server_log="$runtime_dir/server-$gameplay_transport.log"
    client_log="$runtime_dir/client-$gameplay_transport.log"
    case "$architecture:$gameplay_transport" in
	x86:udp) client_name=W32UDP ;;
	x86:tcp) client_name=W32TCP ;;
	x86_64:udp) client_name=W64UDP ;;
	x86_64:tcp) client_name=W64TCP ;;
    esac

    (
        cd "$runtime_package"
        exec "$wine_program" ./xpilot-ng-server.exe \
            -map lib/maps/ndh.xp2 \
            -port "$contact_port" \
            -noQuit +reportMeta \
            -gameTransport "$gameplay_transport"
    ) >"$server_log" 2>&1 &
    server_pid=$!
    wait_until "$gameplay_transport server readiness" 30 server_ready

    (
        cd "$runtime_package"
        exec "$wine_program" ./xpilot-ng-sdl.exe \
            -geometry 800x600 \
            -join \
            -port "$contact_port" \
            -name "$client_name" \
            -gameTransport "$gameplay_transport" \
            127.0.0.1
    ) >"$client_log" 2>&1 &
    client_pid=$!

    wait_until "$gameplay_transport client login" 30 client_joined
    wait_until "$gameplay_transport SDL window" 30 find_game_window
    wait_until "$gameplay_transport OpenGL context" 30 \
        grep -q '^OpenGL context:' "$client_log"
    wait_until "$gameplay_transport text renderers" 30 \
        grep -q '^Font text renderers ready: game=renderer map=renderer' \
            "$client_log"

    xdotool key --clearmodifiers --window "$window_id" Escape y \
        >/dev/null 2>&1 \
        || fail "could not request a graceful $gameplay_transport client quit"
    wait_until "$gameplay_transport client shutdown" 20 \
        process_stopped "$client_pid"
    set +e
    wait "$client_pid"
    client_status=$?
    set -e
    client_pid=
    test "$client_status" -eq 0 \
        || fail "$gameplay_transport client returned status $client_status"
    wait_until "$gameplay_transport server departure" 10 client_departed

    if grep -q 'accept error' "$server_log"; then
        fail "$gameplay_transport server reported an accept failure"
    fi
    stop_server
    window_id=
}

server_executable="$package_dir/xpilot-ng-server.exe"
client_executable="$package_dir/xpilot-ng-sdl.exe"
map_file="$package_dir/lib/maps/ndh.xp2"
for required_file in "$server_executable" "$client_executable" "$map_file"; do
    test -f "$required_file" \
        || fail "packaged Windows test input is missing: $required_file"
done

case "$architecture" in
    x86)
        file "$server_executable" | grep -q 'PE32 executable' \
            || fail "x86 server is not a PE32 executable"
        file "$server_executable" | grep -qv 'PE32+' \
            || fail "x86 server unexpectedly uses PE32+"
        ;;
    x86_64)
        file "$server_executable" | grep -q 'PE32+ executable' \
            || fail "x86_64 server is not a PE32+ executable"
        ;;
esac

mkdir -p "$runtime_package"
cp -R "$package_dir/." "$runtime_package/"

echo "===== initialize: Wine $architecture prefix ====="
timeout 60s "$wineboot_program" -u >"$runtime_dir/wineboot.log" 2>&1 \
    || fail "Wine prefix initialization failed"
timeout 30s wineserver -w >>"$runtime_dir/wineboot.log" 2>&1 \
    || fail "Wine prefix initialization did not settle"

for unit_test in test-framed-stream test-game-transport test-socket-io \
    test-sdl-versions test-native-socket-handle; do
    run_wine_unit_test "$unit_test"
done

run_gameplay_case udp
run_gameplay_case tcp

echo "Wine $architecture unit and UDP/TCP integration tests passed"
