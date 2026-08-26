/*
 * XPilot Infinity, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef RECORD_SESSION_H
#define RECORD_SESSION_H

#include <stdint.h>

#include "record_transport.h"

/** Stable identifier assigned to one logical record session. */
typedef uint64_t record_session_id_t;

/** Value that never identifies a logical record session. */
#define RECORD_SESSION_ID_INVALID UINT64_C(0)

/** Opaque stable session owning one replaceable record transport. */
typedef struct record_session record_session_t;

/**
 * Create a uniquely identified logical record session.
 *
 * @param transport Transport whose ownership is transferred, including when
 *        session allocation or identifier generation fails.
 * @return Allocated session, or NULL on invalid input, allocation failure, or
 *         identifier exhaustion.
 */
record_session_t *Record_session_create(record_transport_t *transport);

/**
 * Return the process-lifetime identifier of a session.
 *
 * @param session Session to inspect.
 * @return Stable nonzero identifier, or RECORD_SESSION_ID_INVALID for NULL.
 */
record_session_id_t Record_session_id(const record_session_t *session);

/**
 * Replace a session's transport without changing its identity.
 *
 * @param session Session whose transport is replaced.
 * @param transport New transport whose ownership is transferred on success.
 * @return Zero on success, otherwise -1.
 *
 * @remarks The previous transport is closed and destroyed after the new one
 *          is installed. Passing the currently installed transport is a
 *          successful no-op.
 */
int Record_session_replace_transport(record_session_t *session,
                                     record_transport_t *transport);

/**
 * Move a replacement session's transport into an existing stable session.
 *
 * @param session Stable destination session.
 * @param replacement Session providing the replacement transport.
 * @return Zero on success, otherwise -1.
 *
 * @remarks On success replacement is consumed and its session identifier is
 *          discarded. On failure both sessions retain their ownership.
 */
int Record_session_replace_from(record_session_t *session,
                                record_session_t *replacement);

/**
 * Close and discard the current transport while retaining session identity.
 *
 * @param session Session to detach. NULL is accepted.
 */
void Record_session_close(record_session_t *session);

/** Receive at most one logical record without waiting. */
record_receive_result_t Record_session_receive(
    record_session_t *session, char *destination, size_t capacity,
    size_t *length);

/** Submit one logical record without waiting. */
record_send_result_t Record_session_send(record_session_t *session,
                                         const char *payload,
                                         size_t length,
                                         record_delivery_t delivery);

/** Advance previously accepted output without waiting. */
record_flush_result_t Record_session_flush(record_session_t *session);

/** Return the optional native handle used by a native waiting wrapper. */
socket_handle_t Record_session_native_handle(
    const record_session_t *session);

/** Close and destroy a session and its current transport. NULL is accepted. */
void Record_session_destroy(record_session_t *session);

#endif /* RECORD_SESSION_H */
