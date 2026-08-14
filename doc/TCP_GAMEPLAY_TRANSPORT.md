# TCP gameplay transport

## Scope

Only the transport for an active gameplay session changes from UDP to TCP.
The existing XPilot packet payloads (`PKT_*`), setup states, frame processing,
and application-level reliable-data messages remain unchanged.

The surrounding network paths remain as they were:

- server contact, queueing, and entry negotiation continue to use UDP;
- LAN discovery and broadcast queries continue to use UDP;
- metaserver listing and ping paths keep their existing transports; and
- each gameplay connection uses its own TCP listener and accepted stream.

This split is intentional.  Converting discovery or metaserver protocols is
not required to replace the gameplay transport and would expand the change
beyond the minimum needed for this migration.

## Wire format

TCP does not preserve write boundaries.  Every former gameplay datagram is
therefore sent as one framed record:

```text
0               15 16
+-----------------+--------------------------+
| payload length  | existing PKT payload ... |
+-----------------+--------------------------+
   2 bytes, network byte order
```

The length is unsigned, excludes the two-byte header, and must be between one
and the receiving `sockbuf_t` capacity.  No field inside the existing payload
is changed.  A receiver retains an incomplete header or payload across reads,
returns at most one logical record at a time, and leaves later coalesced
records for subsequent reads.  Zero-length and oversized records terminate
the connection as protocol errors.

Writes are also record-aware.  A partially written record is retained until
it can be completed.  To preserve the previous real-time behavior and keep
memory bounded, only one wire-format record may be pending outside the
logical write buffer.  If that record is still blocked when a newer transient
update is flushed, the newer update is discarded instead of being appended
to an unbounded queue or merged into the older record.  The operating system
TCP send buffer remains an additional bounded queue.

`TCP_NODELAY` is enabled on both endpoints so small gameplay records are not
held by Nagle's algorithm.  EOF, reset, a zero-byte write, malformed framing,
and fatal read/write failures are treated as connection errors.  `SIGPIPE` is
ignored by the existing client and server signal setup, so a failed stream is
handled through the normal network error path.

## Connection lifecycle

The UDP contact exchange returns a per-player gameplay port as before.  The
server now listens for TCP on that port, and the client binds its configured
local port (if any) before connecting.  After `accept`, the server keeps the
listener's descriptor number with `dup2`.  The scheduler and record/playback
code index handlers by descriptor offset, so preserving this number avoids a
larger change to those subsystems.

The accepted source address must match the address seen by the UDP contact
socket, and the existing `PKT_VERIFY` user/nickname check still runs on the
first framed record.  This does not add authentication, confidentiality, or
integrity protection; it only retains the existing association checks.

The protocol versions are `4F16` for polygon maps and `4502` for old-format
maps.  The server and client accept only these TCP-capable versions in their
respective protocol families.  This prevents a legacy UDP peer from passing
contact negotiation and failing later on the gameplay port.  Old UDP session
recordings are likewise not compatible with the framed stream format.

## Consequences of using TCP

### Loss, ordering, and the existing reliability layer

TCP eliminates application-visible network loss, duplication, and reordering
for bytes that remain on a live connection.  Code whose sole purpose was to
repair UDP delivery is consequently redundant.  In particular, XPilot's
`PKT_RELIABLE` stream, acknowledgements, retransmission timers, sequence
checks, and RTT estimator no longer provide transport reliability.

They are deliberately retained in this migration because removing them would
change packet construction and several setup/gameplay state transitions.
They still deduplicate their own data, but now add acknowledgement traffic and
can retransmit data already queued by TCP.  During congestion this extra data
can worsen delay.  Removing the layer is a reasonable later refactoring, but
it should be measured and tested separately from the transport conversion.

The packet loss meter also changes meaning.  It can no longer measure UDP
network loss or reordering.  It may still report logical frame gaps caused by
bounded-output drops, server-side update selection, or a renderer that cannot
consume updates quickly enough.  The existing UI name is retained to avoid an
unrelated interface change.

The receive window and duplicate/out-of-order branches likewise cease to
observe normal network reordering.  The window remains useful for discarding
older complete records when rendering falls behind, while the checks still
guard malformed playback or application data.  Removing them would mix a
larger frame-processing refactor into this transport change.

### Head-of-line blocking and stale state

TCP delivers bytes strictly in order.  If one segment is lost, newer gameplay
records already accepted by the kernel cannot be delivered until the missing
bytes are retransmitted.  UDP allowed a newer frame to arrive even when an
older frame was lost, which is often preferable for real-time state.

The bounded application queue limits memory and allows an update that has not
entered TCP to be discarded.  It cannot cancel an older record already
accepted into the kernel or network.  Head-of-line latency is therefore the
principal behavioral regression of this transport choice and cannot be fully
removed without using a transport that supports independent or unreliable
messages.

### Congestion and backpressure

