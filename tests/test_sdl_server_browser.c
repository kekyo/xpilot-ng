#include "test_helpers.h"

#include "sdl_server_browser.h"

#include <string.h>

static void initialize_meta_server(server_info_t *server,
                                   char *hostname,
                                   char *address,
                                   unsigned port,
                                   unsigned users)
{
    memset(server, 0, sizeof(*server));
    server->version = "4.7.3infinity";
    server->hostname = hostname;
    server->users_str = "3";
    server->mapname = "meta-map";
    server->mapsize = "100x100";
    server->author = "meta-author";
    server->status = "ok";
    server->bases_str = "10";
    server->fps_str = "20";
    server->playlist = "Alice,Bob,Carol";
    server->sound = "yes";
    server->teambases_str = "0";
    server->timing = "0";
    server->ip_str = address;
    server->freebases = "7";
    server->queue_str = "0";
    server->port = port;
    server->users = users;
    server->fps = 20;
    server->contact_transport = GAME_TRANSPORT_UDP;
    server->game_transport = GAME_TRANSPORT_UDP;
    strlcpy(server->transport_pair, "UDP -> UDP",
            sizeof(server->transport_pair));
}

static void initialize_local_server(Connect_param_t *connection,
                                    const char *hostname,
                                    const char *address,
                                    int port)
{
    memset(connection, 0, sizeof(*connection));
    strlcpy(connection->server_name, hostname,
            sizeof(connection->server_name));
    strlcpy(connection->server_addr, address,
            sizeof(connection->server_addr));
    connection->contact_port = port;
    connection->server_port = port;
    connection->server_version = 0x4f12;
    connection->contact_transport = GAME_TRANSPORT_UDP;
    connection->game_transport = GAME_TRANSPORT_UDP;
}

static int check_local_entries_are_merged_and_prioritized(void)
{
    server_info_t matching_meta;
    server_info_t remote_meta;
    server_info_t duplicate_remote_meta;
    Connect_param_t local_servers[3];
    SdlServerBrowser browser = {0};
    const SdlServerBrowserEntry *entry;
    list_t metadata_servers = List_new();

    TEST_CHECK(metadata_servers != NULL);
    initialize_meta_server(&matching_meta, "meta-local.example",
                           "192.0.2.10", 15345, 3);
    initialize_meta_server(&remote_meta, "remote.example",
                           "198.51.100.20", 15345, 8);
    initialize_meta_server(&duplicate_remote_meta, "duplicate.example",
                           "198.51.100.20", 15345, 8);
    TEST_CHECK(List_push_back(metadata_servers, &matching_meta) != NULL);
    TEST_CHECK(List_push_back(metadata_servers, &remote_meta) != NULL);
    TEST_CHECK(List_push_back(metadata_servers, &duplicate_remote_meta)
               != NULL);

    initialize_local_server(&local_servers[0], "lan-meta.local",
                            "192.0.2.10", 15345);
    initialize_local_server(&local_servers[1], "lan-only.local",
                            "192.0.2.30", 25345);
    local_servers[2] = local_servers[1];

    TEST_CHECK(Sdl_server_browser_init(
                   &browser, metadata_servers, local_servers,
                   sizeof(local_servers) / sizeof(local_servers[0])));
    TEST_CHECK(browser.entry_count == 3);

    entry = &browser.entries[0];
    TEST_CHECK(entry->sources
               == (SDL_SERVER_SOURCE_META | SDL_SERVER_SOURCE_LAN));
    TEST_CHECK(entry->has_local_connection);
    TEST_CHECK(entry->metadata == &matching_meta);
    TEST_CHECK(strcmp(entry->display.source, "LAN + Meta") == 0);
    TEST_CHECK(strcmp(entry->display.hostname, "meta-local.example") == 0);
    TEST_CHECK(strcmp(entry->display.mapname, "meta-map") == 0);
    TEST_CHECK(strcmp(entry->local_connection.server_name,
                      "lan-meta.local") == 0);

    entry = &browser.entries[1];
    TEST_CHECK(entry->sources == SDL_SERVER_SOURCE_LAN);
    TEST_CHECK(entry->has_local_connection);
    TEST_CHECK(entry->metadata == NULL);
    TEST_CHECK(strcmp(entry->display.source, "LAN") == 0);
    TEST_CHECK(strcmp(entry->display.hostname, "lan-only.local") == 0);
    TEST_CHECK(strcmp(entry->display.address, "192.0.2.30") == 0);
    TEST_CHECK(entry->display.port == 25345);
    TEST_CHECK(strcmp(entry->display.mapname, "N/A") == 0);
    TEST_CHECK(strcmp(entry->display.users, "-") == 0);
    TEST_CHECK(strcmp(entry->display.playlist, "") == 0);

    entry = &browser.entries[2];
    TEST_CHECK(entry->sources == SDL_SERVER_SOURCE_META);
    TEST_CHECK(!entry->has_local_connection);
    TEST_CHECK(entry->metadata == &remote_meta);
    TEST_CHECK(strcmp(entry->display.source, "Meta") == 0);
    TEST_CHECK(strcmp(entry->display.hostname, "remote.example") == 0);

    Sdl_server_browser_cleanup(&browser);
    TEST_CHECK(browser.entries == NULL);
    TEST_CHECK(browser.entry_count == 0);
    List_delete(metadata_servers);
    return 0;
}

int main(void)
{
    TEST_CHECK(check_local_entries_are_merged_and_prioritized() == 0);
    return 0;
}
