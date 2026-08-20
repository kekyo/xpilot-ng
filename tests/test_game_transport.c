#include "test_helpers.h"

#include "game_transport.h"

#include <string.h>

static int check_parse(void)
{
    game_transport_t transport = GAME_TRANSPORT_UDP;

    TEST_CHECK(Game_transport_parse("udp", &transport));
    TEST_CHECK(transport == GAME_TRANSPORT_UDP);
    TEST_CHECK(Game_transport_parse("UDP", &transport));
    TEST_CHECK(transport == GAME_TRANSPORT_UDP);
    TEST_CHECK(Game_transport_parse("tcp", &transport));
    TEST_CHECK(transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(Game_transport_parse("TcP", &transport));
    TEST_CHECK(transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(!Game_transport_parse("", &transport));
    TEST_CHECK(!Game_transport_parse("quic", &transport));
    TEST_CHECK(!Game_transport_parse(NULL, &transport));
    TEST_CHECK(!Game_transport_parse("udp", NULL));
    return 0;
}

static int check_names(void)
{
    TEST_CHECK(strcmp(Game_transport_name(GAME_TRANSPORT_UDP), "udp") == 0);
    TEST_CHECK(strcmp(Game_transport_name(GAME_TRANSPORT_TCP), "tcp") == 0);
    TEST_CHECK(strcmp(Game_transport_name((game_transport_t)-1), "unknown")
               == 0);
    return 0;
}

static int check_protocol_versions(void)
{
    game_transport_t transport = GAME_TRANSPORT_UDP;

    TEST_CHECK(Game_transport_protocol_version(GAME_TRANSPORT_UDP, true)
               == GAME_PROTOCOL_UDP_POLYGON_VERSION);
    TEST_CHECK(Game_transport_protocol_version(GAME_TRANSPORT_UDP, false)
               == GAME_PROTOCOL_UDP_LEGACY_VERSION);
    TEST_CHECK(Game_transport_protocol_version(GAME_TRANSPORT_TCP, true)
               == GAME_PROTOCOL_TCP_POLYGON_VERSION);
    TEST_CHECK(Game_transport_protocol_version(GAME_TRANSPORT_TCP, false)
               == GAME_PROTOCOL_TCP_LEGACY_VERSION);
    TEST_CHECK(Game_transport_protocol_version((game_transport_t)-1, true)
               == 0);

    TEST_CHECK(Game_transport_from_protocol_version(
                   GAME_PROTOCOL_UDP_POLYGON_VERSION, &transport));
    TEST_CHECK(transport == GAME_TRANSPORT_UDP);
    TEST_CHECK(Game_transport_from_protocol_version(
                   GAME_PROTOCOL_UDP_LEGACY_VERSION, &transport));
    TEST_CHECK(transport == GAME_TRANSPORT_UDP);
    TEST_CHECK(Game_transport_from_protocol_version(
                   GAME_PROTOCOL_TCP_POLYGON_VERSION, &transport));
    TEST_CHECK(transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(Game_transport_from_protocol_version(
                   GAME_PROTOCOL_TCP_LEGACY_VERSION, &transport));
    TEST_CHECK(transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(!Game_transport_from_protocol_version(0, &transport));
    TEST_CHECK(!Game_transport_from_protocol_version(
                   GAME_PROTOCOL_UDP_POLYGON_VERSION, NULL));
    return 0;
}

int main(void)
{
    TEST_CHECK(check_parse() == 0);
    TEST_CHECK(check_names() == 0);
    TEST_CHECK(check_protocol_versions() == 0);
    return 0;
}
