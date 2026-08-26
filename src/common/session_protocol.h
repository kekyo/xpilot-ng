/*
 * XPilot NG, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SESSION_PROTOCOL_H
#define SESSION_PROTOCOL_H

#include <stddef.h>

#include "const.h"
#include "net.h"
#include "pack.h"
#include "session_token.h"

/** Current version of the backend-neutral session control protocol. */
#define SESSION_PROTOCOL_VERSION 2

/** Largest control-plane record accepted by the session protocol. */
#define SESSION_PROTOCOL_MAX_RECORD_SIZE SERVER_RECV_SIZE

/** Purpose declared by the first record of a newly attached session. */
typedef enum {
    /** Admit the session as a gameplay connection. */
    SESSION_PURPOSE_GAME = 1,
    /** Execute one control request and return one or more replies. */
    SESSION_PURPOSE_CONTROL = 2,
    /** Reattach a replacement transport to an active gameplay session. */
    SESSION_PURPOSE_RESUME = 3
} session_purpose_t;

/** Gameplay admission request carried by a session-open record. */
typedef struct {
    /** Polygon-map protocol version supported by the client. */
    unsigned short polygon_version;
    /** Legacy-map protocol version supported by the client. */
    unsigned short legacy_version;
    /** Operating-system user identity. */
    char user[MAX_CHARS];
    /** Requested in-game nickname. */
    char nick[MAX_CHARS];
    /** Client display identifier. */
    char display[MAX_CHARS];
    /** Client-reported host identifier. */
    char host[MAX_CHARS];
    /** Requested team, or TEAM_NOT_SET. */
    int team;
} session_game_request_t;

/** One-shot server control request carried by a session-open record. */
typedef struct {
    /** Polygon-map protocol version supported by the client. */
    unsigned short polygon_version;
    /** Legacy-map protocol version supported by the client. */
    unsigned short legacy_version;
    /** User claiming to issue the command. */
    char user[MAX_CHARS];
    /** Command identifier from pack.h. */
    unsigned char command;
    /** Command-specific argument. */
    char argument[MSG_LEN];
} session_control_request_t;

/** Gameplay resumption request carried by a session-open record. */
typedef struct {
    /** Bearer token issued when the original gameplay session was admitted. */
    session_token_t token;
} session_resume_request_t;

/** Decoded first record of a newly attached session. */
typedef struct {
    /** Declared purpose selecting the active union member. */
    session_purpose_t purpose;
    /** Purpose-specific request. */
    union {
        /** Gameplay admission data when purpose is SESSION_PURPOSE_GAME. */
        session_game_request_t game;
        /** Control request data when purpose is SESSION_PURPOSE_CONTROL. */
        session_control_request_t control;
        /** Resumption data when purpose is SESSION_PURPOSE_RESUME. */
        session_resume_request_t resume;
    } request;
} session_open_t;

/** Result of gameplay admission. */
typedef struct {
    /** SUCCESS or an E_* admission error from pack.h. */
    unsigned char status;
    /** Protocol selected by the server for the current map. */
    unsigned short selected_version;
    /** Human-readable result description. */
    char reason[MSG_LEN];
} session_game_reply_t;

/** One record in a one-shot control response. */
typedef struct {
    /** Command whose result is being returned. */
    unsigned char command;
    /** SUCCESS or an E_* command error from pack.h. */
    unsigned char status;
    /** Nonzero when another response record follows. */
    unsigned char more;
    /** Command-specific response data. */
    char payload[MSG_LEN];
} session_control_reply_t;

/** Result of attempting to resume an existing gameplay session. */
typedef struct {
    /** SUCCESS or an E_* resumption error from pack.h. */
    unsigned char status;
    /** Human-readable result description. */
    char reason[MSG_LEN];
} session_resume_reply_t;

