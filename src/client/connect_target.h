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

struct Connect_param;

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

/** Outcome of contacting an ordered set of explicit server endpoints. */
typedef struct Contact_servers_result {
    /** At least one server returned a valid contact response. */
    bool contacted;
    /** A contact response led to a completed gameplay join. */
    bool connected;
} Contact_servers_result_t;

/** Result of parsing one or more command-line connection targets. */
typedef enum Connect_target_status {
    /** Every target was parsed successfully. */
    CONNECT_TARGET_STATUS_OK = 0,
    /** A required argument or default setting is invalid. */
    CONNECT_TARGET_STATUS_INVALID_ARGUMENT,
    /** A qualified target uses a scheme other than `tcp` or `udp`. */
    CONNECT_TARGET_STATUS_UNSUPPORTED_SCHEME,
    /** The target does not contain a host address. */
    CONNECT_TARGET_STATUS_MISSING_ADDRESS,
    /** The target contains URI components or address syntax not supported. */
    CONNECT_TARGET_STATUS_UNSUPPORTED_SYNTAX,
    /** An explicit port is not an integer in the range 1 through 65535. */
    CONNECT_TARGET_STATUS_INVALID_PORT,
    /** The parsed host does not fit in `Connect_target_t`. */
    CONNECT_TARGET_STATUS_ADDRESS_TOO_LONG,
    /** Memory for the target array could not be allocated. */
    CONNECT_TARGET_STATUS_NO_MEMORY
} Connect_target_status_t;

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
 * Parse one command-line connection target.
 *
 * A bare `HOST` copies all defaults. `tcp://HOST[:PORT]` and
 * `udp://HOST[:PORT]` select that transport for both contact/lobby and
 * gameplay. An explicit port overrides the default contact port. Paths,
 * credentials, queries, fragments, percent encoding, and IPv6 literals are
 * not supported.
 *
 * @param target Receives the parsed endpoint.
 * @param specification Target specification from a positional argument.
 * @param defaults Port and transports used for unqualified values.
 * @return Detailed parsing status.
 */
Connect_target_status_t Connect_target_parse(
    Connect_target_t *target, const char *specification,
    const Connect_defaults_t *defaults);

/**
 * Parse an ordered array of command-line connection targets.
 *
 * On success, the caller owns `*targets` and must release it with `free()`.
 * A count of zero succeeds with `*targets` set to `NULL`. On failure, no
 * partial array is returned and `invalid_index` identifies the rejected
 * specification, or is set to `-1` for an argument or allocation failure.
 *
 * @param count Number of target specifications.
 * @param specifications Array containing `count` target specifications.
 * @param defaults Default port and transports.
 * @param targets Receives the allocated target array or `NULL`.
 * @param invalid_index Receives the rejected specification index or `-1`.
 * @return Detailed parsing or allocation status.
 */
Connect_target_status_t Connect_targets_parse(
    int count, char *const *specifications,
    const Connect_defaults_t *defaults, Connect_target_t **targets,
    int *invalid_index);

/**
 * Contact explicit endpoints in order until one joins successfully.
 *
 * A failed attempt is isolated from subsequent targets. `conpar` is updated
 * with endpoint and negotiated connection data only after a successful join.
 *
 * @param count Number of entries in `targets`.
 * @param targets Connection targets to try in command-line order.
 * @param auto_connect Whether to request joining without an interactive prompt.
 * @param list_servers Whether to request and print server status.
 * @param auto_shutdown Whether to request server shutdown.
 * @param shutdown_message Shutdown reason used when `auto_shutdown` is set.
 * @param conpar Client identity input and successful connection output.
 * @return Separate indicators for receiving a response and completing a join.
 */
Contact_servers_result_t Contact_servers_detailed(
    int count, const Connect_target_t *targets,
    int auto_connect, int list_servers,
    int auto_shutdown, char *shutdown_message,
    struct Connect_param *conpar);

/**
 * Discover and optionally contact servers using UDP LAN broadcast.
 *
 * Unlike the legacy parallel discovery arrays, `found_servers` retains every
 * negotiated field needed to join a selected server later.
 *
 * @param defaults Default contact port and expected gameplay transport.
 * @param auto_connect Whether to request joining without an interactive prompt.
 * @param list_servers Set to 2 to collect discovery results, or 1 to print
 * server status; zero performs the normal contact interaction.
 * @param auto_shutdown Whether to request server shutdown.
 * @param shutdown_message Shutdown reason used when `auto_shutdown` is set.
 * @param find_max Capacity of `found_servers`.
 * @param num_found Receives the number of stored discovery results, or `NULL`.
 * @param found_servers Optional array receiving complete negotiated connection
 * parameters for each discovered server.
 * @param conpar Client identity input and successful connection output.
 * @return Nonzero after a successful join, otherwise zero. Discovery-only
 * operation reports its result count through `num_found` and returns zero.
 */
int Contact_local_servers_detailed(
    const Connect_defaults_t *defaults,
    int auto_connect, int list_servers,
    int auto_shutdown, char *shutdown_message,
    int find_max, int *num_found,
    struct Connect_param *found_servers,
    struct Connect_param *conpar);

/**
 * Return a user-facing explanation for a connection target status.
 *
 * @param status Status returned by a connection target parser.
 * @return Static English text describing the status.
 */
const char *Connect_target_status_message(Connect_target_status_t status);

#endif /* CONNECT_TARGET_H */
