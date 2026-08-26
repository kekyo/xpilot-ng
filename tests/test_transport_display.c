#include "test_helpers.h"

#include "transport_display.h"
#include "version.h"

#include <string.h>

static int check_names(void)
{
    TEST_CHECK(strcmp(Transport_display_name(GAME_TRANSPORT_UDP), "UDP")
               == 0);
    TEST_CHECK(strcmp(Transport_display_name(GAME_TRANSPORT_TCP), "TCP")
               == 0);
    TEST_CHECK(strcmp(Transport_display_name(GAME_TRANSPORT_WEBSOCKET),
                      "WebSocket") == 0);
    TEST_CHECK(strcmp(Transport_display_name((game_transport_t)-1),
                      "UNKNOWN") == 0);
    return 0;
}

static int check_pairs(void)
{
    char pair[TRANSPORT_DISPLAY_PAIR_SIZE];

    TEST_CHECK(Transport_display_pair(pair, sizeof(pair),
                                     GAME_TRANSPORT_UDP,
                                     GAME_TRANSPORT_UDP));
    TEST_CHECK(strcmp(pair, "UDP -> UDP") == 0);
    TEST_CHECK(Transport_display_pair(pair, sizeof(pair),
                                     GAME_TRANSPORT_UDP,
                                     GAME_TRANSPORT_TCP));
    TEST_CHECK(strcmp(pair, "UDP -> TCP") == 0);
    TEST_CHECK(Transport_display_pair(pair, sizeof(pair),
                                     GAME_TRANSPORT_TCP,
                                     GAME_TRANSPORT_UDP));
    TEST_CHECK(strcmp(pair, "TCP -> UDP") == 0);
    TEST_CHECK(Transport_display_pair(pair, sizeof(pair),
                                     GAME_TRANSPORT_TCP,
                                     GAME_TRANSPORT_TCP));
    TEST_CHECK(strcmp(pair, "TCP -> TCP") == 0);
    TEST_CHECK(Transport_display_pair(pair, sizeof(pair),
                                     GAME_TRANSPORT_WEBSOCKET,
                                     GAME_TRANSPORT_WEBSOCKET));
    TEST_CHECK(strcmp(pair, "WebSocket -> WebSocket") == 0);

    TEST_CHECK(!Transport_display_pair(pair, sizeof(pair) - 1,
                                      GAME_TRANSPORT_WEBSOCKET,
                                      GAME_TRANSPORT_WEBSOCKET));
    TEST_CHECK(!Transport_display_pair(pair, sizeof(pair),
                                      (game_transport_t)-1,
                                      GAME_TRANSPORT_TCP));
    TEST_CHECK(!Transport_display_pair(NULL, sizeof(pair),
                                      GAME_TRANSPORT_UDP,
                                      GAME_TRANSPORT_TCP));
    return 0;
}

static int check_gameplay_titles(void)
{
    char title[128];

    TEST_CHECK(Transport_display_gameplay_title(
                   title, sizeof(title), TITLE,
                   "game.example", GAME_TRANSPORT_TCP));
    TEST_CHECK(strcmp(
                   title,
                   "XPilot Infinity 4.7.3 - game.example [Gameplay: TCP]")
               == 0);
    TEST_CHECK(Transport_display_gameplay_title(
                   title, sizeof(title), TITLE,
                   "127.0.0.1", GAME_TRANSPORT_UDP));
    TEST_CHECK(strcmp(
                   title,
                   "XPilot Infinity 4.7.3 - 127.0.0.1 [Gameplay: UDP]")
               == 0);
    TEST_CHECK(Transport_display_gameplay_title(
                   title, sizeof(title), TITLE,
                   "ws.example", GAME_TRANSPORT_WEBSOCKET));
    TEST_CHECK(strcmp(
                   title,
                   "XPilot Infinity 4.7.3 - ws.example [Gameplay: WebSocket]")
               == 0);

    TEST_CHECK(!Transport_display_gameplay_title(
                    title, 12, TITLE, "game.example",
                    GAME_TRANSPORT_TCP));
    TEST_CHECK(!Transport_display_gameplay_title(
                    title, sizeof(title), NULL, "game.example",
                    GAME_TRANSPORT_TCP));
    TEST_CHECK(!Transport_display_gameplay_title(
                    title, sizeof(title), TITLE, NULL,
                    GAME_TRANSPORT_TCP));
    TEST_CHECK(!Transport_display_gameplay_title(
                    title, sizeof(title), TITLE, "game.example",
                    (game_transport_t)-1));
    return 0;
}

int main(void)
{
    TEST_CHECK(check_names() == 0);
    TEST_CHECK(check_pairs() == 0);
    TEST_CHECK(check_gameplay_titles() == 0);
    return 0;
}
