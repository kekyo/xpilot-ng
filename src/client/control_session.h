/*
 * XPilot NG, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef CONTROL_SESSION_H
#define CONTROL_SESSION_H

#include "record_session.h"
#include "session_protocol.h"

/** Result of advancing a control exchange once. */
typedef enum {
    /** A protocol or transport error occurred. */
    CONTROL_SESSION_ERROR = -1,
    /** More transport progress is required. */
    CONTROL_SESSION_PENDING = 0,
    /** One reply was returned and another reply follows. */
    CONTROL_SESSION_REPLY_READY = 1,
    /** The final reply was returned. */
    CONTROL_SESSION_FINISHED = 2
} control_session_result_t;

/** Opaque nonblocking control-request exchange. */
typedef struct control_session control_session_t;

/**
 * Create a control exchange on an existing logical session.
 *
 * @param session Stable session retained and owned by the caller.
 * @param request Control request copied by this function.
 * @return Allocated exchange, or NULL on invalid input or allocation failure.
 */
control_session_t *Control_session_create(
    record_session_t *session, const session_control_request_t *request);

/**
 * Advance a control exchange once without waiting.
 *
 * @param session Control exchange.
 * @param reply Receives one reply when the result is
 *        CONTROL_SESSION_REPLY_READY or CONTROL_SESSION_FINISHED.
 * @return Current exchange result.
 */
control_session_result_t Control_session_step(
    control_session_t *session, session_control_reply_t *reply);

/**
 * Return the stable logical-session identifier used by this exchange.
 *
 * @param session Control exchange to inspect.
 * @return Stable identifier, or RECORD_SESSION_ID_INVALID for NULL.
 */
record_session_id_t Control_session_id(const control_session_t *session);

/**
 * Return the human-readable failure reason.
 *
 * @param session Control exchange to inspect.
 * @return Stable failure text, an empty string before failure, or an invalid
 *         session message for NULL.
 */
const char *Control_session_error(const control_session_t *session);

/**
 * Destroy control state without destroying the caller-owned session.
 *
 * @param session Control exchange to destroy. NULL is accepted.
 */
void Control_session_destroy(control_session_t *session);

#endif /* CONTROL_SESSION_H */
