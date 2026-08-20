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

runtime_dir=$(mktemp -d "${TMPDIR:-/tmp}/xpilot-sdl3-e2e.XXXXXX")
meta_pid=
meta_fixture_pid=
meta_report_fixture_pid=
server_pid=
client_pid=
window_id=
window_owner_pid=
game_server_log=
game_client_log=
game_client_name=

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
    for process_id in "$client_pid" "$meta_pid" "$meta_fixture_pid" \
        "$meta_report_fixture_pid" "$server_pid"; do
        if test -n "$process_id" && kill -0 "$process_id" 2>/dev/null; then
            kill -TERM "$process_id" 2>/dev/null || true
        fi
    done
    while :; do
        cleanup_running=0
        for process_id in "$client_pid" "$meta_pid" "$meta_fixture_pid" \
            "$meta_report_fixture_pid" "$server_pid"; do
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
                "$meta_fixture_pid" "$meta_report_fixture_pid" \
                "$server_pid"; do
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
        "$meta_report_fixture_pid" "$server_pid"; do
        if test -n "$process_id"; then
            wait "$process_id" 2>/dev/null || true
        fi
    done
    case "$runtime_dir" in
        "${TMPDIR:-/tmp}"/xpilot-sdl3-e2e.*)
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

meta_initialized()
{
    grep -q "SDL_ttf initialized" "$runtime_dir/meta.log" 2>/dev/null \
        && core_context_logged "$runtime_dir/meta.log"
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

meta_report_fixture_ready()
{
    if ! kill -0 "$meta_report_fixture_pid" 2>/dev/null; then
        fail "local metaserver report fixture stopped before listening"
    fi
    test -s "$runtime_dir/meta-report-fixture.port"
}

meta_tcp_transport_reported()
{
    test -s "$runtime_dir/meta-report-fixture.received" \
        && grep -q "^source-port $meta_report_contact_port$" \
            "$runtime_dir/meta-report-fixture.received" \
        && grep -q '^add version 4.7.3ng+ct=tcp+gt=udp$' \
            "$runtime_dir/meta-report-fixture.received"
}

server_ready()
{
    if ! kill -0 "$server_pid" 2>/dev/null; then
	fail "server stopped before becoming ready"
    fi
    grep -q "Server runs at" "$game_server_log" 2>/dev/null
}

client_accepted()
{
    if ! kill -0 "$client_pid" 2>/dev/null; then
	fail "client stopped before joining the server"
    fi
    grep -q "Welcome .*$game_client_name" "$game_server_log" 2>/dev/null
}

client_departed()
{
    grep -q "Goodbye .*$game_client_name" "$game_server_log" 2>/dev/null
}

game_frame_ready()
{
    if ! kill -0 "$client_pid" 2>/dev/null; then
	fail "client stopped before presenting a semantic game frame"
    fi
    grep -q '^Game frame ready: semantic=ok, presented=1$' \
	"$game_client_log" 2>/dev/null
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

process_has_connected_inet_socket()
{
    node -e '
const fs = require("fs");
const pid = process.argv[1];
const protocol = process.argv[2];
const socketInodes = new Set();
for (const fd of fs.readdirSync(`/proc/${pid}/fd`)) {
  try {
    const target = fs.readlinkSync(`/proc/${pid}/fd/${fd}`);
    const match = /^socket:\[([0-9]+)\]$/.exec(target);
    if (match) socketInodes.add(match[1]);
  } catch {
    // File descriptors may disappear while the process is running.
  }
}
const connected = fs.readFileSync(`/proc/net/${protocol}`, "utf8")
  .trim()
  .split("\n")
  .slice(1)
  .some((line) => {
    const fields = line.trim().split(/\s+/);
    return socketInodes.has(fields[9])
      && fields[2] !== "00000000:0000"
      && (protocol === "udp" || fields[3] === "01");
  });
process.exit(connected ? 0 : 1);
' "$1" "$2"
}

window_resized()
{
    xdotool getwindowgeometry --shell "$window_id" 2>/dev/null \
	| grep -q '^WIDTH=900$'
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

stop_local_server()
{
    kill -TERM "$server_pid" 2>/dev/null || true
    wait_until "server shutdown" 10 process_stopped "$server_pid"
    wait "$server_pid" 2>/dev/null || true
    server_pid=
}

run_recording_playback()
{
    playback_case=$1
    playback_transport=$2
    playback_recording=$3
    port=$(reserve_contact_port)
    game_server_log="$runtime_dir/server-$playback_case-playback.log"

    test -s "$playback_recording" \
	|| fail "$playback_case recording was not written"
    if test "$playback_transport" = default; then
	"$server" -map "$map" -port "$port" -noQuit +reportMeta \
	    -recordFileName "$playback_recording" -recordMode 2 \
	    >"$game_server_log" 2>&1 &
    else
	"$server" -map "$map" -port "$port" -noQuit +reportMeta \
	    -gameTransport "$playback_transport" \
	    -recordFileName "$playback_recording" -recordMode 2 \
	    >"$game_server_log" 2>&1 &
    fi
    server_pid=$!
    wait_until "$playback_case playback startup" 20 \
	grep -q "Server runs at" "$game_server_log"
    wait_until "$playback_case recorded join" 20 \
	grep -q "Welcome .*$game_client_name" "$game_server_log"
    wait_until "$playback_case recorded quit" 20 \
	grep -q "Goodbye .*$game_client_name" "$game_server_log"
    stop_local_server
}

run_gameplay_case()
{
    game_case=$1
    game_transport=$2
    game_capture=$3
    contact_transport=$4
    port=$(reserve_contact_port)
    game_recording="$runtime_dir/server-$game_case.xpr"
    game_server_log="$runtime_dir/server-$game_case.log"
    game_client_log="$runtime_dir/client-$game_case.log"
    case "$game_case" in
    tcp)
	game_client_name=SDL3TCP
	;;
    tcp-contact)
	game_client_name=SDL3TCPContact
	;;
    tcp-contact-udp-game)
	game_client_name=SDL3TCPUDP
	;;
    udp-default)
	game_client_name=SDL3UDPDefault
	;;
    udp-explicit)
	game_client_name=SDL3UDPOption
	;;
    esac

    if test "$game_transport" = default \
	&& test "$contact_transport" = default; then
	"$server" -map "$map" -port "$port" -noQuit +reportMeta \
	    -recordFileName "$game_recording" -recordMode 1 \
	    >"$game_server_log" 2>&1 &
    elif test "$contact_transport" = default; then
	"$server" -map "$map" -port "$port" -noQuit +reportMeta \
	    -gameTransport "$game_transport" \
	    -recordFileName "$game_recording" -recordMode 1 \
	    >"$game_server_log" 2>&1 &
    else
	"$server" -map "$map" -port "$port" -noQuit +reportMeta \
	    -gameTransport "$game_transport" \
	    -contactTransport "$contact_transport" \
	    -recordFileName "$game_recording" -recordMode 1 \
	    >"$game_server_log" 2>&1 &
    fi
    server_pid=$!
    wait_until "$game_case server readiness" 20 server_ready

    if test "$game_transport" = default \
	&& test "$contact_transport" = default; then
	"$client" -geometry 800x600 -join -port "$port" \
	    -name "$game_client_name" 127.0.0.1 >"$game_client_log" 2>&1 &
    elif test "$contact_transport" = default; then
	"$client" -geometry 800x600 -join -port "$port" \
	    -name "$game_client_name" -gameTransport "$game_transport" \
	    127.0.0.1 >"$game_client_log" 2>&1 &
    else
	"$client" -geometry 800x600 -join -port "$port" \
	    -name "$game_client_name" -gameTransport "$game_transport" \
	    -contactTransport "$contact_transport" \
	    127.0.0.1 >"$game_client_log" 2>&1 &
    fi
    client_pid=$!
    window_owner_pid=$client_pid
    wait_until "$game_case SDL game window" 20 find_game_window
    wait_until "$game_case local client acceptance" 20 client_accepted
    if test "$game_transport" = tcp; then
	game_socket_protocol=tcp
    else
	game_socket_protocol=udp
    fi
    wait_until "$game_case client $game_socket_protocol connection" 5 \
	process_has_connected_inet_socket "$client_pid" "$game_socket_protocol"
    wait_until "$game_case server $game_socket_protocol connection" 5 \
	process_has_connected_inet_socket "$server_pid" "$game_socket_protocol"
    wait_until "$game_case game core OpenGL context diagnostics" 10 \
	core_context_logged "$game_client_log"
    wait_until "$game_case game text renderers" 10 \
	grep -q '^Font text renderers ready: game=renderer map=renderer$' \
	    "$game_client_log"
    wait_until "$game_case semantic game frame presentation" 20 \
	game_frame_ready
    test -r "$runtime_dir/textures/ndh-1.3/bakedmud.pnm" \
	|| fail "bundled map data was not extracted for $game_case"
    test ! -e "$XPILOT_TEST_PKGDATADIR/textures/ndh-1.3.xpd" \
	|| fail "bundled map data modified the installed texture directory"

    xdotool windowsize "$window_id" 900 700 >/dev/null 2>&1 \
	|| fail "could not request an SDL window resize for $game_case"
    wait_until "$game_case SDL window resize" 10 window_resized
    xdotool keydown --window "$window_id" Shift_L >/dev/null 2>&1 \
	|| fail "could not send key-down event for $game_case"
    xdotool keyup --window "$window_id" Shift_L >/dev/null 2>&1 \
	|| fail "could not send key-up event for $game_case"
    xdotool key --window "$window_id" Up Return >/dev/null 2>&1 \
	|| fail "could not send gameplay key events for $game_case"
    xdotool key --window "$window_id" m >/dev/null 2>&1 \
	|| fail "could not open the console for $game_case"
    xdotool type --window "$window_id" --delay 10 "$game_client_name" \
	>/dev/null 2>&1 \
	|| fail "could not enter console text for $game_case"
    xdotool key --window "$window_id" Return >/dev/null 2>&1 \
	|| fail "could not submit console text for $game_case"
    if ! kill -0 "$client_pid" 2>/dev/null; then
	fail "client stopped after resize/input events for $game_case"
    fi
    if test "$game_capture" = yes; then
	capture_window game
    fi

    # xdotool may report BadWindow after the final key event because SDL tears
    # down the window before XSync completes.  That is a successful quit, not
    # an input failure; only reject the command while the client is still live.
    if ! xdotool key --clearmodifiers --window "$window_id" Escape y \
	>/dev/null 2>&1 && kill -0 "$client_pid" 2>/dev/null; then
	fail "could not request a graceful client quit for $game_case"
    fi
    wait_until "$game_case graceful client shutdown" 15 process_stopped \
	"$client_pid"
    finished_client_pid=$client_pid
    set +e
    wait "$client_pid"
    client_status=$?
    set -e
    client_pid=
    window_id=
    if test "$client_status" -ne 0; then
	fail "$game_case client returned status $client_status"
    fi
    wait_until "$game_case game window teardown" 5 process_window_absent \
	"$finished_client_pid"
    wait_until "$game_case server-side client departure" 5 client_departed
    stop_local_server
    run_recording_playback "$game_case" "$game_transport" "$game_recording"
}

