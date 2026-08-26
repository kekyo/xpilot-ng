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

#ifndef WEBSOCKET_TRANSPORT_H
#define WEBSOCKET_TRANSPORT_H

#include <stddef.h>

#include "record_transport.h"

/**
 * Wrap an accepted TCP stream as a server-side WebSocket record transport.
 *
 * @param connected_socket Connected socket whose ownership is transferred on
 *        success.
 * @param receive_capacity Largest accepted incoming binary message.
 * @param send_capacity Largest accepted outgoing binary message.
 * @return Allocated transport, or NULL on invalid input or allocation failure.
 *
 * @remarks The opening handshake is advanced lazily by receive and flush
 * operations. One binary WebSocket message represents one XPilot record.
 */
record_transport_t *Record_transport_create_websocket_server(
    sock_t *connected_socket, size_t receive_capacity, size_t send_capacity);

/**
 * Wrap a connected TCP stream as a client-side WebSocket record transport.
 *
 * @param connected_socket Connected socket whose ownership is transferred on
 *        success.
 * @param host Host name sent in the HTTP opening handshake.
 * @param port TCP destination port in the range 1 through 65535.
 * @param receive_capacity Largest accepted incoming binary message.
 * @param send_capacity Largest accepted outgoing binary message.
 * @return Allocated transport, or NULL on invalid input, entropy failure, or
 *         allocation failure.
 *
 * @remarks Client frames are masked with a fresh secure mask generated for
 * each frame by wslay. The opening handshake is advanced lazily.
 */
record_transport_t *Record_transport_create_websocket_client(
    sock_t *connected_socket, const char *host, int port,
    size_t receive_capacity, size_t send_capacity);

#endif /* WEBSOCKET_TRANSPORT_H */
