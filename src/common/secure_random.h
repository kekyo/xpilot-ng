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

#ifndef SECURE_RANDOM_H
#define SECURE_RANDOM_H

#include <stdbool.h>
#include <stddef.h>

/**
 * Fill caller-owned storage with cryptographically secure OS entropy.
 *
 * @param destination Writable storage receiving exactly `size` bytes.
 * @param size Positive number of bytes to generate.
 * @return `true` on success, otherwise `false` with the destination cleared.
 */
bool Secure_random_fill(void *destination, size_t size);

#endif /* SECURE_RANDOM_H */
