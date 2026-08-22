/*
 * XPilot NG, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "xpcommon.h"

#include "record_session.h"

struct record_session {
    record_session_id_t id;
    record_transport_t *transport;
};

static record_session_id_t next_session_id = UINT64_C(1);

record_session_t *Record_session_create(record_transport_t *transport)
{
    record_session_t *session;

    if (transport == NULL) {
        errno = EINVAL;
        return NULL;
    }
    if (next_session_id == RECORD_SESSION_ID_INVALID) {
        Record_transport_destroy(transport);
        errno = EOVERFLOW;
        return NULL;
    }
    session = malloc(sizeof(*session));
    if (session == NULL) {
        Record_transport_destroy(transport);
        return NULL;
    }
    session->id = next_session_id;
    session->transport = transport;
    if (next_session_id == UINT64_MAX)
        next_session_id = RECORD_SESSION_ID_INVALID;
    else
        next_session_id++;
    return session;
}

record_session_id_t Record_session_id(const record_session_t *session)
{
    return session != NULL ? session->id : RECORD_SESSION_ID_INVALID;
}

int Record_session_replace_transport(record_session_t *session,
                                     record_transport_t *transport)
{
    record_transport_t *previous;

    if (session == NULL || transport == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (session->transport == transport)
        return 0;
    previous = session->transport;
    session->transport = transport;
    Record_transport_destroy(previous);
    return 0;
}

record_receive_result_t Record_session_receive(
    record_session_t *session, char *destination, size_t capacity,
    size_t *length)
{
    if (session == NULL) {
        errno = EINVAL;
        return RECORD_RECEIVE_ERROR;
    }
    return Record_transport_receive(
        session->transport, destination, capacity, length);
}

record_send_result_t Record_session_send(record_session_t *session,
                                         const char *payload,
                                         size_t length,
                                         record_delivery_t delivery)
{
    if (session == NULL) {
        errno = EINVAL;
        return RECORD_SEND_ERROR;
    }
    return Record_transport_send(
        session->transport, payload, length, delivery);
}

record_flush_result_t Record_session_flush(record_session_t *session)
{
    if (session == NULL) {
        errno = EINVAL;
        return RECORD_FLUSH_ERROR;
    }
    return Record_transport_flush(session->transport);
}

socket_handle_t Record_session_native_handle(
    const record_session_t *session)
{
    if (session == NULL)
        return SOCK_FD_INVALID;
    return Record_transport_native_handle(session->transport);
}

void Record_session_destroy(record_session_t *session)
{
    if (session == NULL)
        return;
    Record_transport_destroy(session->transport);
    free(session);
}
