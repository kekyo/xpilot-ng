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

#include "xpclient.h"

#include "tcp_transport.h"

static int Client_tcp_open_source(sock_t *socket, int start, int end)
{
    int port;

    if (start == 0 && end == 0)
        return sock_open_tcp_bound(socket, NULL, 0);
    for (port = start; port <= end; port++) {
        if (sock_open_tcp_bound(socket, NULL, port) != SOCK_IS_ERROR)
            return SOCK_IS_OK;
    }
    return SOCK_IS_ERROR;
}

record_transport_t *Client_tcp_transport_connect(
    const char *server, int port, int source_port_start,
    int source_port_end, int timeout_seconds)
{
    record_transport_t *transport;
    sock_t socket;

    if (server == NULL || port <= 0 || port > 65535
        || source_port_start < 0 || source_port_start > 65535
        || source_port_end < 0 || source_port_end > 65535
        || ((source_port_start == 0) != (source_port_end == 0))
        || source_port_start > source_port_end
        || timeout_seconds <= 0) {
        errno = EINVAL;
        return NULL;
    }
    sock_init(&socket);
    if (Client_tcp_open_source(
            &socket, source_port_start, source_port_end) == SOCK_IS_ERROR) {
        error("Cannot create a TCP socket in the requested source range");
        return NULL;
    }
    if (sock_connect_with_timeout(
            &socket, (char *)server, port,
            timeout_seconds) == SOCK_IS_ERROR) {
        error("Can't contact %s on port %d", server, port);
        sock_close(&socket);
        return NULL;
    }
    if (sock_set_tcp_nodelay(&socket, 1) == SOCK_IS_ERROR
        || sock_set_non_blocking(&socket, 1) == SOCK_IS_ERROR) {
        error("Can't configure TCP session socket");
        sock_close(&socket);
        return NULL;
    }
    if (sock_set_send_buffer_size(
            &socket, CLIENT_SEND_SIZE + 256) == SOCK_IS_ERROR)
        error("Can't set send buffer size to %d", CLIENT_SEND_SIZE + 256);
    if (sock_set_receive_buffer_size(
            &socket, CLIENT_RECV_SIZE + 256) == SOCK_IS_ERROR)
        error("Can't set receive buffer size to %d", CLIENT_RECV_SIZE + 256);

    transport = Record_transport_create_tcp(
        &socket, CLIENT_RECV_SIZE, CLIENT_SEND_SIZE);
    if (transport == NULL)
        sock_close(&socket);
    return transport;
}
