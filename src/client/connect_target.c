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

#include "connect_target.h"

#include <stdlib.h>
#include <string.h>

static bool transport_is_valid(game_transport_t transport)
{
    return Game_transport_protocol_version(transport, true) != 0;
}

static char ascii_lower(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return (char)(ch - 'A' + 'a');
    return ch;
}

static bool scheme_equals(const char *scheme, size_t length,
                          const char *expected)
{
    size_t index;

    if (length != strlen(expected))
        return false;
    for (index = 0; index < length; index++) {
        if (ascii_lower(scheme[index]) != expected[index])
            return false;
    }
    return true;
}

static bool defaults_are_valid(const Connect_defaults_t *defaults)
{
    return defaults != NULL
        && defaults->contact_port >= 0
        && defaults->contact_port <= 65535
        && transport_is_valid(defaults->contact_transport)
        && transport_is_valid(defaults->game_transport);
}

static bool address_syntax_is_supported(const char *address, size_t length)
{
    size_t index;

    for (index = 0; index < length; index++) {
        unsigned char ch = (unsigned char)address[index];

        if (ch <= 0x20 || ch == 0x7f
            || strchr("/:?#@[]%\\", (int)ch) != NULL)
            return false;
    }
    return true;
}

static bool parse_explicit_port(const char *text, int *port)
{
    unsigned value = 0;

    if (*text == '\0')
        return false;
    while (*text != '\0') {
        unsigned digit;

        if (*text < '0' || *text > '9')
            return false;
        digit = (unsigned)(*text - '0');
        if (value > (65535U - digit) / 10U)
            return false;
        value = value * 10U + digit;
        text++;
    }
    if (value == 0)
        return false;
    *port = (int)value;
    return true;
}

bool Connect_target_init(Connect_target_t *target, const char *address,
                         int contact_port,
                         game_transport_t contact_transport,
                         game_transport_t game_transport)
{
    size_t address_length;

    if (target == NULL || address == NULL)
        return false;
    address_length = strlen(address);
    if (address_length == 0 || address_length >= sizeof(target->address)
        || contact_port < 0 || contact_port > 65535
        || !transport_is_valid(contact_transport)
        || !transport_is_valid(game_transport))
        return false;

    memcpy(target->address, address, address_length + 1);
    target->contact_port = contact_port;
    target->contact_transport = contact_transport;
    target->game_transport = game_transport;
    return true;
}

Connect_target_status_t Connect_target_parse(
    Connect_target_t *target, const char *specification,
    const Connect_defaults_t *defaults)
{
    const char *address;
    const char *port_separator;
    const char *scheme_separator;
    size_t address_length;
    game_transport_t contact_transport;
    game_transport_t game_transport;
    int contact_port;
    char parsed_address[MAX_HOST_LEN];

    if (target == NULL || specification == NULL
        || !defaults_are_valid(defaults))
        return CONNECT_TARGET_STATUS_INVALID_ARGUMENT;

    address = specification;
    contact_port = defaults->contact_port;
    contact_transport = defaults->contact_transport;
    game_transport = defaults->game_transport;
    scheme_separator = strstr(specification, "://");
    if (scheme_separator != NULL) {
        size_t scheme_length = (size_t)(scheme_separator - specification);

        if (scheme_equals(specification, scheme_length, "tcp"))
            contact_transport = game_transport = GAME_TRANSPORT_TCP;
        else if (scheme_equals(specification, scheme_length, "udp"))
            contact_transport = game_transport = GAME_TRANSPORT_UDP;
        else
            return CONNECT_TARGET_STATUS_UNSUPPORTED_SCHEME;
        address = scheme_separator + 3;
    }

    if (*address == '\0')
        return CONNECT_TARGET_STATUS_MISSING_ADDRESS;
    if (strpbrk(address, "/?#@[]%\\") != NULL)
        return CONNECT_TARGET_STATUS_UNSUPPORTED_SYNTAX;

    port_separator = strchr(address, ':');
    if (port_separator != NULL) {
        if (scheme_separator == NULL)
            return CONNECT_TARGET_STATUS_UNSUPPORTED_SYNTAX;
        address_length = (size_t)(port_separator - address);
        if (address_length == 0)
            return CONNECT_TARGET_STATUS_MISSING_ADDRESS;
        if (!parse_explicit_port(port_separator + 1, &contact_port))
            return CONNECT_TARGET_STATUS_INVALID_PORT;
    }
    else {
        address_length = strlen(address);
    }

    if (address_length >= sizeof(parsed_address))
        return CONNECT_TARGET_STATUS_ADDRESS_TOO_LONG;
    if (!address_syntax_is_supported(address, address_length))
        return CONNECT_TARGET_STATUS_UNSUPPORTED_SYNTAX;

    memcpy(parsed_address, address, address_length);
    parsed_address[address_length] = '\0';
    if (!Connect_target_init(target, parsed_address, contact_port,
                             contact_transport, game_transport))
        return CONNECT_TARGET_STATUS_INVALID_ARGUMENT;
    return CONNECT_TARGET_STATUS_OK;
}

Connect_target_status_t Connect_targets_parse(
    int count, char *const *specifications,
    const Connect_defaults_t *defaults, Connect_target_t **targets,
    int *invalid_index)
{
    Connect_target_t *parsed_targets;
    Connect_target_status_t status;
    int index;

    if (targets == NULL || invalid_index == NULL)
        return CONNECT_TARGET_STATUS_INVALID_ARGUMENT;
    *targets = NULL;
    *invalid_index = -1;
    if (count < 0 || !defaults_are_valid(defaults)
        || (count > 0 && specifications == NULL))
        return CONNECT_TARGET_STATUS_INVALID_ARGUMENT;
    if (count == 0)
        return CONNECT_TARGET_STATUS_OK;

    parsed_targets = calloc((size_t)count, sizeof(*parsed_targets));
    if (parsed_targets == NULL)
        return CONNECT_TARGET_STATUS_NO_MEMORY;
    for (index = 0; index < count; index++) {
        status = Connect_target_parse(&parsed_targets[index],
                                      specifications[index], defaults);
        if (status != CONNECT_TARGET_STATUS_OK) {
            free(parsed_targets);
            *invalid_index = index;
            return status;
        }
    }
    *targets = parsed_targets;
    return CONNECT_TARGET_STATUS_OK;
}

const char *Connect_target_status_message(Connect_target_status_t status)
{
    switch (status) {
    case CONNECT_TARGET_STATUS_OK:
        return "no error";
    case CONNECT_TARGET_STATUS_INVALID_ARGUMENT:
        return "invalid connection target arguments or defaults";
    case CONNECT_TARGET_STATUS_UNSUPPORTED_SCHEME:
        return "unsupported scheme; expected tcp:// or udp://";
    case CONNECT_TARGET_STATUS_MISSING_ADDRESS:
        return "missing server host";
    case CONNECT_TARGET_STATUS_UNSUPPORTED_SYNTAX:
        return "unsupported syntax; expected HOST, tcp://HOST[:PORT], or "
            "udp://HOST[:PORT]";
    case CONNECT_TARGET_STATUS_INVALID_PORT:
        return "port must be an integer from 1 through 65535";
    case CONNECT_TARGET_STATUS_ADDRESS_TOO_LONG:
        return "server host is too long";
    case CONNECT_TARGET_STATUS_NO_MEMORY:
        return "not enough memory for connection targets";
    default:
        return "unknown connection target error";
    }
}
