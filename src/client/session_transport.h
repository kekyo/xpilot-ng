/*
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2026 XPilot NG contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef CLIENT_SESSION_TRANSPORT_H
#define CLIENT_SESSION_TRANSPORT_H

#include "game_transport.h"
#include "record_transport.h"

/**
 * Connect a fixed-endpoint session transport over a native TCP socket.
 *
 * @param transport TCP record framing or WebSocket message framing.
 * @param server Numeric address or hostname.
 * @param port Destination port in the range 1 through 65535.
 * @param source_port_start First requested source port, or zero to let the
 * operating system select a source port.
 * @param source_port_end Last requested source port, or zero together with
 * source_port_start.
 * @param timeout_seconds Positive connection timeout.
 * @param report_errors Whether to emit connection diagnostics.
 * @return Owned transport, or NULL on failure.
 *
 * @remarks The returned transport owns the connected socket and closes it
 * when Record_transport_destroy() is called.
 */
record_transport_t *Client_session_transport_connect(
    game_transport_t transport, const char *server, int port,
    int source_port_start, int source_port_end, int timeout_seconds,
    bool report_errors);

#endif /* CLIENT_SESSION_TRANSPORT_H */
