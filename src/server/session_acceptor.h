/*
 * XPilot Infinity, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SESSION_ACCEPTOR_H
#define SESSION_ACCEPTOR_H

#include "record_session.h"
#include "session_protocol.h"

/** Result of advancing a newly attached server session once. */
typedef enum {
    /** A protocol or transport error occurred. */
    SESSION_ACCEPTOR_ERROR = -1,
    /** No complete session-open record is currently available. */
    SESSION_ACCEPTOR_PENDING = 0,
    /** A gameplay admission request was returned. */
    SESSION_ACCEPTOR_GAME_READY = 1,
    /** A one-shot control request was returned. */
    SESSION_ACCEPTOR_CONTROL_READY = 2,
    /** A gameplay resumption request was returned. */
    SESSION_ACCEPTOR_RESUME_READY = 3
} session_acceptor_result_t;

/** Opaque owner of a session awaiting its first control-plane record. */
typedef struct session_acceptor session_acceptor_t;

/**
 * Create an acceptor owning a newly attached logical session.
 *
 * @param session Session whose ownership is transferred, including when
 *        acceptor allocation fails.
 * @return Allocated acceptor, or NULL on invalid input or allocation failure.
 */
session_acceptor_t *Session_acceptor_create(record_session_t *session);

/**
 * Advance acceptance once without waiting.
 *
 * @param acceptor Acceptor to advance.
 * @param open Receives a validated request when a ready result is returned.
 * @return Current acceptance result.
 */
session_acceptor_result_t Session_acceptor_step(
    session_acceptor_t *acceptor, session_open_t *open);

/**
 * Return the stable identifier of the owned logical session.
 *
 * @param acceptor Acceptor to inspect.
 * @return Stable identifier, or RECORD_SESSION_ID_INVALID when unavailable.
 */
record_session_id_t Session_acceptor_id(const session_acceptor_t *acceptor);

/**
 * Transfer an accepted session to its gameplay or control owner.
 *
 * @param acceptor Acceptor that has returned a ready result.
 * @return Owned session on success, otherwise NULL with errno set.
 */
record_session_t *Session_acceptor_take_session(
    session_acceptor_t *acceptor);

/**
 * Return the human-readable failure reason.
 *
 * @param acceptor Acceptor to inspect.
 * @return Stable failure text, an empty string before failure, or an invalid
 *         acceptor message for NULL.
 */
const char *Session_acceptor_error(const session_acceptor_t *acceptor);

/**
 * Destroy the acceptor and any session that was not transferred.
 *
 * @param acceptor Acceptor to destroy. NULL is accepted.
 */
void Session_acceptor_destroy(session_acceptor_t *acceptor);

#endif /* SESSION_ACCEPTOR_H */
