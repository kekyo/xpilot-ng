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

#ifndef CONTACT_STREAM_H
#define CONTACT_STREAM_H

#include "net.h"

/**
 * Handle one complete contact request received over a framed TCP stream.
 *
 * @param request Complete request payload positioned at its first byte.
 * @param reply Empty writable framed reply buffer.
 * @param peer_address Numeric address of the connected peer.
 * @param peer_port Source port of the connected peer.
 * @return Positive when a reply was produced, otherwise non-positive.
 */
typedef int (*contact_stream_request_fn)(sockbuf_t *request,
					 sockbuf_t *reply,
					 const char *peer_address,
					 int peer_port);

/**
 * Open and register the non-blocking TCP contact listener.
 *
 * @param listener Socket object owned by the caller.
 * @param address Local numeric address, or NULL for all interfaces.
 * @param port Local contact port.
 * @param handler Callback for each complete request frame.
 * @return SOCK_IS_OK on success or SOCK_IS_ERROR on failure.
 */
int Contact_stream_init(sock_t *listener, char *address, int port,
			contact_stream_request_fn handler);

/** Advance pending TCP contact reads, writes, and admission timeouts. */
void Contact_stream_poll(void);

/** Close all pending streams and the registered listener. */
void Contact_stream_cleanup(void);

#endif /* CONTACT_STREAM_H */
