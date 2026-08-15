# Direct TCP session transport

## Scope and topology

XPilot NG now uses a direct, connection-oriented topology. The server opens
one TCP listener on `gamePort` (`-port`, default 15345), and that listener
accepts both gameplay sessions and short-lived control sessions. The client
accepts at most one positional server host and uses `127.0.0.1` when no host is
specified.

The application no longer contains a lobby/server-selection interface, LAN
broadcast discovery, metaserver queries or reporting, UDP ping, a waiting
queue, UDP contact negotiation, per-player listeners, or application UDP
sockets. A gameplay connection is admitted and then used for setup, login,
and play without changing streams or ports.

This is deliberately a TCP migration, not a browser transport implementation.
WebSocket, TLS, HTTP upgrade handling, browser origin policy, asset delivery,
and a WebAssembly/WebGL runtime remain outside this change.

## Record framing

TCP is a byte stream and does not preserve write boundaries. Every XPilot
packet payload is therefore carried as one framed record:

```text
0               15 16
+-----------------+--------------------------+
| payload length  | existing PKT payload ... |
+-----------------+--------------------------+
   2 bytes, network byte order
```

The unsigned length excludes the two-byte header and must be nonzero and no
larger than the receiving `sockbuf_t` capacity. Existing packet fields inside
the payload are unchanged. A receiver keeps a split header or payload across
reads, returns one complete logical record at a time, and retains coalesced
later records for subsequent calls. Zero-length or oversized records, EOF in
the middle of a record, and fatal I/O errors terminate the connection.

Writes are record-aware as well. A partially written record is retained until
all its bytes have been submitted; discarding only its remainder would
desynchronize every later record. User-space buffering is bounded to the
logical write buffer plus one pending wire record. If a transient update has
not yet entered TCP and the pending record is still blocked, that new update
can be discarded instead of growing an unbounded queue. Bytes already
accepted by the kernel cannot be recalled.

`TCP_NODELAY` is enabled on gameplay endpoints so small records are not held
by Nagle's algorithm. The client and server use their normal error paths for
EOF, reset, malformed framing, and fatal reads or writes.

## Session opening and admission

The first framed record identifies the purpose of a new connection:

- `SESSION_GAME` carries the gameplay identity and the supported polygon and
  legacy protocol versions;
- `SESSION_CONTROL` carries one control command, its argument, and the claimed
  user name.

The current TCP-capable protocol versions are `4F17` for polygon maps and
`4503` for legacy maps. A gameplay request receives an admission reply and,
on success, the same accepted stream is promoted directly into
`Setup_connection`. There is no `PKT_VERIFY`, second connection, returned
player port, or descriptor replacement with `dup2`.

The server limits pending accepted sessions to four globally and two per
source IP. An initial record that is not completed in four seconds is closed.
The kernel listen backlog is the gameplay connection limit plus those four
admission slots. Both gameplay opening and client control operations use a
five-second connection/session deadline. These bounds prevent ordinary slow
or abandoned opens from consuming all application admission slots; they are
not a complete SYN-flood or connection-flood defense.

Control sessions execute one command and close after their reply frames have
been sent. Status and visible-option queries are public. Commands that mutate
state are accepted only from the IPv4 loopback range when the claimed user
name exactly matches the operating-system login name recorded as the server
owner. This preserves a useful local administrative boundary, but the user
field is not cryptographically authenticated and the protocol provides no
confidentiality or integrity. It must not be treated as a remote
administration security mechanism.

## Consequences of TCP

### Loss, ordering, and retained reliability logic

TCP removes application-visible network loss, duplication, and reordering for
bytes delivered on a live connection. XPilot's application-level reliable
stream, acknowledgements, retransmission timers, sequence handling, and RTT
estimator therefore no longer provide transport reliability. Normal network
reordering branches and logic built solely around missing UDP datagrams are
mostly dormant as well.

Those mechanisms are retained to keep the existing packet payloads and game
state transitions stable during this migration. They can still protect their
own logical sequencing, but they now add acknowledgement traffic and may
retransmit data that TCP has already queued. Under congestion this redundancy
can consume bandwidth and increase latency. Removing it is a separate
protocol refactor that should be measured and tested independently.

The existing packet-loss display can no longer represent UDP network loss.
Any reported gaps now arise from such causes as application-side bounded
output drops, server update selection, playback input, or a client that falls
behind. Renaming or redesigning that display is intentionally deferred.

### Head-of-line blocking and stale state

