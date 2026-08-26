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

#include "xpclient.h"

#include "client_session.h"
#include "control_session.h"
#include "session_connection.h"

static record_session_t *staged_game_session;
static char staged_game_confirmation[CLIENT_RECV_SIZE];
static size_t staged_game_confirmation_length;

static void Session_connection_set_error(char *destination, size_t capacity,
                                         const char *message)
{
    if (destination != NULL && capacity > 0)
        strlcpy(destination, message, capacity);
}

static int Session_connection_wait(record_session_t *session,
                                   time_t deadline)
{
    socket_handle_t handle = Record_session_native_handle(session);
    record_flush_result_t flush_result;
    fd_set read_fds;
    fd_set write_fds;
    struct timeval timeout;
    int status;

    if (handle == SOCK_FD_INVALID) {
        errno = ENOTSUP;
        return -1;
    }
    flush_result = Record_session_flush(session);
    if (flush_result == RECORD_FLUSH_ERROR
        || flush_result == RECORD_FLUSH_CLOSED)
        return -1;
    if (time(NULL) >= deadline) {
        errno = ETIMEDOUT;
        return -1;
    }
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_SET(handle, &read_fds);
    if (flush_result == RECORD_FLUSH_PENDING)
        FD_SET(handle, &write_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
#ifdef _WINDOWS
    status = select(0, &read_fds,
                    flush_result == RECORD_FLUSH_PENDING
                    ? &write_fds : NULL, NULL, &timeout);
#else
    status = select(handle + 1, &read_fds,
                    flush_result == RECORD_FLUSH_PENDING
                    ? &write_fds : NULL, NULL, &timeout);
#endif
    if (status < 0 && errno == EINTR)
        return 0;
    return status < 0 ? -1 : 0;
}

int Session_connection_admit_game(
    record_transport_t *transport, const session_game_request_t *request,
    int timeout_seconds, record_session_t **admitted_session,
    char *confirmation, size_t confirmation_capacity,
    size_t *confirmation_length, unsigned *selected_version,
    char *error_text, size_t error_capacity)
{
    client_session_result_t result;
    client_session_t *admission = NULL;
    record_session_t *session = NULL;
    time_t deadline;

    if (transport == NULL || request == NULL || timeout_seconds <= 0
        || admitted_session == NULL || confirmation == NULL
        || confirmation_capacity == 0 || confirmation_length == NULL
        || selected_version == NULL
        || (error_text == NULL && error_capacity != 0)) {
        Record_transport_destroy(transport);
        errno = EINVAL;
        return -1;
    }
    *admitted_session = NULL;
    *confirmation_length = 0;
    *selected_version = 0;
    Session_connection_set_error(error_text, error_capacity, "");

    session = Record_session_create(transport);
    if (session == NULL)
        goto failure;
    admission = Client_session_create(session, request);
    if (admission == NULL)
        goto failure;
    deadline = time(NULL) + timeout_seconds;

    for (;;) {
        result = Client_session_step(
            admission, confirmation, confirmation_capacity,
            confirmation_length);
        if (result == CLIENT_SESSION_CONFIRMATION_READY)
            break;
        if (result == CLIENT_SESSION_ERROR) {
            Session_connection_set_error(
                error_text, error_capacity, Client_session_error(admission));
            goto failure;
        }
        if (Session_connection_wait(session, deadline) == -1) {
            Session_connection_set_error(
                error_text, error_capacity,
                errno == ETIMEDOUT ? "session admission timed out"
                                   : "session wait failed");
            goto failure;
        }
    }

    *selected_version = Client_session_selected_version(admission);
    Client_session_destroy(admission);
    *admitted_session = session;
    return 0;

failure:
    if (error_text != NULL && error_capacity > 0 && error_text[0] == '\0')
        Session_connection_set_error(
            error_text, error_capacity, "cannot initialize session admission");
    Client_session_destroy(admission);
    Record_session_destroy(session);
    return -1;
}

int Session_connection_resume_game(
    record_session_t *session, const session_token_t *token,
    int timeout_seconds, char *error_text, size_t error_capacity)
{
    client_resume_result_t result;
    client_resume_t *resume;
    time_t deadline;

    if (session == NULL || token == NULL || timeout_seconds <= 0
        || (error_text == NULL && error_capacity != 0)) {
        errno = EINVAL;
        return -1;
    }
    Session_connection_set_error(error_text, error_capacity, "");
    resume = Client_resume_create(session, token);
    if (resume == NULL) {
        Session_connection_set_error(
            error_text, error_capacity,
            "cannot initialize session resumption");
        return -1;
    }
    deadline = time(NULL) + timeout_seconds;

    for (;;) {
        result = Client_resume_step(resume);
        if (result == CLIENT_RESUME_ACCEPTED) {
            Client_resume_destroy(resume);
            return 0;
        }
        if (result == CLIENT_RESUME_ERROR) {
            Session_connection_set_error(
                error_text, error_capacity, Client_resume_error(resume));
            Client_resume_destroy(resume);
            return -1;
        }
        if (Session_connection_wait(session, deadline) == -1) {
            Session_connection_set_error(
                error_text, error_capacity,
                errno == ETIMEDOUT ? "session resumption timed out"
                                   : "session resumption wait failed");
            Client_resume_destroy(resume);
            return -1;
        }
    }
}

int Session_connection_run_control(
    record_transport_t *transport, const session_control_request_t *request,
    int timeout_seconds, session_control_reply_fn receive_reply,
    void *context, char *error_text, size_t error_capacity)
{
    control_session_result_t result;
    control_session_t *control = NULL;
    record_session_t *session = NULL;
    session_control_reply_t reply;
    time_t deadline;
    int status = -1;

    if (transport == NULL || request == NULL || timeout_seconds <= 0
        || receive_reply == NULL
        || (error_text == NULL && error_capacity != 0)) {
        Record_transport_destroy(transport);
        errno = EINVAL;
        return -1;
    }
    Session_connection_set_error(error_text, error_capacity, "");
    session = Record_session_create(transport);
    if (session == NULL)
        goto cleanup;
    control = Control_session_create(session, request);
    if (control == NULL)
        goto cleanup;
    deadline = time(NULL) + timeout_seconds;

    for (;;) {
        result = Control_session_step(control, &reply);
        if (result == CONTROL_SESSION_ERROR) {
            Session_connection_set_error(
                error_text, error_capacity, Control_session_error(control));
            goto cleanup;
        }
        if (result == CONTROL_SESSION_REPLY_READY
            || result == CONTROL_SESSION_FINISHED) {
            if (!receive_reply(&reply, context)) {
                Session_connection_set_error(
                    error_text, error_capacity,
                    "control response callback failed");
                goto cleanup;
            }
            if (result == CONTROL_SESSION_FINISHED) {
                status = 0;
                goto cleanup;
            }
        }
        if (Session_connection_wait(session, deadline) == -1) {
            Session_connection_set_error(
                error_text, error_capacity,
                errno == ETIMEDOUT ? "control request timed out"
                                   : "control session wait failed");
            goto cleanup;
        }
    }

cleanup:
    if (status == -1 && error_text != NULL && error_capacity > 0
        && error_text[0] == '\0')
        Session_connection_set_error(
            error_text, error_capacity, "cannot initialize control session");
    Control_session_destroy(control);
    Record_session_destroy(session);
    return status;
}

int Session_connection_stage_game(record_session_t *session,
                                  const char *confirmation,
                                  size_t confirmation_length)
{
    if (session == NULL || confirmation == NULL
        || confirmation_length == 0
        || confirmation_length > sizeof(staged_game_confirmation)
        || staged_game_session != NULL) {
        Record_session_destroy(session);
        errno = EINVAL;
        return -1;
    }
    staged_game_session = session;
    memcpy(staged_game_confirmation, confirmation, confirmation_length);
    staged_game_confirmation_length = confirmation_length;
    return 0;
}

bool Session_connection_game_is_staged(void)
{
    return staged_game_session != NULL;
}

int Session_connection_take_game(record_session_t **session,
                                 char *confirmation,
                                 size_t confirmation_capacity,
                                 size_t *confirmation_length)
{
    if (session == NULL || confirmation == NULL
        || confirmation_length == NULL || staged_game_session == NULL
        || confirmation_capacity < staged_game_confirmation_length) {
        errno = EINVAL;
        return -1;
    }
    *session = staged_game_session;
    *confirmation_length = staged_game_confirmation_length;
    memcpy(confirmation, staged_game_confirmation,
           staged_game_confirmation_length);
    staged_game_session = NULL;
    staged_game_confirmation_length = 0;
    return 0;
}

void Session_connection_discard_game(void)
{
    Record_session_destroy(staged_game_session);
    staged_game_session = NULL;
    staged_game_confirmation_length = 0;
}