A UDP send either emitted or dropped one datagram.  A nonblocking TCP send may
accept only part of a record, so the remainder must be kept; dropping it would
desynchronize every later frame.  TCP congestion control can also reduce the
send rate for much longer than one game tick.

The implementation bounds user-space output to the current logical buffer and
one pending framed record.  When the pending record cannot advance, the next
transient update is dropped.  Reliable application messages will be retried by
the retained protocol machinery.  Kernel buffering is configured near the
existing XPilot send-buffer sizes, although operating systems may internally
adjust the requested size.  Memory is bounded, but latency can still grow by
the amount already accepted into the kernel.

Congestion is isolated per player's TCP connection, so one slow client does
not directly block delivery to other clients.  It can still consume server
CPU through repeated wakeups, retransmissions, and connection-state handling.

### Latency and traffic

`TCP_NODELAY` avoids deliberate coalescing of small writes, but it cannot
remove retransmission delay, delayed acknowledgements, congestion-control
effects, or head-of-line blocking.  The TCP handshake also adds a round trip
after UDP contact negotiation before verification can begin.

The client currently performs this gameplay `connect` synchronously, as the
old initialization flow expected immediate UDP `connect` completion.  An
unreachable TCP endpoint can therefore leave startup waiting for the operating
system's connect timeout.  Moving connection establishment into the event loop
would avoid that pause, but requires a new setup state and is deferred.

Each logical record gains a two-byte framing header.  TCP acknowledgements and
the retained XPilot acknowledgement stream add traffic, while TCP segmentation
removes the old one-datagram/one-packet assumption.  Large logical records no
longer risk UDP datagram truncation in this code, but IP/TCP segmentation and
path-MTU behavior are controlled by the network stack.

### Disconnect and liveness behavior

An orderly close is now observable as EOF, and a reset is a stream error.
This is clearer than waiting for repeated UDP silence.  Conversely, a
physically dead but otherwise idle connection may not fail immediately;
TCP keepalive is not enabled here.  Existing application activity and timeout
state remain responsible for detecting such peers.  A partially received
record is never delivered, including when EOF occurs halfway through it.

### Ports, NAT, and firewalls

The server's `clientPortStart` through `clientPortEnd` range now contains TCP
gameplay listeners.  Firewalls and NAT forwarding rules for that range must
permit TCP rather than UDP.  The default contact port remains UDP, so server
operators must permit both the UDP contact port and the configured TCP
gameplay range.

On the client, the same optional range is used by both the unchanged UDP
contact path and the TCP gameplay socket.  Client-side filtering rules that
restrict source ports must therefore allow both protocols.  TCP NAT state is
connection-oriented and generally persists differently from UDP mappings.

Creating one short-lived listener for every pending login also introduces a
TCP SYN queue and related resource exposure.  The backlog is limited to one,
the normal listening timeout still applies, and unexpected source addresses
are closed.  This is not a defense against a deliberate SYN or connection
flood; deployment-level rate limiting may still be required.

### Record and playback

During connection verification, the record wrapper stores underlying stream
reads, including two-byte headers and partial I/O boundaries, while the framed
`Sockbuf` still presents complete payloads to game code.  During normal play,
the existing optimized recording path instead stores each complete payload
after deframing and injects that payload directly during playback.  The
accepted socket is moved onto the listener descriptor so scheduler indices in
new recordings remain stable.  Playback bypasses live `accept` and marks the
recorded endpoint as an already-connected TCP stream.

Recordings produced by the UDP implementation do not contain TCP length
headers and are not expected to play with this version.  Preserving that
backward compatibility would require a separate recording-format discriminator
and is outside this transport-only change.

## Intentionally deferred work

The following changes may reduce overhead or improve real-time behavior, but
are not part of the minimal TCP migration:

- removing `PKT_RELIABLE`, its ACKs, retransmission timers, and RTT estimator;
- renaming or redesigning the packet loss meter;
- introducing priority queues, frame replacement inside the kernel queue, or
  a separate control connection;
- moving the blocking client TCP connect into the event loop;
- enabling TCP keepalive or platform-specific low-latency TCP options;
- adding TLS or changing the existing contact/identity model;
- converting UDP discovery/contact protocols; and
- redesigning record files for UDP/TCP cross-version playback.

## Verification criteria

The migration is complete when:

1. gameplay endpoints report `SO_TYPE == SOCK_STREAM` and use
   `TCP_NODELAY`;
2. existing `PKT_*` payloads cross a two-byte framed stream unchanged;
3. split headers, split payloads, coalesced records, and partial writes retain
   record boundaries;
4. backpressure remains bounded and does not merge logical records;
5. malformed lengths, EOF, and fatal I/O errors close the session path;
6. the UDP contact flow still leads to a verified TCP gameplay connection;
7. both normal and server-only builds pass the complete test suite; and
8. auxiliary UDP paths remain outside the gameplay conversion.
