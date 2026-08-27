#include "test_helpers.h"

#include "connect_target.h"

#include <stdlib.h>
#include <string.h>

static int check_explicit_targets_are_independent(void)
{
    Connect_target_t targets[2];

    TEST_CHECK(Connect_target_init(&targets[0], "tcp.example", 15345,
                                   GAME_TRANSPORT_TCP,
                                   GAME_TRANSPORT_TCP));
    TEST_CHECK(Connect_target_init(&targets[1], "udp.example", 25345,
                                   GAME_TRANSPORT_UDP,
                                   GAME_TRANSPORT_UDP));

    TEST_CHECK(strcmp(targets[0].address, "tcp.example") == 0);
    TEST_CHECK(targets[0].contact_port == 15345);
    TEST_CHECK(targets[0].contact_transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(targets[0].game_transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(strcmp(targets[1].address, "udp.example") == 0);
    TEST_CHECK(targets[1].contact_port == 25345);
    TEST_CHECK(targets[1].contact_transport == GAME_TRANSPORT_UDP);
    TEST_CHECK(targets[1].game_transport == GAME_TRANSPORT_UDP);
    return 0;
}

static int check_target_specs_preserve_order_and_override_defaults(void)
{
    Connect_defaults_t defaults = {
        15346,
        GAME_TRANSPORT_UDP,
        GAME_TRANSPORT_TCP
    };
    char *addresses[] = {
        "TcP://tcp.example",
        "udp://udp.example:25345",
        "default.example",
        "ws://websocket.example:35345"
    };
    Connect_target_t *targets;
    Connect_target_status_t status;
    int invalid_index;

    status = Connect_targets_parse(4, addresses, &defaults, &targets,
                                   &invalid_index);
    TEST_CHECK(status == CONNECT_TARGET_STATUS_OK);
    TEST_CHECK(invalid_index == -1);
    defaults.contact_port = 1;
    defaults.contact_transport = GAME_TRANSPORT_TCP;
    defaults.game_transport = GAME_TRANSPORT_UDP;

    TEST_CHECK(strcmp(targets[0].address, "tcp.example") == 0);
    TEST_CHECK(strcmp(targets[1].address, "udp.example") == 0);
    TEST_CHECK(strcmp(targets[2].address, "default.example") == 0);
    TEST_CHECK(strcmp(targets[3].address, "websocket.example") == 0);
    TEST_CHECK(targets[0].contact_port == 15346);
    TEST_CHECK(targets[1].contact_port == 25345);
    TEST_CHECK(targets[2].contact_port == 15346);
    TEST_CHECK(targets[3].contact_port == 35345);
    TEST_CHECK(targets[0].contact_transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(targets[0].game_transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(targets[1].contact_transport == GAME_TRANSPORT_UDP);
    TEST_CHECK(targets[1].game_transport == GAME_TRANSPORT_UDP);
    TEST_CHECK(targets[2].contact_transport == GAME_TRANSPORT_UDP);
    TEST_CHECK(targets[2].game_transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(targets[3].contact_transport == GAME_TRANSPORT_WEBSOCKET);
    TEST_CHECK(targets[3].game_transport == GAME_TRANSPORT_WEBSOCKET);
    free(targets);
    return 0;
}

static int check_invalid_targets_are_rejected(void)
{
    Connect_target_t target;
    Connect_target_t *targets;
    Connect_target_status_t status;
    Connect_defaults_t defaults = {
        15345,
        GAME_TRANSPORT_UDP,
        GAME_TRANSPORT_UDP
    };
    char *addresses[] = { "valid.example", NULL };
    char too_long[MAX_HOST_LEN + 1];
    int invalid_index;

    memset(too_long, 'a', sizeof(too_long));
    too_long[sizeof(too_long) - 1] = '\0';

    TEST_CHECK(!Connect_target_init(NULL, "host", 15345,
                                    GAME_TRANSPORT_UDP,
                                    GAME_TRANSPORT_UDP));
    TEST_CHECK(!Connect_target_init(&target, NULL, 15345,
                                    GAME_TRANSPORT_UDP,
                                    GAME_TRANSPORT_UDP));
    TEST_CHECK(!Connect_target_init(&target, "", 15345,
                                    GAME_TRANSPORT_UDP,
                                    GAME_TRANSPORT_UDP));
    TEST_CHECK(!Connect_target_init(&target, too_long, 15345,
                                    GAME_TRANSPORT_UDP,
                                    GAME_TRANSPORT_UDP));
    TEST_CHECK(!Connect_target_init(&target, "host", -1,
                                    GAME_TRANSPORT_UDP,
                                    GAME_TRANSPORT_UDP));
    TEST_CHECK(!Connect_target_init(&target, "host", 65536,
                                    GAME_TRANSPORT_UDP,
                                    GAME_TRANSPORT_UDP));
    TEST_CHECK(!Connect_target_init(&target, "host", 15345,
                                    (game_transport_t)-1,
                                    GAME_TRANSPORT_UDP));
    TEST_CHECK(!Connect_target_init(&target, "host", 15345,
                                    GAME_TRANSPORT_UDP,
                                    (game_transport_t)-1));
    TEST_CHECK(!Connect_target_init(&target, "host", 15345,
                                    GAME_TRANSPORT_WEBSOCKET,
                                    GAME_TRANSPORT_TCP));
    TEST_CHECK(!Connect_target_init(&target, "host", 15345,
                                    GAME_TRANSPORT_UDP,
                                    GAME_TRANSPORT_WEBSOCKET));

    defaults.contact_transport = GAME_TRANSPORT_WEBSOCKET;
    defaults.game_transport = GAME_TRANSPORT_TCP;
    TEST_CHECK(Connect_target_parse(&target, "host", &defaults)
               == CONNECT_TARGET_STATUS_INVALID_ARGUMENT);
    defaults.contact_transport = GAME_TRANSPORT_UDP;
    defaults.game_transport = GAME_TRANSPORT_UDP;

    targets = &target;
    status = Connect_targets_parse(0, NULL, &defaults, &targets,
                                   &invalid_index);
    TEST_CHECK(status == CONNECT_TARGET_STATUS_OK);
    TEST_CHECK(targets == NULL);
    TEST_CHECK(invalid_index == -1);
    status = Connect_targets_parse(-1, addresses, &defaults, &targets,
                                   &invalid_index);
    TEST_CHECK(status == CONNECT_TARGET_STATUS_INVALID_ARGUMENT);
    status = Connect_targets_parse(1, NULL, &defaults, &targets,
                                   &invalid_index);
    TEST_CHECK(status == CONNECT_TARGET_STATUS_INVALID_ARGUMENT);
    status = Connect_targets_parse(1, addresses, NULL, &targets,
                                   &invalid_index);
    TEST_CHECK(status == CONNECT_TARGET_STATUS_INVALID_ARGUMENT);
    status = Connect_targets_parse(2, addresses, &defaults, &targets,
                                   &invalid_index);
    TEST_CHECK(status == CONNECT_TARGET_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(targets == NULL);
    TEST_CHECK(invalid_index == 1);

    addresses[0] = "tcp://";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_MISSING_ADDRESS);
    addresses[0] = "tls://host.example";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_UNSUPPORTED_SCHEME);
    addresses[0] = "tcp://host.example:";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_INVALID_PORT);
    addresses[0] = "tcp://host.example:0";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_INVALID_PORT);
    addresses[0] = "tcp://host.example:65536";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_INVALID_PORT);
    addresses[0] = "tcp://host.example:not-a-port";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_INVALID_PORT);
    addresses[0] = "tcp://host.example/path";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_UNSUPPORTED_SYNTAX);
    addresses[0] = "tcp://host.example?query";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_UNSUPPORTED_SYNTAX);
    addresses[0] = "tcp://host.example#fragment";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_UNSUPPORTED_SYNTAX);
    addresses[0] = "tcp://user@host.example";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_UNSUPPORTED_SYNTAX);
    addresses[0] = "tcp://host%20name.example";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_UNSUPPORTED_SYNTAX);
    addresses[0] = "tcp://host.example\\path";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_UNSUPPORTED_SYNTAX);
    addresses[0] = "tcp://[::1]";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_UNSUPPORTED_SYNTAX);
    addresses[0] = "wss://host.example";
    TEST_CHECK(Connect_target_parse(&target, addresses[0], &defaults)
               == CONNECT_TARGET_STATUS_UNSUPPORTED_SCHEME);
    TEST_CHECK(Connect_target_parse(&target, too_long, &defaults)
               == CONNECT_TARGET_STATUS_ADDRESS_TOO_LONG);
    TEST_CHECK(Connect_target_parse(NULL, "host", &defaults)
               == CONNECT_TARGET_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Connect_target_parse(&target, NULL, &defaults)
               == CONNECT_TARGET_STATUS_INVALID_ARGUMENT);

    addresses[0] = "valid.example";
    addresses[1] = "tls://host.example";
    status = Connect_targets_parse(2, addresses, &defaults, &targets,
                                   &invalid_index);
    TEST_CHECK(status == CONNECT_TARGET_STATUS_UNSUPPORTED_SCHEME);
    TEST_CHECK(targets == NULL);
    TEST_CHECK(invalid_index == 1);
    return 0;
}

int main(void)
{
    TEST_CHECK(check_explicit_targets_are_independent() == 0);
    TEST_CHECK(check_target_specs_preserve_order_and_override_defaults()
               == 0);
    TEST_CHECK(check_invalid_targets_are_rejected() == 0);
    return 0;
}
