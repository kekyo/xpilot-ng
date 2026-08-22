/*
 * XPilot NG, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include <stddef.h>

#include "record_session.h"
#include "session_protocol.h"

/** Result of advancing a gameplay admission exchange once. */
typedef enum {
    /** The request was rejected or a protocol or transport error occurred. */
    CLIENT_SESSION_ERROR = -1,
    /** More transport progress is required. */
    CLIENT_SESSION_PENDING = 0,
    /** The first gameplay confirmation record was returned. */
    CLIENT_SESSION_CONFIRMATION_READY = 1
} client_session_result_t;

/** Opaque nonblocking gameplay admission exchange. */
typedef struct client_session client_session_t;

/**
 * Create a gameplay admission exchange on an existing logical session.
 *
 * @param session Stable session retained and owned by the caller.
 * @param request Gameplay request copied by this function.
 * @return Allocated exchange, or NULL on invalid input or allocation failure.
 */
client_session_t *Client_session_create(
    record_session_t *session, const session_game_request_t *request);

/**
 * Advance gameplay admission once without waiting.
 *
 * @param session Admission exchange.
 * @param confirmation Buffer receiving the first gameplay record.
 * @param capacity Size of confirmation in bytes.
 * @param length Receives the confirmation length, or zero while pending.
 * @return Current admission result.
 *
 * @remarks A required request remains staged while backpressured. A successful
 *          admission reply and its following gameplay confirmation are
 *          deliberately consumed in separate calls.
 */
client_session_result_t Client_session_step(
    client_session_t *session, char *confirmation, size_t capacity,
    size_t *length);

/**
 * Return the protocol version selected by an accepted server.
 *
 * @param session Admission exchange to inspect.
 * @return A version advertised in the request after acceptance, otherwise
 *         zero.
 */
unsigned Client_session_selected_version(const client_session_t *session);

/**
 * Return the stable logical-session identifier used by this exchange.
 *
 * @param session Admission exchange to inspect.
 * @return Stable identifier, or RECORD_SESSION_ID_INVALID for NULL.
 */
record_session_id_t Client_session_id(const client_session_t *session);

/**
 * Return the human-readable failure reason.
 *
 * @param session Admission exchange to inspect.
 * @return Stable failure text, an empty string before failure, or an invalid
 *         session message for NULL.
 */
const char *Client_session_error(const client_session_t *session);

/**
 * Destroy admission state without destroying the caller-owned session.
 *
 * @param session Admission exchange to destroy. NULL is accepted.
 */
void Client_session_destroy(client_session_t *session);

#endif /* CLIENT_SESSION_H */
