# wslay

This directory contains the data-transfer portion of wslay 1.1.1.

- Upstream: https://github.com/tatsuhiro-t/wslay
- Tag: `release-1.1.1`
- Commit: `c9a84aa6df8512584c77c8cd15be9536b89c35aa`
- License: MIT; see `LICENSE`

The files are copied without modification from upstream.  XPilot NG builds
the four source files listed by wslay's Automake build (`wslay_frame.c`,
`wslay_event.c`, `wslay_queue.c`, and `wslay_net.c`) into `libxpcommon.a`.
wslay deliberately implements RFC 6455 framing rather than the HTTP opening
handshake; the latter is implemented by XPilot NG's WebSocket adapter.
