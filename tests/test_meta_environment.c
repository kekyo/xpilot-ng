#define _POSIX_C_SOURCE 200809L

#include "test_helpers.h"

#include "xpcommon.h"
#include "meta.h"
#include "meta_test_support.h"

#include "metaserver.h"

#include <stdlib.h>
#include <string.h>

static int tcp_connect_attempts;

int sock_close(sock_t *sock)
{
    sock->fd = SOCK_FD_INVALID;
    return SOCK_IS_OK;
}

int sock_open_tcp_connected_non_blocking(sock_t *sock, char *host, int port)
{
    (void)host;
    (void)port;
    tcp_connect_attempts++;
    sock->fd = SOCK_FD_INVALID;
    return SOCK_IS_ERROR;
}

void error(const char *format, ...)
{
    (void)format;
}

static int set_environment(const char *host, const char *host_two,
                           const char *port)
{
    if (host == NULL) {
        if (unsetenv("XPILOT_META_HOST") != 0)
            return -1;
    } else if (setenv("XPILOT_META_HOST", host, 1) != 0) {
        return -1;
    }
    if (host_two == NULL) {
        if (unsetenv("XPILOT_META_HOST_TWO") != 0)
            return -1;
    } else if (setenv("XPILOT_META_HOST_TWO", host_two, 1) != 0) {
        return -1;
    }
    if (port == NULL) {
        if (unsetenv("XPILOT_META_PORT") != 0)
            return -1;
    } else if (setenv("XPILOT_META_PORT", port, 1) != 0) {
        return -1;
    }
    return 0;
}

static int endpoint_equals(int index, const char *expected_name,
                           const char *expected_address, int expected_port)
{
    const char *name = NULL;
    const char *address = NULL;
    int port = 0;

    TEST_CHECK(Meta_test_environment(index, &name, &address, &port) == 0);
    TEST_CHECK(strcmp(name, expected_name) == 0);
    TEST_CHECK(strcmp(address, expected_address) == 0);
    TEST_CHECK(port == expected_port);
    return 0;
}

static int check_defaults_are_stable(void)
{
    TEST_CHECK(set_environment(NULL, NULL, NULL) == 0);
    Meta_test_reset_environment();
    Meta_test_apply_environment();
    TEST_CHECK(endpoint_equals(0, META_HOST, META_IP, META_PROG_PORT) == 0);
    TEST_CHECK(endpoint_equals(
                   1, META_HOST_TWO, META_IP_TWO, META_PROG_PORT) == 0);
    return 0;
}

static int check_invalid_ports_are_ignored(void)
{
    static const char *invalid_ports[] = {
        "0", "-1", "65536", "4401trailing", "", "999999999999999999999"
    };
    size_t index;

    for (index = 0;
         index < sizeof(invalid_ports) / sizeof(invalid_ports[0]);
         index++) {
        TEST_CHECK(set_environment(NULL, NULL, invalid_ports[index]) == 0);
        Meta_test_reset_environment();
        Meta_test_apply_environment();
        TEST_CHECK(endpoint_equals(
                       0, META_HOST, META_IP, META_PROG_PORT) == 0);
        TEST_CHECK(endpoint_equals(
                       1, META_HOST_TWO, META_IP_TWO, META_PROG_PORT) == 0);
    }
    return 0;
}

static int check_valid_port_is_applied(void)
{
    TEST_CHECK(set_environment(NULL, NULL, "55221") == 0);
    Meta_test_reset_environment();
    Meta_test_apply_environment();
    TEST_CHECK(endpoint_equals(0, META_HOST, META_IP, 55221) == 0);
    TEST_CHECK(endpoint_equals(1, META_HOST_TWO, META_IP_TWO, 55221) == 0);
    return 0;
}

static int check_private_hosts_fail_closed(void)
{
    const char *primary = "private-meta.example.invalid";
    const char *secondary = "backup-meta.example.invalid";

    TEST_CHECK(set_environment(primary, secondary, "55222") == 0);
    Meta_test_reset_environment();
    Meta_test_apply_environment();

    /* A failed lookup of an overridden hostname must not fall through to the
     * compiled public numeric address. */
    TEST_CHECK(endpoint_equals(0, primary, "", 55222) == 0);
    TEST_CHECK(endpoint_equals(1, secondary, "", 55222) == 0);
    return 0;
}

static int check_unresolved_private_hosts_are_not_connected(void)
{
    const char *primary = "private-meta.example.invalid";
    const char *secondary = "backup-meta.example.invalid";
    int connections = 42;
    int maxfd = 42;

    TEST_CHECK(set_environment(primary, secondary, "55222") == 0);
    Meta_test_reset_environment();
    Meta_test_apply_environment();
    TEST_CHECK(endpoint_equals(0, primary, "", 55222) == 0);
    TEST_CHECK(endpoint_equals(1, secondary, "", 55222) == 0);

    tcp_connect_attempts = 0;
    Meta_connect(&connections, &maxfd);

    TEST_CHECK(connections == 0);
    TEST_CHECK(maxfd == -1);
    TEST_CHECK(tcp_connect_attempts == 0);
    return 0;
}

static int check_invalid_query_is_rejected(void)
{
    const char *name = NULL;
    const char *address = NULL;
    int port = 0;

    TEST_CHECK(Meta_test_environment(-1, &name, &address, &port) == -1);
    TEST_CHECK(Meta_test_environment(NUM_METAS, &name, &address, &port) == -1);
    TEST_CHECK(Meta_test_environment(0, NULL, &address, &port) == -1);
    TEST_CHECK(Meta_test_environment(0, &name, NULL, &port) == -1);
    TEST_CHECK(Meta_test_environment(0, &name, &address, NULL) == -1);
    return 0;
}

int main(void)
{
    TEST_CHECK(check_defaults_are_stable() == 0);
    TEST_CHECK(check_invalid_ports_are_ignored() == 0);
    TEST_CHECK(check_valid_port_is_applied() == 0);
    TEST_CHECK(check_private_hosts_fail_closed() == 0);
    TEST_CHECK(check_unresolved_private_hosts_are_not_connected() == 0);
    TEST_CHECK(check_invalid_query_is_rejected() == 0);
    return 0;
}
