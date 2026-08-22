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

#ifndef CLIENT_SESSION_CONNECTION_H
#define CLIENT_SESSION_CONNECTION_H

#include <stdbool.h>
#include <stddef.h>

#include "record_session.h"
#include "session_protocol.h"

/** Callback receiving one decoded control response. */
typedef bool (*session_control_reply_fn)(
    const session_control_reply_t *reply, void *context);

/**
 * Admit one connected logical transport as a gameplay session.
 *
 * @param transport Connected transport whose ownership is transferred.
 * @param request Gameplay admission request.
 * @param timeout_seconds Positive total exchange timeout.
 * @param admitted_session Receives the admitted session on success.
 * @param confirmation Storage receiving the first gameplay record.
 * @param confirmation_capacity Size of confirmation.
 * @param confirmation_length Receives the first gameplay record length.
 * @param selected_version Receives the negotiated gameplay version.
 * @param error_text Optional storage receiving a stable diagnostic.
 * @param error_capacity Size of error_text, or zero when it is NULL.
 * @return Zero on success, otherwise -1.
 */
int Session_connection_admit_game(
    record_transport_t *transport, const session_game_request_t *request,
    int timeout_seconds, record_session_t **admitted_session,
    char *confirmation, size_t confirmation_capacity,
    size_t *confirmation_length, unsigned *selected_version,
    char *error_text, size_t error_capacity);

/**
 * Execute one control exchange over a connected logical transport.
 *
 * @param transport Connected transport whose ownership is transferred.
 * @param request Control request.
 * @param timeout_seconds Positive total exchange timeout.
 * @param receive_reply Callback invoked for every response record.
 * @param context Caller context passed to receive_reply.
 * @param error_text Optional storage receiving a stable diagnostic.
 * @param error_capacity Size of error_text, or zero when it is NULL.
 * @return Zero after the final reply, otherwise -1.
 */
int Session_connection_run_control(
    record_transport_t *transport, const session_control_request_t *request,
    int timeout_seconds, session_control_reply_fn receive_reply,
    void *context, char *error_text, size_t error_capacity);

/**
 * Stage an admitted gameplay session for the network client.
 *
 * @param session Admitted session whose ownership is transferred.
 * @param confirmation First gameplay record returned by admission.
 * @param confirmation_length Size of confirmation.
 * @return Zero on success, otherwise -1; ownership is always consumed.
 */
int Session_connection_stage_game(record_session_t *session,
                                  const char *confirmation,
                                  size_t confirmation_length);

/** Return whether an admitted gameplay session is currently staged. */
bool Session_connection_game_is_staged(void);

/**
 * Transfer the currently staged gameplay session and confirmation.
 *
 * @param session Receives the owned admitted session.
 * @param confirmation Storage receiving the first gameplay record.
 * @param confirmation_capacity Size of confirmation.
 * @param confirmation_length Receives the first gameplay record length.
 * @return Zero on success, otherwise -1.
 */
int Session_connection_take_game(record_session_t **session,
                                 char *confirmation,
                                 size_t confirmation_capacity,
                                 size_t *confirmation_length);

/** Close and discard a staged gameplay session, if any. */
void Session_connection_discard_game(void);

#endif /* CLIENT_SESSION_CONNECTION_H */
