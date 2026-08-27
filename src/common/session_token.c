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

#include "xpcommon.h"

#include "secure_random.h"
#include "session_token.h"

bool Session_token_generate(session_token_t *token)
{
    if (token == NULL)
        return false;
    if (!Secure_random_fill(token, sizeof(*token)))
        return false;
    return true;
}

bool Session_token_equal(const session_token_t *left,
                         const session_token_t *right)
{
    uint32_t difference = 0;
    int i;

    if (left == NULL || right == NULL)
        return false;
    for (i = 0; i < SESSION_TOKEN_WORDS; i++)
        difference |= left->words[i] ^ right->words[i];
    return difference == 0;
}