run_transport_mismatch()
{
    server_transport=$1
    client_transport=$2
    game_case="mismatch-$server_transport-server-$client_transport-client"
    port=$(reserve_contact_port)
    game_server_log="$runtime_dir/server-$game_case.log"
    game_client_log="$runtime_dir/client-$game_case.log"
    game_client_name="SDL3Mismatch-$server_transport-$client_transport"

    "$server" -map "$map" -port "$port" -noQuit +reportMeta \
	-gameTransport "$server_transport" >"$game_server_log" 2>&1 &
    server_pid=$!
    wait_until "$game_case server readiness" 20 server_ready

    "$client" -geometry 800x600 -join -port "$port" \
	-name "$game_client_name" -gameTransport "$client_transport" \
	127.0.0.1 >"$game_client_log" 2>&1 &
    client_pid=$!
    rejected_client_pid=$client_pid
    wait_until "$game_case client rejection" 15 process_stopped "$client_pid"
    wait "$client_pid" 2>/dev/null || true
    client_pid=
    grep -q 'Gameplay transport mismatch with server' "$game_client_log" \
	|| fail "$game_case did not report the transport mismatch"
    if grep -q "Welcome .*$game_client_name" "$game_server_log"; then
	fail "$game_case advanced to gameplay"
    fi
    wait_until "$game_case window absence" 5 process_window_absent \
	"$rejected_client_pid"
    stop_local_server
}

