#!/bin/sh

set -eu

if test "${1:-}" != --inside-xvfb; then
    for command_name in xvfb-run xdotool node; do
        if ! command -v "$command_name" >/dev/null 2>&1; then
            echo "Missing E2E dependency: $command_name" >&2
            exit 1
        fi
    done
    if test -n "${XPILOT_E2E_SCREENSHOT_DIR:-}"; then
        for command_name in xwd ffmpeg; do
            if ! command -v "$command_name" >/dev/null 2>&1; then
                echo "Missing screenshot dependency: $command_name" >&2
                exit 1
            fi
        done
    fi

    case "${XPILOT_TEST_PREFIX:?XPILOT_TEST_PREFIX is required}" in
        /tmp/*|/var/tmp/*)
            ;;
        *)
            echo "SKIP: configure with a prefix below /tmp or /var/tmp for E2E" >&2
            exit 77
            ;;
    esac
    export LIBGL_ALWAYS_SOFTWARE=1
    export SDL_VIDEODRIVER=x11
    exec xvfb-run -a -s "-screen 0 1280x1024x24 +extension GLX +render -noreset" \
        /bin/sh "$0" --inside-xvfb
fi

client="${XPILOT_TEST_BINDIR:?XPILOT_TEST_BINDIR is required}/xpilot-ng-sdl"
server="${XPILOT_TEST_BINDIR}/xpilot-ng-server"
map="${XPILOT_TEST_PKGDATADIR:?XPILOT_TEST_PKGDATADIR is required}/maps/ndh.xp2"

for required_file in "$client" "$server" "$map"; do
    if test ! -r "$required_file"; then
        echo "Installed E2E input is missing: $required_file" >&2
        exit 1
    fi
done

runtime_dir=$(mktemp -d "${TMPDIR:-/tmp}/xpilot-sdl2-e2e.XXXXXX")
server_pid=
client_pid=
window_id=
window_owner_pid=

capture_window()
{
    capture_name=$1
    if test -z "${XPILOT_E2E_SCREENSHOT_DIR:-}"; then
        return 0
    fi
    capture_variant=${XPILOT_TEST_PREFIX##*/}
    capture_xwd="$runtime_dir/$capture_name.xwd"
    capture_png="$XPILOT_E2E_SCREENSHOT_DIR/$capture_variant-$capture_name.png"
    mkdir -p -- "$XPILOT_E2E_SCREENSHOT_DIR"
    xwd -silent -id "$window_id" -out "$capture_xwd" \
        || fail "could not capture the $capture_name window"
    ffmpeg -hide_banner -loglevel error -y -i "$capture_xwd" \
        "$capture_png" \
        || fail "could not encode the $capture_name screenshot"
}

cleanup()
{
    cleanup_deadline=$(($(date +%s) + 5))
    for process_id in "$client_pid" "$server_pid"; do
        if test -n "$process_id" && kill -0 "$process_id" 2>/dev/null; then
            kill -TERM "$process_id" 2>/dev/null || true
        fi
    done
    while :; do
        cleanup_running=0
	for process_id in "$client_pid" "$server_pid"; do
            if test -n "$process_id" \
                && kill -0 "$process_id" 2>/dev/null; then
                cleanup_running=1
            fi
        done
        if test "$cleanup_running" -eq 0; then
            break
        fi
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
        "${TMPDIR:-/tmp}"/xpilot-sdl2-e2e.*)
            rm -rf -- "$runtime_dir"
            ;;
    esac
}