/**
 * Encode a gameplay admission request as one complete logical record.
 *
 * @param destination Buffer receiving the encoded record.
 * @param capacity Size of destination in bytes.
 * @param length Receives the encoded record length.
 * @param request Request to encode.
 * @return Zero on success, otherwise -1.
 */
int Session_protocol_encode_game_open(
    char *destination, size_t capacity, size_t *length,
    const session_game_request_t *request);

/**
 * Encode a control request as one complete logical record.
 *
 * @param destination Buffer receiving the encoded record.
 * @param capacity Size of destination in bytes.
 * @param length Receives the encoded record length.
 * @param request Request to encode.
 * @return Zero on success, otherwise -1.
 */
int Session_protocol_encode_control_open(
    char *destination, size_t capacity, size_t *length,
    const session_control_request_t *request);

/**
 * Encode a gameplay resumption request as one complete logical record.
 *
 * @param destination Buffer receiving the encoded record.
 * @param capacity Size of destination in bytes.
 * @param length Receives the encoded record length.
 * @param request Request to encode.
 * @return Zero on success, otherwise -1.
 */
int Session_protocol_encode_resume_open(
    char *destination, size_t capacity, size_t *length,
    const session_resume_request_t *request);

/**
 * Decode and validate a complete session-open logical record.
 *
 * @param payload Complete record payload.
 * @param length Payload size in bytes.
 * @param open Receives the purpose and request data.
 * @return Zero on success, otherwise -1 with errno set to EPROTO for malformed
 *         input.
 */
int Session_protocol_decode_open(const char *payload, size_t length,
                                 session_open_t *open);

/**
 * Encode one gameplay admission reply as a complete logical record.
 *
 * @param destination Buffer receiving the encoded record.
 * @param capacity Size of destination in bytes.
 * @param length Receives the encoded record length.
 * @param reply Reply to encode.
 * @return Zero on success, otherwise -1.
 */
int Session_protocol_encode_game_reply(
    char *destination, size_t capacity, size_t *length,
    const session_game_reply_t *reply);

/**
 * Decode and validate one complete gameplay admission reply.
 *
 * @param payload Complete record payload.
 * @param length Payload size in bytes.
 * @param reply Receives the decoded reply.
 * @return Zero on success, otherwise -1.
 */
int Session_protocol_decode_game_reply(const char *payload, size_t length,
                                       session_game_reply_t *reply);

/**
 * Encode one control reply as a complete logical record.
 *
 * @param destination Buffer receiving the encoded record.
 * @param capacity Size of destination in bytes.
 * @param length Receives the encoded record length.
 * @param reply Reply to encode.
 * @return Zero on success, otherwise -1.
 */
int Session_protocol_encode_control_reply(
    char *destination, size_t capacity, size_t *length,
    const session_control_reply_t *reply);

/**
 * Decode and validate one complete control reply logical record.
 *
 * @param payload Complete record payload.
 * @param length Payload size in bytes.
 * @param reply Receives the decoded reply.
 * @return Zero on success, otherwise -1.
 */
int Session_protocol_decode_control_reply(
    const char *payload, size_t length, session_control_reply_t *reply);

/**
 * Encode one gameplay resumption reply as a complete logical record.
 *
 * @param destination Buffer receiving the encoded record.
 * @param capacity Size of destination in bytes.
 * @param length Receives the encoded record length.
 * @param reply Reply to encode.
 * @return Zero on success, otherwise -1.
 */
int Session_protocol_encode_resume_reply(
    char *destination, size_t capacity, size_t *length,
    const session_resume_reply_t *reply);

/**
 * Decode and validate one complete gameplay resumption reply.
 *
 * @param payload Complete record payload.
 * @param length Payload size in bytes.
 * @param reply Receives the decoded reply.
 * @return Zero on success, otherwise -1.
 */
int Session_protocol_decode_resume_reply(
    const char *payload, size_t length, session_resume_reply_t *reply);

#endif /* SESSION_PROTOCOL_H */
