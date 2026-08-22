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

#ifndef CONTACT_SESSION_H
#define CONTACT_SESSION_H

#include "record_transport.h"

/**
 * Attach an accepted logical transport to fixed-endpoint admission.
 *
 * @param transport Transport whose ownership is transferred.
 * @param address Observed peer address.
 * @param peer_port Observed peer source port.
 * @return Zero on success, otherwise -1.
 */
int Contact_attach_transport(record_transport_t *transport,
                             const char *address, int peer_port);

#endif /* CONTACT_SESSION_H */