dump_logs()
{
    for log_file in "$runtime_dir"/*.log; do
        if test -f "$log_file"; then
            echo "===== $log_file =====" >&2
            sed -n '1,240p' "$log_file" >&2
        fi
    done
}

fail()
{
    echo "E2E failure: $*" >&2
    dump_logs
    exit 1
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

core_context_logged()
{
    grep -Eq '^OpenGL context: .*, profile=core, attributes=[0-9]+\.[0-9]+$' \
        "$1" 2>/dev/null
}

server_ready()
{
    if ! kill -0 "$server_pid" 2>/dev/null; then
        fail "server stopped before becoming ready"
    fi
    grep -q "Server runs at" "$runtime_dir/server.log" 2>/dev/null
}

client_accepted()
{
    if ! kill -0 "$client_pid" 2>/dev/null; then
        fail "client stopped before joining the server"
    fi
    grep -q "Welcome .*SDL2Smoke" "$runtime_dir/server.log" 2>/dev/null
}

tcp_gameplay_connected()
{
    if ! kill -0 "$server_pid" 2>/dev/null; then
        fail "server stopped before accepting the TCP gameplay stream"
    fi
    grep -q "TCP gameplay connection established" \
        "$runtime_dir/server.log" 2>/dev/null
}

recorded_session_finished()
{
    grep -q "Goodbye SDL2Smoke" "$runtime_dir/server.log" 2>/dev/null
}

recorded_session_replayed()
{
    if grep -q "Welcome .*SDL2Smoke" \
        "$runtime_dir/playback.log" 2>/dev/null \
        && grep -q "Goodbye SDL2Smoke" \
            "$runtime_dir/playback.log" 2>/dev/null; then
        return 0
    fi
    if ! kill -0 "$server_pid" 2>/dev/null; then
        fail "playback server stopped before replaying the TCP session"
    fi
    return 1
}

game_frame_ready()
{
    if ! kill -0 "$client_pid" 2>/dev/null; then
        fail "client stopped before presenting a semantic game frame"
    fi
    grep -q '^Game frame ready: semantic=ok, presented=1$' \
        "$runtime_dir/client.log" 2>/dev/null
}

runtime_logs_have_no_gl_errors()
{
    if grep -q 'OpenGL error at ' "$runtime_dir"/*.log 2>/dev/null; then
        return 1
    fi
    return 0
}

find_game_window()
{
    window_id=
    for candidate_id in $(xdotool search --onlyvisible --name '^XPilot NG ' \
        2>/dev/null || true); do
        candidate_pid=$(xdotool getwindowpid "$candidate_id" 2>/dev/null \
            || true)
        if test "$candidate_pid" = "$window_owner_pid"; then
            window_id=$candidate_id
            return 0
        fi
    done
    return 1
}

process_window_absent()
{
    absent_pid=$1
    for candidate_id in $(xdotool search --name '^XPilot NG ' 2>/dev/null \
        || true); do
        candidate_pid=$(xdotool getwindowpid "$candidate_id" 2>/dev/null \
            || true)
        if test "$candidate_pid" = "$absent_pid"; then
            return 1
        fi
    done
    return 0
}

window_resized()
{
    xdotool getwindowgeometry --shell "$window_id" 2>/dev/null \
        | grep -q '^WIDTH=900$'
}

mkdir -p -- "$runtime_dir/textures"
printf 'xpilot.texturePath: %s:%s\n' \
    "$runtime_dir/textures" "$XPILOT_TEST_PKGDATADIR/textures" \
    >"$runtime_dir/xpilotrc"
export XPILOTRC="$runtime_dir/xpilotrc"

port=$(node -e '
const net = require("net");
const listener = net.createServer();
listener.listen(0, "127.0.0.1", () => {
  process.stdout.write(String(listener.address().port));
  listener.close();
});')

recording="$runtime_dir/gameplay.rec"
"$server" -map "$map" -port "$port" -noQuit \
    -recordMode 1 -recordFile "$recording" \
    >"$runtime_dir/server.log" 2>&1 &
server_pid=$!
wait_until "local server readiness" 20 server_ready

if ! "$client" -port "$port" -status \
    >"$runtime_dir/status.log" 2>&1; then
    fail "TCP status request failed"
fi
grep -q '^SERVER VERSION\.\.: ' "$runtime_dir/status.log" \
    || fail "TCP status response did not include the server version"
grep -q '^STATUS\.\.\.\.\.\.\.\.\.\.: ' "$runtime_dir/status.log" \
    || fail "TCP status response did not include the game status"

set +e
"$client" -port "$port" -user TCPGuest \
    -shutdown "unauthorized E2E shutdown" \
    >"$runtime_dir/unauthorized-control.log" 2>&1
unauthorized_status=$?
set -e
if test "$unauthorized_status" -eq 0; then
    fail "non-owner TCP shutdown request was accepted"
fi
grep -q '^permission denied$' "$runtime_dir/unauthorized-control.log" \
    || fail "non-owner TCP shutdown did not report permission denial"
kill -0 "$server_pid" 2>/dev/null \
    || fail "non-owner TCP shutdown stopped the server"

printf 'lock on\nstatus\nlock off\noptions\nquit\n' \
    | "$client" -port "$port" -text \
        >"$runtime_dir/text-control.log" 2>&1 \
    || fail "interactive TCP control session failed"
grep -q 'STATUS\.\.\.\.\.\.\.\.\.\.: locked' \
    "$runtime_dir/text-control.log" \
    || fail "interactive TCP lock command did not affect status"
grep -q '^framesPerSecond:50$' "$runtime_dir/text-control.log" \
    || fail "interactive TCP option listing was incomplete"

"$client" -geometry 800x600 -port "$port" -name SDL2Smoke \
    >"$runtime_dir/client.log" 2>&1 &
client_pid=$!
window_owner_pid=$client_pid
wait_until "SDL game window" 20 find_game_window
wait_until "local client acceptance" 20 client_accepted
wait_until "TCP gameplay connection" 10 tcp_gameplay_connected
wait_until "game core OpenGL context diagnostics" 10 \
    core_context_logged "$runtime_dir/client.log"
wait_until "game text renderers" 10 \
    grep -q '^Font text renderers ready: game=renderer map=renderer$' \
        "$runtime_dir/client.log"
wait_until "successful semantic game frame presentation" 20 \
    game_frame_ready
test -r "$runtime_dir/textures/ndh-1.3/bakedmud.pnm" \
    || fail "bundled map data was not extracted into the configured texture path"
test ! -e "$XPILOT_TEST_PKGDATADIR/textures/ndh-1.3.xpd" \
    || fail "bundled map data modified the installed texture directory"

xdotool windowsize "$window_id" 900 700 >/dev/null 2>&1 \
    || fail "could not request an SDL window resize"
wait_until "SDL window resize" 10 window_resized
xdotool keydown --window "$window_id" Shift_L >/dev/null 2>&1 \
    || fail "could not send key-down event"
xdotool keyup --window "$window_id" Shift_L >/dev/null 2>&1 \
    || fail "could not send key-up event"
xdotool key --window "$window_id" Up Return >/dev/null 2>&1 \
    || fail "could not send gameplay key events"
xdotool key --window "$window_id" m >/dev/null 2>&1 \
    || fail "could not open the console"
xdotool type --window "$window_id" --delay 10 SDL2Smoke >/dev/null 2>&1 \
    || fail "could not enter console text"
xdotool key --window "$window_id" Return >/dev/null 2>&1 \
    || fail "could not submit console text"
if ! kill -0 "$client_pid" 2>/dev/null; then
    fail "client stopped after resize/input events"
fi
capture_window game

xdotool key --window "$window_id" Escape y >/dev/null 2>&1 \
    || fail "could not request a graceful client quit"
wait_until "graceful client shutdown" 15 process_stopped "$client_pid"
set +e
wait "$client_pid"
client_status=$?
set -e
client_pid=
if test "$client_status" -ne 0; then
    fail "client returned status $client_status"
fi
if ! runtime_logs_have_no_gl_errors; then
    fail "OpenGL diagnostics reported a runtime error"
fi
wait_until "recorded client disconnect" 10 recorded_session_finished

if ! "$client" -port "$port" -shutdown "E2E control shutdown" \
    >"$runtime_dir/shutdown.log" 2>&1; then
    fail "owner TCP shutdown request failed"
fi
grep -q '^accepted$' "$runtime_dir/shutdown.log" \
    || fail "owner TCP shutdown did not report success"
wait_until "server shutdown" 10 process_stopped "$server_pid"
wait "$server_pid" 2>/dev/null || true
server_pid=
test -s "$recording" || fail "server did not produce a gameplay recording"

playback_port=$(node -e '
const net = require("net");
const listener = net.createServer();
listener.listen(0, "127.0.0.1", () => {
  process.stdout.write(String(listener.address().port));
  listener.close();
});')
"$server" -map "$map" -port "$playback_port" -noQuit \
    -recordMode 2 -recordFile "$recording" \
    >"$runtime_dir/playback.log" 2>&1 &
server_pid=$!
wait_until "recorded TCP gameplay session" 20 recorded_session_replayed
kill -TERM "$server_pid" 2>/dev/null || true
wait_until "playback server shutdown" 10 process_stopped "$server_pid"
wait "$server_pid" 2>/dev/null || true
server_pid=

echo "SDL2 E2E smoke passed"
