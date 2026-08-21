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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _WINDOWS
#ifndef _CRT_RAND_S
#define _CRT_RAND_S
#endif
#endif

#include "xpcommon.h"

#include "session_token.h"

#ifdef _WINDOWS
static bool Fill_secure_random(void *destination, size_t size)
{
    unsigned char *bytes = destination;
    size_t offset = 0;

    while (offset < size) {
        unsigned int random_value;
        size_t remaining;
        size_t copied;

        if (rand_s(&random_value) != 0)
            return false;
        remaining = size - offset;
        copied = MIN(remaining, sizeof(random_value));
        memcpy(bytes + offset, &random_value, copied);
        offset += copied;
    }
    return true;
}
#else
static bool Fill_secure_random(void *destination, size_t size)
{
    FILE *random_file;
    size_t bytes_read;

    random_file = fopen("/dev/urandom", "rb");
    if (random_file == NULL)
        return false;
    bytes_read = fread(destination, 1, size, random_file);
    if (fclose(random_file) != 0 || bytes_read != size)
        return false;
    return true;
}
#endif

bool Session_token_generate(session_token_t *token)
{
    if (token == NULL)
        return false;
    if (!Fill_secure_random(token, sizeof(*token))) {
        memset(token, 0, sizeof(*token));
        return false;
    }
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
