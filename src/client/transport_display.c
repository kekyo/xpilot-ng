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

#include "transport_display.h"

#include <stdio.h>
#include <string.h>

const char *Transport_display_name(game_transport_t transport)
{
    switch (transport) {
    case GAME_TRANSPORT_UDP:
        return "UDP";
    case GAME_TRANSPORT_TCP:
        return "TCP";
    case GAME_TRANSPORT_WEBSOCKET:
        return "WebSocket";
    default:
        return "UNKNOWN";
    }
}

bool Transport_display_pair(char *output, size_t output_size,
                            game_transport_t contact,
                            game_transport_t gameplay)
{
    const char *contact_name;
    const char *gameplay_name;
    int length;

    if (output == NULL || output_size == 0)
        return false;
    contact_name = Transport_display_name(contact);
    gameplay_name = Transport_display_name(gameplay);
    if (strcmp(contact_name, "UNKNOWN") == 0
        || strcmp(gameplay_name, "UNKNOWN") == 0)
        return false;

    length = snprintf(output, output_size, "%s -> %s",
                      contact_name, gameplay_name);
    return length >= 0 && (size_t)length < output_size;
}

bool Transport_display_gameplay_title(char *output, size_t output_size,
                                      const char *application_title,
                                      const char *server_name,
                                      game_transport_t gameplay)
{
    const char *gameplay_name;
    int length;

    if (output == NULL || output_size == 0 || application_title == NULL
        || application_title[0] == '\0' || server_name == NULL
        || server_name[0] == '\0')
        return false;
    gameplay_name = Transport_display_name(gameplay);
    if (strcmp(gameplay_name, "UNKNOWN") == 0)
        return false;

    length = snprintf(output, output_size, "%s - %s [Gameplay: %s]",
                      application_title, server_name, gameplay_name);
    return length >= 0 && (size_t)length < output_size;
}
