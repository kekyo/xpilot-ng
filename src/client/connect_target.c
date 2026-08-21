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

Connect_target_t *Connect_targets_create(int count, char *const *addresses,
                                         const Connect_defaults_t *defaults)
{
    Connect_target_t *targets;
    int index;

    if (count == 0)
        return NULL;
    if (count < 0 || addresses == NULL || defaults == NULL)
        return NULL;

    targets = calloc((size_t)count, sizeof(*targets));
    if (targets == NULL)
        return NULL;
    for (index = 0; index < count; index++) {
        if (!Connect_target_init(&targets[index], addresses[index],
                                 defaults->contact_port,
                                 defaults->contact_transport,
                                 defaults->game_transport)) {
            free(targets);
            return NULL;
        }
    }
    return targets;
}
