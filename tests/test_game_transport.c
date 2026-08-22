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

    TEST_CHECK(GAME_PROTOCOL_UDP_POLYGON_VERSION == 0x4F15);
    TEST_CHECK(GAME_PROTOCOL_UDP_LEGACY_VERSION == 0x4501);
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
    TEST_CHECK(Game_transport_session_protocol_version(
                   GAME_TRANSPORT_TCP, true)
               == GAME_PROTOCOL_TCP_SESSION_POLYGON_VERSION);
    TEST_CHECK(Game_transport_session_protocol_version(
                   GAME_TRANSPORT_TCP, false)
               == GAME_PROTOCOL_TCP_SESSION_LEGACY_VERSION);
    TEST_CHECK(Game_transport_session_protocol_version(
                   GAME_TRANSPORT_UDP, true) == 0);

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
                   GAME_PROTOCOL_TCP_SESSION_POLYGON_VERSION, &transport));
    TEST_CHECK(transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(Game_transport_from_protocol_version(
                   GAME_PROTOCOL_TCP_SESSION_LEGACY_VERSION, &transport));
    TEST_CHECK(transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(Game_transport_from_protocol_version(
                   GAME_PROTOCOL_TCP_LEGACY_VERSION, &transport));
    TEST_CHECK(transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(Game_transport_from_protocol_version(
                   GAME_PROTOCOL_TCP_POLYGON_PRE_RECONNECT_VERSION,
                   &transport));
    TEST_CHECK(transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(Game_transport_from_protocol_version(
                   GAME_PROTOCOL_TCP_LEGACY_PRE_RECONNECT_VERSION,
                   &transport));
    TEST_CHECK(transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(Game_transport_protocol_supports_reconnect(
                   GAME_PROTOCOL_TCP_POLYGON_VERSION));
    TEST_CHECK(Game_transport_protocol_supports_reconnect(
                   GAME_PROTOCOL_TCP_LEGACY_VERSION));
    TEST_CHECK(!Game_transport_protocol_supports_reconnect(
                   GAME_PROTOCOL_TCP_POLYGON_PRE_RECONNECT_VERSION));
    TEST_CHECK(!Game_transport_protocol_supports_reconnect(
                   GAME_PROTOCOL_UDP_POLYGON_VERSION));
    TEST_CHECK(Game_transport_protocol_uses_session(
                   GAME_PROTOCOL_TCP_SESSION_POLYGON_VERSION));
    TEST_CHECK(Game_transport_protocol_uses_session(
                   GAME_PROTOCOL_TCP_SESSION_LEGACY_VERSION));
    TEST_CHECK(!Game_transport_protocol_uses_session(
                   GAME_PROTOCOL_TCP_POLYGON_VERSION));
    TEST_CHECK(!Game_transport_from_protocol_version(0, &transport));
    TEST_CHECK(!Game_transport_from_protocol_version(
                   GAME_PROTOCOL_UDP_POLYGON_VERSION, NULL));
    return 0;
}

static int check_meta_versions(void)
{
    char version[64];
    size_t base_length = 0;
    game_transport_t contact = GAME_TRANSPORT_UDP;
    game_transport_t gameplay = GAME_TRANSPORT_UDP;

    TEST_CHECK(Game_transport_format_meta_version(
                   version, sizeof(version), "4.7.3ng",
                   GAME_TRANSPORT_TCP, GAME_TRANSPORT_UDP));
    TEST_CHECK(strcmp(version, "4.7.3ng+ct=tcp+gt=udp") == 0);
    TEST_CHECK(Game_transport_parse_meta_version(
                   version, &contact, &gameplay, &base_length));
    TEST_CHECK(contact == GAME_TRANSPORT_TCP);
    TEST_CHECK(gameplay == GAME_TRANSPORT_UDP);
    TEST_CHECK(base_length == strlen("4.7.3ng"));

    TEST_CHECK(Game_transport_format_meta_version(
                   version, sizeof(version), "4.7.3ng",
                   GAME_TRANSPORT_UDP, GAME_TRANSPORT_TCP));
    TEST_CHECK(strcmp(version, "4.7.3ng+ct=udp+gt=tcp") == 0);
    TEST_CHECK(Game_transport_parse_meta_version(
                   version, &contact, &gameplay, &base_length));
    TEST_CHECK(contact == GAME_TRANSPORT_UDP);
    TEST_CHECK(gameplay == GAME_TRANSPORT_TCP);

    TEST_CHECK(!Game_transport_format_meta_version(
                    version, 8, "4.7.3ng",
                    GAME_TRANSPORT_TCP, GAME_TRANSPORT_TCP));
    TEST_CHECK(!Game_transport_format_meta_version(
                    version, sizeof(version), "4.7.3ng",
                    (game_transport_t)-1, GAME_TRANSPORT_TCP));
    TEST_CHECK(!Game_transport_parse_meta_version(
                    "4.7.3ng", &contact, &gameplay, &base_length));
    TEST_CHECK(!Game_transport_parse_meta_version(
                    "4.7.3ng+ct=tcp+gt=quic",
                    &contact, &gameplay, &base_length));
    TEST_CHECK(!Game_transport_parse_meta_version(
                    NULL, &contact, &gameplay, &base_length));
    TEST_CHECK(!Game_transport_parse_meta_version(
                    "4.7.3ng+ct=tcp+gt=tcp", NULL,
                    &gameplay, &base_length));
    return 0;
}

int main(void)
{
    TEST_CHECK(check_parse() == 0);
    TEST_CHECK(check_names() == 0);
    TEST_CHECK(check_protocol_versions() == 0);
    TEST_CHECK(check_meta_versions() == 0);
    return 0;
}
