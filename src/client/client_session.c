/*
 * XPilot NG, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "xpcommon.h"

#include "client_session.h"

typedef enum {
    CLIENT_SESSION_SEND_OPEN,
    CLIENT_SESSION_WAIT_REPLY,
    CLIENT_SESSION_WAIT_CONFIRMATION,
    CLIENT_SESSION_FINISHED,
    CLIENT_SESSION_FAILED
} client_session_state_t;

struct client_session {
    record_session_t *session;
    client_session_state_t state;
    unsigned short polygon_version;
    unsigned short legacy_version;
    unsigned selected_version;
    size_t open_length;
    char open_record[SESSION_PROTOCOL_MAX_RECORD_SIZE];
    char reply_record[SESSION_PROTOCOL_MAX_RECORD_SIZE];
    char error[MSG_LEN];
};

static client_session_result_t Client_session_fail(
    client_session_t *session, const char *reason)
{
    session->state = CLIENT_SESSION_FAILED;
    strlcpy(session->error, reason, sizeof(session->error));
    return CLIENT_SESSION_ERROR;
}

static int Client_session_flush(client_session_t *session)
{
    record_flush_result_t result = Record_session_flush(session->session);

    if (result == RECORD_FLUSH_ERROR) {
        Client_session_fail(session, "transport output failed");
        return -1;
    }
    if (result == RECORD_FLUSH_CLOSED) {
        Client_session_fail(session, "transport closed");
        return -1;
    }
    return result == RECORD_FLUSH_PENDING ? 0 : 1;
}

static client_session_result_t Client_session_send_open(
    client_session_t *session)
{
    record_send_result_t result = Record_session_send(
        session->session, session->open_record, session->open_length,
        RECORD_DELIVERY_REQUIRED);

    if (result == RECORD_SEND_BACKPRESSURED)
        return CLIENT_SESSION_PENDING;
    if (result == RECORD_SEND_ERROR)
        return Client_session_fail(session, "session request send failed");
    if (result == RECORD_SEND_CLOSED)
        return Client_session_fail(session, "transport closed");
    if (result != RECORD_SEND_ACCEPTED)
        return Client_session_fail(session, "invalid required-send result");
    session->state = CLIENT_SESSION_WAIT_REPLY;
    return CLIENT_SESSION_PENDING;
}

static client_session_result_t Client_session_receive_reply(
    client_session_t *session)
{
    record_receive_result_t receive_result;
    session_game_reply_t reply;
    size_t length;

    receive_result = Record_session_receive(
        session->session, session->reply_record,
        sizeof(session->reply_record), &length);
    if (receive_result == RECORD_RECEIVE_EMPTY)
        return CLIENT_SESSION_PENDING;
    if (receive_result == RECORD_RECEIVE_ERROR)
        return Client_session_fail(session, "session response read failed");
    if (receive_result == RECORD_RECEIVE_CLOSED)
        return Client_session_fail(session, "transport closed");
    if (Session_protocol_decode_game_reply(
            session->reply_record, length, &reply) == -1)
        return Client_session_fail(session, "invalid session response");
    if (reply.status != SUCCESS)
        return Client_session_fail(
            session, reply.reason[0] != '\0' ? reply.reason
                                              : "session rejected");
    if (reply.selected_version != session->polygon_version
        && reply.selected_version != session->legacy_version)
        return Client_session_fail(session, "unsupported protocol version");

    session->selected_version = reply.selected_version;
    session->state = CLIENT_SESSION_WAIT_CONFIRMATION;
    return CLIENT_SESSION_PENDING;
}

static client_session_result_t Client_session_receive_confirmation(
    client_session_t *session, char *confirmation, size_t capacity,
    size_t *length)
{
    record_receive_result_t receive_result = Record_session_receive(
        session->session, confirmation, capacity, length);

    if (receive_result == RECORD_RECEIVE_EMPTY)
        return CLIENT_SESSION_PENDING;
    if (receive_result == RECORD_RECEIVE_ERROR)
        return Client_session_fail(
            session, "session confirmation read failed");
    if (receive_result == RECORD_RECEIVE_CLOSED)
        return Client_session_fail(session, "transport closed");
    session->state = CLIENT_SESSION_FINISHED;
    return CLIENT_SESSION_CONFIRMATION_READY;
}

client_session_t *Client_session_create(
    record_session_t *record_session, const session_game_request_t *request)
{
    client_session_t *session;

    if (record_session == NULL || request == NULL) {
        errno = EINVAL;
        return NULL;
    }
    session = calloc(1, sizeof(*session));
    if (session == NULL)
        return NULL;
    if (Session_protocol_encode_game_open(
            session->open_record, sizeof(session->open_record),
            &session->open_length, request) == -1) {
        free(session);
        return NULL;
    }
    session->session = record_session;
    session->state = CLIENT_SESSION_SEND_OPEN;
    session->polygon_version = request->polygon_version;
    session->legacy_version = request->legacy_version;
    return session;
}

client_session_result_t Client_session_step(
    client_session_t *session, char *confirmation, size_t capacity,
    size_t *length)
{
    int flush_result;

    if (session == NULL || confirmation == NULL || capacity == 0
        || length == NULL) {
        errno = EINVAL;
        return CLIENT_SESSION_ERROR;
    }
    *length = 0;
    if (session->state == CLIENT_SESSION_FAILED
        || session->state == CLIENT_SESSION_FINISHED) {
        errno = EALREADY;
        return CLIENT_SESSION_ERROR;
    }
    flush_result = Client_session_flush(session);
    if (flush_result < 0)
        return CLIENT_SESSION_ERROR;
    if (flush_result == 0)
        return CLIENT_SESSION_PENDING;

    if (session->state == CLIENT_SESSION_SEND_OPEN)
        return Client_session_send_open(session);
    if (session->state == CLIENT_SESSION_WAIT_REPLY)
        return Client_session_receive_reply(session);
    return Client_session_receive_confirmation(
        session, confirmation, capacity, length);
}

unsigned Client_session_selected_version(const client_session_t *session)
{
    return session != NULL ? session->selected_version : 0;
}

record_session_id_t Client_session_id(const client_session_t *session)
{
    return session != NULL
        ? Record_session_id(session->session) : RECORD_SESSION_ID_INVALID;
}

const char *Client_session_error(const client_session_t *session)
{
    return session != NULL ? session->error : "invalid session";
}

void Client_session_destroy(client_session_t *session)
{
    free(session);
}
