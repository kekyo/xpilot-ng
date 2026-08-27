/*
 * XPilot Infinity/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2026 XPilot Infinity contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SDL_SERVER_BROWSER_H
#define SDL_SERVER_BROWSER_H

#include "xpclient.h"

#include <stdbool.h>
#include <stddef.h>

/** Origin flags for one SDL server-browser entry. */
typedef enum SdlServerSource {
    /** The entry was returned by a metaserver. */
    SDL_SERVER_SOURCE_META = 1U << 0,
    /** The entry was discovered through UDP LAN broadcast. */
    SDL_SERVER_SOURCE_LAN = 1U << 1
} SdlServerSource;

/** Null-safe values displayed by the SDL server browser. */
typedef struct SdlServerDisplay {
    /** Human-readable discovery source. */
    const char *source;
    /** Server hostname or the best available address. */
    const char *hostname;
    /** Numeric server address when available. */
    const char *address;
    /** Server protocol or application version. */
    const char *version;
    /** Current map name. */
    const char *mapname;
    /** Current map dimensions. */
    const char *mapsize;
    /** Current map author. */
    const char *author;
    /** Current server status. */
    const char *status;
    /** Number of bases as display text. */
    const char *bases;
    /** Frames per second as display text. */
    const char *fps;
    /** Comma-separated player list. */
    const char *playlist;
    /** Sound setting as display text. */
    const char *sound;
    /** Number of team bases as display text. */
    const char *teambases;
    /** Timing setting as display text. */
    const char *timing;
    /** Number of free bases as display text. */
    const char *freebases;
    /** Queue size as display text. */
    const char *queue;
    /** Player count as display text. */
    const char *users;
    /** Contact/lobby and gameplay transports as display text. */
    const char *transport_pair;
    /** Contact port. */
    unsigned port;
    /** Contact and lobby transport. */
    game_transport_t contact_transport;
    /** Gameplay transport. */
    game_transport_t game_transport;
} SdlServerDisplay;

/** One combined metaserver and LAN result. */
typedef struct SdlServerBrowserEntry {
    /** Bitwise combination of `SdlServerSource` values. */
    unsigned sources;
    /** Whether `local_connection` contains a discovered LAN connection. */
    bool has_local_connection;
    /** Complete connection data retained from LAN discovery. */
    Connect_param_t local_connection;
    /** Matching metaserver entry, or `NULL` for a LAN-only result. */
    const server_info_t *metadata;
    /** Null-safe values used by all SDL browser widgets. */
    SdlServerDisplay display;
    /** Storage for a LAN protocol version rendered as hexadecimal. */
    char version_storage[16];
    /** Storage for a LAN transport pair. */
    char transport_pair_storage[TRANSPORT_DISPLAY_PAIR_SIZE];
} SdlServerBrowserEntry;

/** Owned array backing one SDL server-browser window. */
typedef struct SdlServerBrowser {
    /** Combined entries in display order, with LAN entries first. */
    SdlServerBrowserEntry *entries;
    /** Number of valid entries. */
    size_t entry_count;
} SdlServerBrowser;

/**
 * Combine metaserver data and detailed UDP LAN discovery results.
 *
 * LAN entries are placed first. Entries with the same numeric address,
 * contact port, contact transport, and gameplay transport are merged.
 * The metadata list and all strings owned by it must outlive `browser`.
 * `browser` must be zero-initialized or previously cleaned up.
 *
 * @param browser Browser to initialize.
 * @param metadata_servers List of `server_info_t` pointers, or `NULL`.
 * @param local_servers Array of complete LAN connection results, or `NULL`
 * when `local_server_count` is zero.
 * @param local_server_count Number of LAN results in `local_servers`.
 * @return `true` on success, otherwise `false`.
 */
bool Sdl_server_browser_init(SdlServerBrowser *browser,
                             list_t metadata_servers,
                             const Connect_param_t *local_servers,
                             size_t local_server_count);

/**
 * Release entries owned by an SDL server browser.
 *
 * @param browser Browser to clean up. `NULL` is accepted.
 */
void Sdl_server_browser_cleanup(SdlServerBrowser *browser);

#endif /* SDL_SERVER_BROWSER_H */
