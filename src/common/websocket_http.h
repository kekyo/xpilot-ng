/*
 * XPilot Infinity, a multiplayer space war game.
 *
 * Copyright (C) 2026 XPilot Infinity contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef WEBSOCKET_HTTP_H
#define WEBSOCKET_HTTP_H

#include <stdbool.h>
#include <stddef.h>

/** WebSocket resource used by XPilot session connections. */
#define WEBSOCKET_HTTP_RESOURCE "/xpilot"
/** Case-sensitive WebSocket subprotocol identifying XPilot logical records. */
#define WEBSOCKET_HTTP_SUBPROTOCOL "xpilot-infinity-v1"
/** Bytes in an RFC 6455 client nonce before Base64 encoding. */
#define WEBSOCKET_HTTP_NONCE_SIZE 16
/** Buffer capacity required for a Base64 `Sec-WebSocket-Key`. */
#define WEBSOCKET_HTTP_KEY_CAPACITY 25
/** Buffer capacity required for a `Sec-WebSocket-Accept` value. */
#define WEBSOCKET_HTTP_ACCEPT_CAPACITY 29
/** Maximum accepted HTTP opening-handshake header block. */
#define WEBSOCKET_HTTP_HEADER_CAPACITY 8192

/** Result of parsing an RFC 6455 opening-handshake request. */
typedef enum {
    /** More header bytes are required. */
    WEBSOCKET_HTTP_INCOMPLETE = 0,
    /** The request is valid and can be upgraded. */
    WEBSOCKET_HTTP_SWITCHING_PROTOCOLS = 101,
    /** The request syntax or required headers are invalid. */
    WEBSOCKET_HTTP_BAD_REQUEST = 400,
    /** The request targets a resource other than `/xpilot`. */
    WEBSOCKET_HTTP_NOT_FOUND = 404,
    /** The request does not select WebSocket version 13. */
    WEBSOCKET_HTTP_UPGRADE_REQUIRED = 426
} websocket_http_status_t;

/** Parsed server-side WebSocket opening-handshake data. */
typedef struct {
    /** Bytes occupied by the complete HTTP header block. */
    size_t consumed;
    /** Value to return in `Sec-WebSocket-Accept`. */
    char accept[WEBSOCKET_HTTP_ACCEPT_CAPACITY];
} websocket_http_server_request_t;

/** Parsed client-side WebSocket opening-handshake response data. */
typedef struct {
    /** Bytes occupied by the complete HTTP header block. */
    size_t consumed;
} websocket_http_client_response_t;

/**
 * Compute the RFC 6455 accept value for a client key.
 *
 * @param key Canonical Base64 encoding of exactly 16 nonce bytes.
 * @param accept Destination for the 28-character Base64 value.
 * @param capacity Destination capacity, including the terminating null.
 * @return `true` on success, otherwise `false` for invalid arguments or key.
 */
bool WebSocket_http_create_accept(const char *key, char *accept,
                                  size_t capacity);

/**
 * Create an XPilot WebSocket client opening-handshake request.
 *
 * @param output Destination byte buffer.
 * @param capacity Destination capacity.
 * @param length Receives the request length without a terminating null.
 * @param host Nonempty HTTP Host name.
 * @param port TCP port in the range 1 through 65535.
 * @param nonce Exactly 16 cryptographically random bytes.
 * @param expected_accept Receives the response value the client must verify.
 * @param expected_accept_capacity Capacity of `expected_accept`.
 * @return Zero on success, otherwise -1 with errno set.
 */
int WebSocket_http_create_client_request(
    char *output, size_t capacity, size_t *length, const char *host, int port,
    const unsigned char nonce[WEBSOCKET_HTTP_NONCE_SIZE],
    char *expected_accept, size_t expected_accept_capacity);

/**
 * Parse a WebSocket request received by an XPilot server.
 *
 * @param input Bytes received so far, which may include WebSocket frames
 *        after the HTTP header terminator.
 * @param length Number of available bytes.
 * @param request Receives parsed data for a successful upgrade.
 * @return Incomplete, upgrade, or the HTTP error status to return.
 */
websocket_http_status_t WebSocket_http_parse_server_request(
    const char *input, size_t length, websocket_http_server_request_t *request);

/**
 * Create the complete server response for a parsed request status.
 *
 * @param output Destination byte buffer.
 * @param capacity Destination capacity.
 * @param length Receives the response length without a terminating null.
 * @param status Upgrade or supported HTTP error status.
 * @param accept Accept value for an upgrade; ignored for error responses.
 * @return Zero on success, otherwise -1 with errno set.
 */
int WebSocket_http_create_server_response(
    char *output, size_t capacity, size_t *length,
    websocket_http_status_t status, const char *accept);

/**
 * Validate a server response to an XPilot WebSocket client handshake.
 *
 * @param input Bytes received so far, which may include WebSocket frames
 *        after the HTTP header terminator.
 * @param length Number of available bytes.
 * @param expected_accept Accept value derived from the client's nonce.
 * @param response Receives the consumed HTTP header length.
 * @return Incomplete, switching-protocols, or bad-request status.
 */
websocket_http_status_t WebSocket_http_parse_client_response(
    const char *input, size_t length, const char *expected_accept,
    websocket_http_client_response_t *response);

#endif /* WEBSOCKET_HTTP_H */
