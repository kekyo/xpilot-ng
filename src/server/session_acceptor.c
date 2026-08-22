/*
 * XPilot NG, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "xpcommon.h"

#include "session_acceptor.h"

typedef enum {
    SESSION_ACCEPTOR_WAIT_OPEN,
    SESSION_ACCEPTOR_READY,
    SESSION_ACCEPTOR_TRANSFERRED,
    SESSION_ACCEPTOR_FAILED
} session_acceptor_state_t;

struct session_acceptor {
    record_session_t *session;
    session_acceptor_state_t state;
    char input[SESSION_PROTOCOL_MAX_RECORD_SIZE];
    char error[MSG_LEN];
};

static session_acceptor_result_t Session_acceptor_fail(
    session_acceptor_t *acceptor, const char *reason)
{
    acceptor->state = SESSION_ACCEPTOR_FAILED;
    strlcpy(acceptor->error, reason, sizeof(acceptor->error));
    return SESSION_ACCEPTOR_ERROR;
}

session_acceptor_t *Session_acceptor_create(record_session_t *session)
{
    session_acceptor_t *acceptor;

    if (session == NULL) {
        errno = EINVAL;
        return NULL;
    }
    acceptor = calloc(1, sizeof(*acceptor));
    if (acceptor == NULL) {
        Record_session_destroy(session);
        return NULL;
    }
    acceptor->session = session;
    acceptor->state = SESSION_ACCEPTOR_WAIT_OPEN;
    return acceptor;
}

session_acceptor_result_t Session_acceptor_step(
    session_acceptor_t *acceptor, session_open_t *open)
{
    record_receive_result_t result;
    size_t length;

    if (acceptor == NULL || open == NULL) {
        errno = EINVAL;
        return SESSION_ACCEPTOR_ERROR;
    }
    if (acceptor->state != SESSION_ACCEPTOR_WAIT_OPEN) {
        errno = EALREADY;
        return SESSION_ACCEPTOR_ERROR;
    }
    result = Record_session_receive(
        acceptor->session, acceptor->input, sizeof(acceptor->input), &length);
    if (result == RECORD_RECEIVE_EMPTY)
        return SESSION_ACCEPTOR_PENDING;
    if (result == RECORD_RECEIVE_CLOSED)
        return Session_acceptor_fail(acceptor, "transport closed");
    if (result == RECORD_RECEIVE_ERROR)
        return Session_acceptor_fail(acceptor, "session request read failed");
    if (Session_protocol_decode_open(acceptor->input, length, open) == -1)
        return Session_acceptor_fail(acceptor, "invalid session request");
    acceptor->state = SESSION_ACCEPTOR_READY;
    return open->purpose == SESSION_PURPOSE_GAME
        ? SESSION_ACCEPTOR_GAME_READY : SESSION_ACCEPTOR_CONTROL_READY;
}

record_session_id_t Session_acceptor_id(const session_acceptor_t *acceptor)
{
    return acceptor != NULL && acceptor->session != NULL
        ? Record_session_id(acceptor->session) : RECORD_SESSION_ID_INVALID;
}

record_session_t *Session_acceptor_take_session(
    session_acceptor_t *acceptor)
{
    record_session_t *session;

    if (acceptor == NULL) {
        errno = EINVAL;
        return NULL;
    }
    if (acceptor->state != SESSION_ACCEPTOR_READY
        || acceptor->session == NULL) {
        errno = EAGAIN;
        return NULL;
    }
    session = acceptor->session;
    acceptor->session = NULL;
    acceptor->state = SESSION_ACCEPTOR_TRANSFERRED;
    return session;
}

const char *Session_acceptor_error(const session_acceptor_t *acceptor)
{
    return acceptor != NULL ? acceptor->error : "invalid acceptor";
}

void Session_acceptor_destroy(session_acceptor_t *acceptor)
{
    if (acceptor == NULL)
        return;
    Record_session_destroy(acceptor->session);
    free(acceptor);
}
