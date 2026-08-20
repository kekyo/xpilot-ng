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

#include "game_transport.h"

#include <stddef.h>

static char ascii_lower(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return (char)(ch - 'A' + 'a');
    return ch;
}

static bool ascii_equal_case_insensitive(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0') {
        if (ascii_lower(*left) != ascii_lower(*right))
            return false;
        left++;
        right++;
    }
    return *left == '\0' && *right == '\0';
}

bool Game_transport_parse(const char *value, game_transport_t *transport)
{
    if (value == NULL || transport == NULL)
        return false;
    if (ascii_equal_case_insensitive(value, "udp")) {
        *transport = GAME_TRANSPORT_UDP;
        return true;
    }
    if (ascii_equal_case_insensitive(value, "tcp")) {
        *transport = GAME_TRANSPORT_TCP;
        return true;
    }
    return false;
}

const char *Game_transport_name(game_transport_t transport)
{
    switch (transport) {
    case GAME_TRANSPORT_UDP:
        return "udp";
    case GAME_TRANSPORT_TCP:
        return "tcp";
    default:
        return "unknown";
    }
}

unsigned Game_transport_protocol_version(game_transport_t transport,
                                         bool polygon_map)
{
    switch (transport) {
    case GAME_TRANSPORT_UDP:
        return polygon_map ? GAME_PROTOCOL_UDP_POLYGON_VERSION
                           : GAME_PROTOCOL_UDP_LEGACY_VERSION;
    case GAME_TRANSPORT_TCP:
        return polygon_map ? GAME_PROTOCOL_TCP_POLYGON_VERSION
                           : GAME_PROTOCOL_TCP_LEGACY_VERSION;
    default:
        return 0;
    }
}

bool Game_transport_from_protocol_version(unsigned version,
                                          game_transport_t *transport)
{
    if (transport == NULL)
        return false;

    if (version == GAME_PROTOCOL_TCP_POLYGON_VERSION
        || version == GAME_PROTOCOL_TCP_LEGACY_VERSION) {
        *transport = GAME_TRANSPORT_TCP;
        return true;
    }
    if (version >= 0x4203
        && version <= GAME_PROTOCOL_UDP_POLYGON_VERSION) {
        *transport = GAME_TRANSPORT_UDP;
        return true;
    }
    return false;
}
