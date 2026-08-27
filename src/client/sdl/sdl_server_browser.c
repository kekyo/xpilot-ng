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

#include "sdl_server_browser.h"

#include "commonproto.h"
#include "transport_display.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *display_value(const char *value)
{
    return value != NULL && value[0] != '\0' ? value : "N/A";
}

static bool endpoints_match(const server_info_t *metadata,
                            const Connect_param_t *local)
{
    return metadata != NULL
        && metadata->ip_str != NULL
        && strcmp(metadata->ip_str, local->server_addr) == 0
        && metadata->port == (unsigned)local->contact_port
        && metadata->contact_transport == local->contact_transport
        && metadata->game_transport == local->game_transport;
}

static bool local_endpoints_match(const Connect_param_t *left,
                                  const Connect_param_t *right)
{
    return strcmp(left->server_addr, right->server_addr) == 0
        && left->contact_port == right->contact_port
        && left->contact_transport == right->contact_transport
        && left->game_transport == right->game_transport;
}

static bool metadata_endpoints_match(const server_info_t *left,
                                     const server_info_t *right)
{
    return left != NULL
        && right != NULL
        && left->ip_str != NULL
        && right->ip_str != NULL
        && left->ip_str[0] != '\0'
        && right->ip_str[0] != '\0'
        && strcmp(left->ip_str, right->ip_str) == 0
        && left->port == right->port
        && left->contact_transport == right->contact_transport
        && left->game_transport == right->game_transport;
}

static const server_info_t *find_matching_metadata(
    list_t metadata_servers, const Connect_param_t *local)
{
    list_iter_t iterator;

    if (metadata_servers == NULL)
        return NULL;
    for (iterator = List_begin(metadata_servers);
         iterator != List_end(metadata_servers);
         LI_FORWARD(iterator)) {
        server_info_t *metadata = SI_DATA(iterator);

        if (endpoints_match(metadata, local))
            return metadata;
    }
    return NULL;
}

static bool local_result_is_duplicate(const SdlServerBrowser *browser,
                                      const Connect_param_t *local)
{
    size_t index;

    for (index = 0; index < browser->entry_count; index++) {
        const SdlServerBrowserEntry *entry = &browser->entries[index];

        if (entry->has_local_connection
            && local_endpoints_match(&entry->local_connection, local))
            return true;
    }
    return false;
}

static bool metadata_was_merged(const SdlServerBrowser *browser,
                                const server_info_t *metadata)
{
    size_t index;

    for (index = 0; index < browser->entry_count; index++) {
        const SdlServerBrowserEntry *entry = &browser->entries[index];

        if (entry->metadata == metadata
            || metadata_endpoints_match(entry->metadata, metadata)
            || (entry->has_local_connection
                && endpoints_match(metadata, &entry->local_connection)))
            return true;
    }
    return false;
}

static void set_metadata_display(SdlServerBrowserEntry *entry,
                                 const server_info_t *metadata)
{
    SdlServerDisplay *display = &entry->display;

    display->source = entry->sources == (SDL_SERVER_SOURCE_LAN
                                         | SDL_SERVER_SOURCE_META)
        ? "LAN + Meta" : "Meta";
    display->hostname = display_value(metadata->hostname);
    display->address = display_value(metadata->ip_str);
    display->version = display_value(metadata->version);
    display->mapname = display_value(metadata->mapname);
    display->mapsize = display_value(metadata->mapsize);
    display->author = display_value(metadata->author);
    display->status = display_value(metadata->status);
    display->bases = display_value(metadata->bases_str);
    display->fps = display_value(metadata->fps_str);
    display->playlist = metadata->playlist != NULL ? metadata->playlist : "";
    display->sound = display_value(metadata->sound);
    display->teambases = display_value(metadata->teambases_str);
    display->timing = display_value(metadata->timing);
    display->freebases = display_value(metadata->freebases);
    display->queue = display_value(metadata->queue_str);
    display->users = display_value(metadata->users_str);
    display->transport_pair = display_value(metadata->transport_pair);
    display->port = metadata->port;
    display->contact_transport = metadata->contact_transport;
    display->game_transport = metadata->game_transport;
}

