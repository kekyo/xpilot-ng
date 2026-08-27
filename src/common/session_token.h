/*
 * XPilot Infinity, a multiplayer space war game.
 *
 * Copyright (C) 2026 XPilot Infinity contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SESSION_TOKEN_H
#define SESSION_TOKEN_H

#include <stdbool.h>
#include <stdint.h>

/** Packet carrying the gameplay resumption token to the client. */
#define PKT_SESSION_TOKEN 64
/** Packet used to authenticate a replacement split TCP gameplay stream. */
#define PKT_RESUME 65

/** Number of 32-bit words in a gameplay resumption token. */
#define SESSION_TOKEN_WORDS 4

/** Opaque bearer token used to resume one gameplay session. */
typedef struct {
    /** Token words serialized in network byte order by the packet layer. */
    uint32_t words[SESSION_TOKEN_WORDS];
} session_token_t;

/**
 * Fill a session token with cryptographically secure operating-system entropy.
 *
 * @param token Token to initialize.
 * @return `true` on success, otherwise `false` with the token cleared.
 */
bool Session_token_generate(session_token_t *token);

/**
 * Compare two session tokens without data-dependent early termination.
 *
 * @param left First token.
 * @param right Second token.
 * @return `true` only when every token word is equal.
 */
bool Session_token_equal(const session_token_t *left,
                         const session_token_t *right);

#endif /* SESSION_TOKEN_H */
