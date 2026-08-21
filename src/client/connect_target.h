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

#ifndef CONNECT_TARGET_H
#define CONNECT_TARGET_H

#include "game_transport.h"
#include "pack.h"

#include <stdbool.h>

/** Default endpoint settings applied to direct client connections. */
typedef struct Connect_defaults {
    /** Contact port used when a target does not provide its own port. */
    int contact_port;
    /** Default contact and lobby transport. */
    game_transport_t contact_transport;
    /** Default gameplay transport. */
    game_transport_t game_transport;
} Connect_defaults_t;

/** Immutable endpoint settings for one client connection attempt. */
typedef struct Connect_target {
    /** Hostname or numeric address passed to the socket layer. */
    char address[MAX_HOST_LEN];
    /** Contact port for this endpoint. */
    int contact_port;
    /** Contact and lobby transport for this endpoint. */
    game_transport_t contact_transport;
    /** Gameplay transport expected from this endpoint. */
    game_transport_t game_transport;
} Connect_target_t;

/**
 * Initialize one connection target with explicit endpoint settings.
 *
 * @param target Target to initialize.
 * @param address Nonempty hostname or numeric address.
 * @param contact_port Contact port in the inclusive range 0 through 65535.
 * @param contact_transport Contact and lobby transport.
 * @param game_transport Gameplay transport.
 * @return `true` when all values are valid and fit in the target.
 */
bool Connect_target_init(Connect_target_t *target, const char *address,
                         int contact_port,
                         game_transport_t contact_transport,
                         game_transport_t game_transport);

/**
 * Create connection targets for unqualified command-line addresses.
 *
 * Each returned target receives an independent copy of `defaults`. The caller
 * owns the returned array and must release it with `free()`. A count of zero
 * returns `NULL` without indicating an error.
 *
 * @param count Number of addresses.
 * @param addresses Array containing `count` nonempty addresses.
 * @param defaults Default port and transports copied to every target.
 * @return Allocated target array, or `NULL` for invalid input or allocation
 * failure.
 */
Connect_target_t *Connect_targets_create(int count, char *const *addresses,
                                         const Connect_defaults_t *defaults);

#endif /* CONNECT_TARGET_H */
