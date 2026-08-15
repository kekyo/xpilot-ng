/*
 * XPilot NG, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SESSION_H
#define SESSION_H

#include "const.h"
#include "net.h"
#include "pack.h"

/** Packet type used by the first frame on a newly accepted stream. */
#define PKT_SESSION_OPEN 64
/** Packet type used for a gameplay admission result. */
#define PKT_SESSION_REPLY 65
/** Packet type used for one frame of a control response. */
#define PKT_CONTROL_REPLY 66

/** Purpose carried by the first frame on a newly accepted TCP connection. */
typedef enum {
    /** Promote the accepted stream to a gameplay connection. */
    SESSION_GAME = 0,
    /** Execute one status or administration command and close the stream. */
    SESSION_CONTROL = 1
} session_purpose_t;

/** Gameplay admission data sent in the first TCP frame. */
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
} session_game_open_t;

/** One-shot control request sent in the first TCP frame. */
typedef struct {
    /** Polygon-map protocol version supported by the client. */
    unsigned short polygon_version;
    /** Legacy-map protocol version supported by the client. */
    unsigned short legacy_version;
    /** User claiming to issue the command. */
    char user[MAX_CHARS];
    /** Control command identifier from pack.h. */
    unsigned char command;
    /** Command-specific UTF-8 argument. */
    char argument[MSG_LEN];
} session_control_open_t;

/** Result of gameplay admission. */
typedef struct {
    /** SUCCESS or an E_* admission error from pack.h. */
    unsigned char status;
    /** Protocol selected by the server for the current map. */
    unsigned short selected_version;
    /** Human-readable result description. */
    char reason[MSG_LEN];
} session_reply_t;

/** One frame in a one-shot control response. */
typedef struct {
    /** Command whose result is being returned. */
    unsigned char command;
    /** SUCCESS or an E_* command error from pack.h. */
    unsigned char status;
    /** Nonzero when another response frame follows. */
    unsigned char more;
    /** Command-specific UTF-8 response data. */
    char payload[MSG_LEN];
} session_control_reply_t;

/** Encode a gameplay session-open payload into an empty writable buffer. */
int Session_encode_game_open(sockbuf_t *buffer,
                             const session_game_open_t *request);

/** Decode and validate a complete gameplay session-open payload. */
int Session_decode_game_open(sockbuf_t *buffer,
                             session_game_open_t *request);

/** Encode a control session-open payload into an empty writable buffer. */
int Session_encode_control_open(sockbuf_t *buffer,
                                const session_control_open_t *request);

/** Decode and validate a complete control session-open payload. */
int Session_decode_control_open(sockbuf_t *buffer,
                                session_control_open_t *request);

/** Encode a gameplay admission result into an empty writable buffer. */
int Session_encode_reply(sockbuf_t *buffer, const session_reply_t *reply);

/** Decode and validate a complete gameplay admission result. */
int Session_decode_reply(sockbuf_t *buffer, session_reply_t *reply);

/** Encode one control response frame into an empty writable buffer. */
int Session_encode_control_reply(sockbuf_t *buffer,
                                 const session_control_reply_t *reply);

/** Decode and validate one complete control response frame. */
int Session_decode_control_reply(sockbuf_t *buffer,
                                 session_control_reply_t *reply);

#endif