mkdir -p -- "$runtime_dir/textures"
printf 'xpilot.texturePath: %s:%s\n' \
    "$runtime_dir/textures" "$XPILOT_TEST_PKGDATADIR/textures" \
    >"$runtime_dir/xpilotrc"
export XPILOTRC="$runtime_dir/xpilotrc"

node -e '
const dgram = require("node:dgram");
const fs = require("fs");
const portFile = process.argv[1];
const receivedFile = process.argv[2];
const socket = dgram.createSocket("udp4");
socket.on("error", (error) => {
  process.stderr.write(`${error.stack}\n`);
  socket.close(() => process.exit(1));
});
socket.on("message", (message, remote) => {
  const payload = message.toString("utf8").replace(/\0+$/, "");
  fs.appendFileSync(
    receivedFile,
    `source-port ${remote.port}\n${payload}\n---\n`,
  );
});
socket.bind(0, "127.0.0.1", () => {
  fs.writeFileSync(portFile, String(socket.address().port));
});
const stop = () => socket.close(() => process.exit(0));
process.on("SIGTERM", stop);
process.on("SIGINT", stop);
' "$runtime_dir/meta-report-fixture.port" \
    "$runtime_dir/meta-report-fixture.received" \
    >"$runtime_dir/meta-report-fixture.log" 2>&1 &