static void set_local_display(SdlServerBrowserEntry *entry)
{
    SdlServerDisplay *display = &entry->display;
    const Connect_param_t *local = &entry->local_connection;

    snprintf(entry->version_storage, sizeof(entry->version_storage),
             "0x%04x", local->server_version);
    if (!Transport_display_pair(
            entry->transport_pair_storage,
            sizeof(entry->transport_pair_storage),
            local->contact_transport, local->game_transport)) {
        strlcpy(entry->transport_pair_storage, "UNKNOWN",
                sizeof(entry->transport_pair_storage));
    }

    display->source = "LAN";
    display->hostname = local->server_name[0] != '\0'
        ? local->server_name : local->server_addr;
    display->address = local->server_addr;
    display->version = entry->version_storage;
    display->mapname = "N/A";
    display->mapsize = "N/A";
    display->author = "N/A";
    display->status = "N/A";
    display->bases = "-";
    display->fps = "-";
    display->playlist = "";
    display->sound = "N/A";
    display->teambases = "-";
    display->timing = "N/A";
    display->freebases = "-";
    display->queue = "-";
    display->users = "-";
    display->transport_pair = entry->transport_pair_storage;
    display->port = (unsigned)local->contact_port;
    display->contact_transport = local->contact_transport;
    display->game_transport = local->game_transport;
}

bool Sdl_server_browser_init(SdlServerBrowser *browser,
                             list_t metadata_servers,
                             const Connect_param_t *local_servers,
                             size_t local_server_count)
{
    size_t metadata_count = metadata_servers != NULL
        ? (size_t)List_size(metadata_servers) : 0;
    size_t capacity;
    size_t index;
    list_iter_t iterator;

    if (browser == NULL
        || (local_server_count > 0 && local_servers == NULL))
        return false;
    browser->entries = NULL;
    browser->entry_count = 0;
    if (local_server_count > SIZE_MAX - metadata_count)
        return false;
    capacity = local_server_count + metadata_count;
    if (capacity == 0)
        return true;
    browser->entries = calloc(capacity, sizeof(*browser->entries));
    if (browser->entries == NULL)
        return false;

    for (index = 0; index < local_server_count; index++) {
        SdlServerBrowserEntry *entry;
        const server_info_t *metadata;

        if (local_result_is_duplicate(browser, &local_servers[index]))
            continue;
        entry = &browser->entries[browser->entry_count++];
        entry->sources = SDL_SERVER_SOURCE_LAN;
        entry->has_local_connection = true;
        entry->local_connection = local_servers[index];
        metadata = find_matching_metadata(metadata_servers,
                                          &local_servers[index]);
        entry->metadata = metadata;
        if (metadata != NULL) {
            entry->sources |= SDL_SERVER_SOURCE_META;
            set_metadata_display(entry, metadata);
        } else {
            set_local_display(entry);
        }
    }

    if (metadata_servers != NULL) {
        for (iterator = List_begin(metadata_servers);
             iterator != List_end(metadata_servers);
             LI_FORWARD(iterator)) {
            server_info_t *metadata = SI_DATA(iterator);
            SdlServerBrowserEntry *entry;

            if (metadata_was_merged(browser, metadata))
                continue;
            entry = &browser->entries[browser->entry_count++];
            entry->sources = SDL_SERVER_SOURCE_META;
            entry->metadata = metadata;
            set_metadata_display(entry, metadata);
        }
    }
    return true;
}

void Sdl_server_browser_cleanup(SdlServerBrowser *browser)
{
    if (browser == NULL)
        return;
    free(browser->entries);
    browser->entries = NULL;
    browser->entry_count = 0;
}
