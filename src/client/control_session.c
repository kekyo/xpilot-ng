/*
 * XPilot NG, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "xpcommon.h"

#include "control_session.h"

typedef enum {
    CONTROL_SESSION_SEND_OPEN,
    CONTROL_SESSION_WAIT_REPLY,
    CONTROL_SESSION_COMPLETE,
    CONTROL_SESSION_FAILED
} control_session_state_t;

struct control_session {
    record_session_t *session;
    control_session_state_t state;
    unsigned char command;
    size_t open_length;
    char open_record[SESSION_PROTOCOL_MAX_RECORD_SIZE];
    char reply_record[SESSION_PROTOCOL_MAX_RECORD_SIZE];
    char error[MSG_LEN];
};

static control_session_result_t Control_session_fail(
    control_session_t *session, const char *reason)
{
    session->state = CONTROL_SESSION_FAILED;
    strlcpy(session->error, reason, sizeof(session->error));
    return CONTROL_SESSION_ERROR;
}

static control_session_result_t Control_session_send_open(
    control_session_t *session)
{
    record_send_result_t result = Record_session_send(
        session->session, session->open_record, session->open_length,
        RECORD_DELIVERY_REQUIRED);

    if (result == RECORD_SEND_BACKPRESSURED)
        return CONTROL_SESSION_PENDING;
    if (result == RECORD_SEND_ERROR)
        return Control_session_fail(session, "control request send failed");
    if (result == RECORD_SEND_CLOSED)
        return Control_session_fail(session, "transport closed");
    if (result != RECORD_SEND_ACCEPTED)
        return Control_session_fail(
            session, "invalid required-send result");
    session->state = CONTROL_SESSION_WAIT_REPLY;
    return CONTROL_SESSION_PENDING;
}

static control_session_result_t Control_session_receive_reply(
    control_session_t *session, session_control_reply_t *reply)
{
    record_receive_result_t result;
    size_t length;

    result = Record_session_receive(
        session->session, session->reply_record,
        sizeof(session->reply_record), &length);
    if (result == RECORD_RECEIVE_EMPTY)
        return CONTROL_SESSION_PENDING;
    if (result == RECORD_RECEIVE_ERROR)
        return Control_session_fail(session, "control response read failed");
    if (result == RECORD_RECEIVE_CLOSED)
        return Control_session_fail(session, "transport closed");
    if (Session_protocol_decode_control_reply(
            session->reply_record, length, reply) == -1)
        return Control_session_fail(session, "invalid control response");
    if (reply->command != session->command)
        return Control_session_fail(session, "unexpected control response");
    if (reply->more)
        return CONTROL_SESSION_REPLY_READY;
    session->state = CONTROL_SESSION_COMPLETE;
    return CONTROL_SESSION_FINISHED;
}

control_session_t *Control_session_create(
    record_session_t *record_session,
    const session_control_request_t *request)
{
    control_session_t *session;

    if (record_session == NULL || request == NULL) {
        errno = EINVAL;
        return NULL;
    }
    session = calloc(1, sizeof(*session));
    if (session == NULL)
        return NULL;
    if (Session_protocol_encode_control_open(
            session->open_record, sizeof(session->open_record),
            &session->open_length, request) == -1) {
        free(session);
        return NULL;
    }
    session->session = record_session;
    session->state = CONTROL_SESSION_SEND_OPEN;
    session->command = request->command;
    return session;
}

control_session_result_t Control_session_step(
    control_session_t *session, session_control_reply_t *reply)
{
    record_flush_result_t flush_result;

    if (session == NULL || reply == NULL) {
        errno = EINVAL;
        return CONTROL_SESSION_ERROR;
    }
    if (session->state == CONTROL_SESSION_FAILED
        || session->state == CONTROL_SESSION_COMPLETE) {
        errno = EALREADY;
        return CONTROL_SESSION_ERROR;
    }
    flush_result = Record_session_flush(session->session);
    if (flush_result == RECORD_FLUSH_ERROR)
        return Control_session_fail(session, "transport output failed");
    if (flush_result == RECORD_FLUSH_CLOSED)
        return Control_session_fail(session, "transport closed");
    if (flush_result == RECORD_FLUSH_PENDING)
        return CONTROL_SESSION_PENDING;

    if (session->state == CONTROL_SESSION_SEND_OPEN)
        return Control_session_send_open(session);
    return Control_session_receive_reply(session, reply);
}

record_session_id_t Control_session_id(const control_session_t *session)
{
    return session != NULL
        ? Record_session_id(session->session) : RECORD_SESSION_ID_INVALID;
}

const char *Control_session_error(const control_session_t *session)
{
    return session != NULL ? session->error : "invalid session";
}

void Control_session_destroy(control_session_t *session)
{
    free(session);
}