meta_report_fixture_pid=$!
wait_until "local metaserver report fixture" 10 meta_report_fixture_ready

meta_report_contact_port=$(reserve_contact_port)
game_server_log="$runtime_dir/server-meta-tcp-contact.log"
XPILOT_META_REPORT_HOST=127.0.0.1 \
XPILOT_META_REPORT_HOST_TWO=127.0.0.1 \
XPILOT_META_REPORT_PORT=$(sed -n '1p' \
    "$runtime_dir/meta-report-fixture.port") \
    "$server" -map "$map" -port "$meta_report_contact_port" -noQuit \
    -reportMeta -contactTransport tcp -gameTransport udp \
    >"$game_server_log" 2>&1 &
server_pid=$!
wait_until "TCP-contact metaserver report startup" 20 server_ready
wait_until "TCP-contact metaserver transport advertisement" 15 \
    meta_tcp_transport_reported
stop_local_server
kill -TERM "$meta_report_fixture_pid" 2>/dev/null || true
wait "$meta_report_fixture_pid" 2>/dev/null || true
meta_report_fixture_pid=

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
wait_until "metaserver text renderers" 10 \
    grep -q '^Font text renderers ready: game=renderer map=renderer$' \
        "$runtime_dir/meta.log"
wait_until "semantic metaserver UI" 20 meta_ui_ready
wait_until "local metaserver request" 5 meta_fixture_served
find_game_window || fail "metaserver window was not visible"
capture_window metaserver
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

run_gameplay_case tcp tcp no default
run_gameplay_case udp-default default yes default
run_gameplay_case udp-explicit udp no default
run_gameplay_case tcp-contact tcp no tcp
run_gameplay_case tcp-contact-udp-game udp no tcp
run_transport_mismatch udp tcp
run_transport_mismatch tcp udp

if ! runtime_logs_have_no_gl_errors; then
    fail "OpenGL diagnostics reported a runtime error"
fi

echo "SDL3 E2E smoke passed"
