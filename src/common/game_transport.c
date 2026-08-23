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
#include <stdio.h>
#include <string.h>

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
    if (ascii_equal_case_insensitive(value, "websocket")
        || ascii_equal_case_insensitive(value, "ws")) {
        *transport = GAME_TRANSPORT_WEBSOCKET;
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
    case GAME_TRANSPORT_WEBSOCKET:
        return "websocket";
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
    case GAME_TRANSPORT_WEBSOCKET:
        return polygon_map
            ? GAME_PROTOCOL_WEBSOCKET_SESSION_POLYGON_VERSION
            : GAME_PROTOCOL_WEBSOCKET_SESSION_LEGACY_VERSION;
    default:
        return 0;
    }
}

unsigned Game_transport_session_protocol_version(game_transport_t transport,
                                                 bool polygon_map)
{
    switch (transport) {
    case GAME_TRANSPORT_TCP:
        return polygon_map ? GAME_PROTOCOL_TCP_SESSION_POLYGON_VERSION
                           : GAME_PROTOCOL_TCP_SESSION_LEGACY_VERSION;
    case GAME_TRANSPORT_WEBSOCKET:
        return polygon_map
            ? GAME_PROTOCOL_WEBSOCKET_SESSION_POLYGON_VERSION
            : GAME_PROTOCOL_WEBSOCKET_SESSION_LEGACY_VERSION;
    default:
        return 0;
    }
}

bool Game_transport_pair_is_supported(game_transport_t contact,
                                      game_transport_t gameplay)
{
    if (Game_transport_protocol_version(contact, true) == 0
        || Game_transport_protocol_version(gameplay, true) == 0)
        return false;
    if (contact == GAME_TRANSPORT_WEBSOCKET
        || gameplay == GAME_TRANSPORT_WEBSOCKET)
        return contact == GAME_TRANSPORT_WEBSOCKET
            && gameplay == GAME_TRANSPORT_WEBSOCKET;
    return true;
}

bool Game_transport_from_protocol_version(unsigned version,
                                          game_transport_t *transport)
{
    if (transport == NULL)
        return false;

    if (version == GAME_PROTOCOL_TCP_POLYGON_VERSION
        || version == GAME_PROTOCOL_TCP_LEGACY_VERSION
        || version == GAME_PROTOCOL_TCP_SESSION_POLYGON_VERSION
        || version == GAME_PROTOCOL_TCP_SESSION_LEGACY_VERSION
        || version
            == GAME_PROTOCOL_TCP_SESSION_POLYGON_PRE_RECONNECT_VERSION
        || version
            == GAME_PROTOCOL_TCP_SESSION_LEGACY_PRE_RECONNECT_VERSION
        || version == GAME_PROTOCOL_TCP_POLYGON_PRE_RECONNECT_VERSION
        || version == GAME_PROTOCOL_TCP_LEGACY_PRE_RECONNECT_VERSION) {
        *transport = GAME_TRANSPORT_TCP;
        return true;
    }
    if (version == GAME_PROTOCOL_WEBSOCKET_SESSION_POLYGON_VERSION
        || version == GAME_PROTOCOL_WEBSOCKET_SESSION_LEGACY_VERSION
        || version
            == GAME_PROTOCOL_WEBSOCKET_SESSION_POLYGON_PRE_RECONNECT_VERSION
        || version
            == GAME_PROTOCOL_WEBSOCKET_SESSION_LEGACY_PRE_RECONNECT_VERSION) {
        *transport = GAME_TRANSPORT_WEBSOCKET;
        return true;
    }
    if (version >= 0x4203
        && version <= GAME_PROTOCOL_UDP_POLYGON_VERSION) {
        *transport = GAME_TRANSPORT_UDP;
        return true;
    }
    return false;
}

bool Game_transport_protocol_supports_reconnect(unsigned version)
{
    return version == GAME_PROTOCOL_TCP_POLYGON_VERSION
        || version == GAME_PROTOCOL_TCP_LEGACY_VERSION
        || version == GAME_PROTOCOL_TCP_SESSION_POLYGON_VERSION
        || version == GAME_PROTOCOL_TCP_SESSION_LEGACY_VERSION
        || version == GAME_PROTOCOL_WEBSOCKET_SESSION_POLYGON_VERSION
        || version == GAME_PROTOCOL_WEBSOCKET_SESSION_LEGACY_VERSION;
}

bool Game_transport_protocol_uses_session(unsigned version)
{
    return version == GAME_PROTOCOL_TCP_SESSION_POLYGON_VERSION
        || version == GAME_PROTOCOL_TCP_SESSION_LEGACY_VERSION
        || version
            == GAME_PROTOCOL_TCP_SESSION_POLYGON_PRE_RECONNECT_VERSION
        || version
            == GAME_PROTOCOL_TCP_SESSION_LEGACY_PRE_RECONNECT_VERSION
        || version == GAME_PROTOCOL_WEBSOCKET_SESSION_POLYGON_VERSION
        || version == GAME_PROTOCOL_WEBSOCKET_SESSION_LEGACY_VERSION
        || version
            == GAME_PROTOCOL_WEBSOCKET_SESSION_POLYGON_PRE_RECONNECT_VERSION
        || version
            == GAME_PROTOCOL_WEBSOCKET_SESSION_LEGACY_PRE_RECONNECT_VERSION;
}

bool Game_transport_format_meta_version(char *output, size_t output_size,
                                        const char *version,
                                        game_transport_t contact,
                                        game_transport_t gameplay)
{
    const char *contact_name;
    const char *gameplay_name;
    int length;

    if (output == NULL || output_size == 0 || version == NULL
        || !Game_transport_pair_is_supported(contact, gameplay))
        return false;
    contact_name = Game_transport_name(contact);
    gameplay_name = Game_transport_name(gameplay);
    if (strcmp(contact_name, "unknown") == 0
        || strcmp(gameplay_name, "unknown") == 0)
        return false;

    length = snprintf(output, output_size, "%s+ct=%s+gt=%s",
                      version, contact_name, gameplay_name);
    return length >= 0 && (size_t)length < output_size;
}

bool Game_transport_parse_meta_version(const char *version,
                                       game_transport_t *contact,
                                       game_transport_t *gameplay,
                                       size_t *base_length)
{
    static const char contact_marker[] = "+ct=";
    static const char gameplay_marker[] = "+gt=";
    const char *contact_tag;
    const char *contact_value;
    const char *gameplay_tag;
    char contact_text[sizeof("websocket")];
    game_transport_t parsed_contact;
    game_transport_t parsed_gameplay;
    size_t contact_length;

    if (version == NULL || contact == NULL || gameplay == NULL
        || base_length == NULL)
        return false;
    contact_tag = strstr(version, contact_marker);
    if (contact_tag == NULL || contact_tag == version)
        return false;
    contact_value = contact_tag + strlen(contact_marker);
    gameplay_tag = strstr(contact_value,
                          gameplay_marker);
    if (gameplay_tag == NULL)
        return false;

    contact_length = (size_t)(gameplay_tag - contact_value);
    if (contact_length == 0 || contact_length >= sizeof(contact_text))
        return false;
    memcpy(contact_text, contact_value, contact_length);
    contact_text[contact_length] = '\0';
    if (!Game_transport_parse(contact_text, &parsed_contact)
        || !Game_transport_parse(gameplay_tag + strlen(gameplay_marker),
                                 &parsed_gameplay)
        || !Game_transport_pair_is_supported(
            parsed_contact, parsed_gameplay))
        return false;

    *contact = parsed_contact;
    *gameplay = parsed_gameplay;
    *base_length = (size_t)(contact_tag - version);
    return true;
}
