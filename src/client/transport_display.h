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

#ifndef TRANSPORT_DISPLAY_H
#define TRANSPORT_DISPLAY_H

#include "game_transport.h"

#include <stdbool.h>
#include <stddef.h>

/** Buffer size required by Transport_display_pair(). */
#define TRANSPORT_DISPLAY_PAIR_SIZE 11

/**
 * Return the user-facing name of a network transport.
 *
 * @param transport Transport to name.
 * @return `UDP`, `TCP`, or `UNKNOWN` for an invalid value.
 */
const char *Transport_display_name(game_transport_t transport);

/**
 * Format contact/lobby and gameplay transports for a server list.
 *
 * @param output Destination buffer.
 * @param output_size Size of the destination buffer.
 * @param contact Contact and lobby transport.
 * @param gameplay Gameplay transport.
 * @return `true` when the complete `UDP -> TCP` form fits and both
 * transports are valid, otherwise `false`.
 */
bool Transport_display_pair(char *output, size_t output_size,
                            game_transport_t contact,
                            game_transport_t gameplay);

/**
 * Format the title of a window used for active gameplay.
 *
 * @param output Destination buffer.
 * @param output_size Size of the destination buffer.
 * @param application_title Application name and version.
 * @param server_name Connected server name.
 * @param gameplay Gameplay transport in use.
 * @return `true` when the complete title fits and all arguments are valid,
 * otherwise `false`.
 */
bool Transport_display_gameplay_title(char *output, size_t output_size,
                                      const char *application_title,
                                      const char *server_name,
                                      game_transport_t gameplay);

#endif /* TRANSPORT_DISPLAY_H */
