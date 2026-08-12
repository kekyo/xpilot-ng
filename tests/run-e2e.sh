#!/bin/sh

set -eu

if test "${1:-}" != --inside-xvfb; then
    for command_name in xvfb-run xdotool node; do
        if ! command -v "$command_name" >/dev/null 2>&1; then
            echo "Missing E2E dependency: $command_name" >&2
            exit 1
        fi
    done

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
map="${XPILOT_TEST_PKGDATADIR:?XPILOT_TEST_PKGDATADIR is required}/maps/circle2.xp2"

for required_file in "$client" "$server" "$map"; do
    if test ! -r "$required_file"; then
        echo "Installed E2E input is missing: $required_file" >&2
        exit 1
    fi
done

runtime_dir=$(mktemp -d "${TMPDIR:-/tmp}/xpilot-sdl2-e2e.XXXXXX")
meta_pid=
meta_fixture_pid=
server_pid=
client_pid=
window_id=
window_owner_pid=

cleanup()
{
    cleanup_deadline=$(($(date +%s) + 5))
    for process_id in "$client_pid" "$meta_pid" "$meta_fixture_pid" \
        "$server_pid"; do
        if test -n "$process_id" && kill -0 "$process_id" 2>/dev/null; then
            kill -TERM "$process_id" 2>/dev/null || true
        fi
    done
    while :; do
        cleanup_running=0
        for process_id in "$client_pid" "$meta_pid" "$meta_fixture_pid" \
            "$server_pid"; do
            if test -n "$process_id" \
                && kill -0 "$process_id" 2>/dev/null; then
                cleanup_running=1
            fi
        done
        if test "$cleanup_running" -eq 0; then
            break
        fi
        if test "$(date +%s)" -ge "$cleanup_deadline"; then
            for process_id in "$client_pid" "$meta_pid" \
                "$meta_fixture_pid" "$server_pid"; do
                if test -n "$process_id" \
                    && kill -0 "$process_id" 2>/dev/null; then
                    kill -KILL "$process_id" 2>/dev/null || true
                fi
            done
            break
        fi
        sleep 0.1
    done
    for process_id in "$client_pid" "$meta_pid" "$meta_fixture_pid" \
        "$server_pid"; do
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

meta_initialized()
{
    grep -q "SDL_ttf initialized" "$runtime_dir/meta.log" 2>/dev/null \
        && grep -q '^OpenGL context:' "$runtime_dir/meta.log" 2>/dev/null
}

meta_ui_ready()
{
    if ! kill -0 "$meta_pid" 2>/dev/null; then
        fail "client stopped before the metaserver UI was drawn"
    fi
    grep -q '^Metaserver UI ready: background=semantic, buttons=3/3, draws=4$' \
        "$runtime_dir/meta.log" 2>/dev/null
}

meta_fixture_ready()
{
    if ! kill -0 "$meta_fixture_pid" 2>/dev/null; then
        fail "local metaserver fixture stopped before listening"
    fi
    test -s "$runtime_dir/meta-fixture.port"
}

meta_fixture_served()
{
    test -s "$runtime_dir/meta-fixture.served"
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

: >"$runtime_dir/xpilotrc"
export XPILOTRC="$runtime_dir/xpilotrc"

node -e '
const fs = require("fs");
const net = require("net");
const portFile = process.argv[1];
const servedFile = process.argv[2];
const response = Array.from({ length: 12 }, (_, index) => [
  "4.7.3",
  `fixture${index}.local`,
  String(15000 + index),
  String(index),
  "fixture-map",
  "100x100",
  "test-author",
  "ok",
  "10",
  "20",
  "-",
  "no",
  "100",
  "0",
  "0",
  `127.0.0.${index + 1}`,
  "10",
  "0",
].join(":"))
  .join("\n") + "\n";
let responseSent = false;
const server = net.createServer((socket) => {
  fs.appendFileSync(servedFile, "served\n");
  if (responseSent) {
    socket.end();
    return;
  }
  responseSent = true;
  socket.end(response);
});
server.listen(0, "127.0.0.1", () => {
  fs.writeFileSync(portFile, String(server.address().port));
});
const stop = () => server.close(() => process.exit(0));
process.on("SIGTERM", stop);
process.on("SIGINT", stop);
' "$runtime_dir/meta-fixture.port" "$runtime_dir/meta-fixture.served" \
    >"$runtime_dir/meta-fixture.log" 2>&1 &
meta_fixture_pid=$!
wait_until "local metaserver fixture" 10 meta_fixture_ready
export XPILOT_META_HOST=127.0.0.1
export XPILOT_META_HOST_TWO=127.0.0.1
XPILOT_META_PORT=$(sed -n '1p' "$runtime_dir/meta-fixture.port")
export XPILOT_META_PORT

# With no arguments the SDL client takes the graphical metaserver path.  This
# scenario requires a completed metaserver fetch so it exercises the actual
# semantic background and button draw, presentation, and graceful teardown.
"$client" >"$runtime_dir/meta.log" 2>&1 &
meta_pid=$!
window_owner_pid=$meta_pid
wait_until "no-argument SDL initialization" 15 meta_initialized
wait_until "semantic metaserver UI" 20 meta_ui_ready
wait_until "local metaserver request" 5 meta_fixture_served
find_game_window || fail "metaserver window was not visible"
kill -0 "$meta_pid" 2>/dev/null \
    || fail "metaserver client exited before Escape"
meta_deadline=$(($(date +%s) + 20))
meta_escape_sent=false
while kill -0 "$meta_pid" 2>/dev/null \
    && test "$(date +%s)" -lt "$meta_deadline"; do
    if find_game_window \
        && xdotool key --clearmodifiers --window "$window_id" Escape \
            >/dev/null 2>&1; then
        meta_escape_sent=true
    fi
    sleep 0.1
done
$meta_escape_sent || fail "could not send Escape to the metaserver UI"
kill -0 "$meta_pid" 2>/dev/null \
    && fail "metaserver UI did not close after Escape"
finished_meta_pid=$meta_pid
set +e
wait "$meta_pid"
meta_status=$?
set -e
meta_pid=
window_id=
if test "$meta_status" -ne 0; then
    fail "metaserver client returned status $meta_status"
fi
wait_until "metaserver window teardown" 5 process_window_absent \
    "$finished_meta_pid"

port=$(node -e '
const socket = require("dgram").createSocket("udp4");
socket.bind(0, "127.0.0.1", () => {
  process.stdout.write(String(socket.address().port));
  socket.close();
});')

"$server" -map "$map" -port "$port" -noQuit +reportMeta \
    >"$runtime_dir/server.log" 2>&1 &
server_pid=$!
wait_until "local server readiness" 20 server_ready

"$client" -geometry 800x600 -join -port "$port" -name SDL2Smoke \
    127.0.0.1 >"$runtime_dir/client.log" 2>&1 &
client_pid=$!
window_owner_pid=$client_pid
wait_until "SDL game window" 20 find_game_window
wait_until "local client acceptance" 20 client_accepted
wait_until "game OpenGL context diagnostics" 10 \
    grep -q '^OpenGL context:' "$runtime_dir/client.log"

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

kill -TERM "$server_pid" 2>/dev/null || true
wait_until "server shutdown" 10 process_stopped "$server_pid"
wait "$server_pid" 2>/dev/null || true
server_pid=

echo "SDL2 E2E smoke passed"