TCP delivers bytes strictly in order. Loss of one TCP segment prevents newer
game records on that connection from reaching the application until the
missing bytes are retransmitted. UDP could discard an old frame while still
delivering a newer state update, which is often preferable for a real-time
game.

The bounded application queue can discard a transient record only before it
enters the stream. It cannot cancel an older record already accepted into the
kernel or network. Consequently, head-of-line delay and delivery of stale
state are the main behavioral regressions of this transport and cannot be
fully repaired while using one ordered byte stream.

Congestion is isolated per gameplay connection, so one slow client does not
directly block another client's stream. It can still consume server memory,
CPU, and scheduler activity within the configured bounds.

### Backpressure, latency, and traffic

A UDP send either emitted or dropped a whole datagram. A nonblocking TCP write
may accept only part of a record, and congestion control may keep the
remainder pending across many game ticks. Correct partial-read, partial-write,
and framing state is therefore mandatory.

`TCP_NODELAY` avoids deliberate Nagle coalescing, but it does not eliminate
retransmission delay, delayed acknowledgements, congestion-control effects,
or head-of-line blocking. Establishing the direct TCP connection also costs a
TCP handshake before the session-open record can be processed.

Each XPilot payload gains a two-byte framing header. TCP acknowledgement and
congestion-control traffic replaces UDP's datagram behavior, while the
retained XPilot reliable layer adds its own acknowledgements. IP fragmentation
of individual application datagrams is no longer an application concern;
segmentation and path-MTU behavior belong to the TCP/IP stack.

### Disconnects and idle peers

An orderly close is visible as EOF and a reset is a stream error, giving a
clearer failure signal than repeated UDP silence. A physically dead but idle
connection may still remain undetected for a substantial time because this
implementation does not enable TCP keepalive. Existing gameplay activity and
timeouts remain responsible for liveness detection.

### Ports, NAT, and exposure

The server uses one fixed TCP port for every gameplay and control connection.
There are no server-side `clientPortStart`/`clientPortEnd` settings and no
dynamic per-player ports. Firewall and NAT configuration therefore needs one
TCP allow/forward rule, normally for port 15345. The client's retained
`clientPortStart`/`clientPortEnd` settings optionally constrain only its TCP
source port.

Sharing one listener simplifies deployment and is closer to the topology
needed for WebSocket, but it also puts gameplay and control admission behind
the same kernel queue. The bounded pre-promotion/control-session pool and
per-IP limit reduce interference after `accept`; deployment-level filtering
or rate limiting is still required against deliberate connection floods.

## Recording and playback

The listener and pre-admission sessions are outside the recorded scheduler.
After gameplay admission, the actual promoted TCP descriptor is recorded as
the gameplay endpoint. Playback reconstructs accepted TCP session state so
the existing scheduler can replay gameplay I/O without a live listener
accepting that client.

Recordings produced by the old UDP topology do not contain the same framed
session lifecycle and are not supported by this version. Backward
compatibility was not retained because it would require a separate recording
format discriminator and a second network model.

## Future WebSocket boundary

Removing discovery, contact datagrams, and per-player port negotiation makes
each client session a single connection with an explicit first message. That
is the structural preparation intended by this migration. A later WebSocket
step still needs explicit decisions and implementation for:

- HTTP upgrade and whether the existing two-byte record framing remains
  inside binary WebSocket messages;
- secure `wss` deployment, certificate termination, browser origin checks,
  and an authentication model for administration;
- browser-compatible asset loading instead of the current filesystem and
  optional HTTP data paths;
- WebAssembly integration, WebGL rendering, browser input/audio, and lifecycle
  handling; and
- a separately designed web lobby or server directory, if one is wanted.

None of those concerns is implemented or partially emulated here.

## Verification criteria

The TCP migration is complete when all of the following hold:

1. the application creates no UDP socket and exposes no UDP discovery,
   contact, ping, or metaserver path;
2. one `gamePort` TCP listener admits both gameplay and control sessions;
3. gameplay admission, setup, login, and play remain on the same stream;
4. existing `PKT_*` payloads cross two-byte framed records unchanged;
5. split/coalesced reads, partial writes, malformed lengths, EOF, and bounded
   backpressure have functional test coverage;
6. pending admission limits and timeouts are enforced;
7. public control queries and loopback-owner mutation checks are covered by
   end-to-end tests;
8. normal gameplay, graphics initialization, recording, and playback pass end
   to end; and
9. both the normal SDL build and server-only build, install, and distribution
   checks pass through `./tests/run-full-suite.sh`.
