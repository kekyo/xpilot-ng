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

static int check_defaults_are_copied_to_each_address(void)
{
    Connect_defaults_t defaults = {
        15346,
        GAME_TRANSPORT_TCP,
        GAME_TRANSPORT_UDP
    };
    char *addresses[] = { "first.example", "second.example" };
    Connect_target_t *targets;

    targets = Connect_targets_create(2, addresses, &defaults);
    TEST_CHECK(targets != NULL);
    defaults.contact_port = 1;
    defaults.contact_transport = GAME_TRANSPORT_UDP;
    defaults.game_transport = GAME_TRANSPORT_TCP;

    TEST_CHECK(strcmp(targets[0].address, "first.example") == 0);
    TEST_CHECK(strcmp(targets[1].address, "second.example") == 0);
    TEST_CHECK(targets[0].contact_port == 15346);
    TEST_CHECK(targets[1].contact_port == 15346);
    TEST_CHECK(targets[0].contact_transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(targets[1].contact_transport == GAME_TRANSPORT_TCP);
    TEST_CHECK(targets[0].game_transport == GAME_TRANSPORT_UDP);
    TEST_CHECK(targets[1].game_transport == GAME_TRANSPORT_UDP);
    free(targets);
    return 0;
}

static int check_invalid_targets_are_rejected(void)
{
    Connect_target_t target;
    Connect_defaults_t defaults = {
        15345,
        GAME_TRANSPORT_UDP,
        GAME_TRANSPORT_UDP
    };
    char *addresses[] = { "valid.example", NULL };
    char too_long[MAX_HOST_LEN + 1];

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
    TEST_CHECK(Connect_targets_create(0, NULL, NULL) == NULL);
    TEST_CHECK(Connect_targets_create(-1, addresses, &defaults) == NULL);
    TEST_CHECK(Connect_targets_create(1, NULL, &defaults) == NULL);
    TEST_CHECK(Connect_targets_create(1, addresses, NULL) == NULL);
    TEST_CHECK(Connect_targets_create(2, addresses, &defaults) == NULL);
    return 0;
}

int main(void)
{
    TEST_CHECK(check_explicit_targets_are_independent() == 0);
    TEST_CHECK(check_defaults_are_copied_to_each_address() == 0);
    TEST_CHECK(check_invalid_targets_are_rejected() == 0);
    return 0;
}
