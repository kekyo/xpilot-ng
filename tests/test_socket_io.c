#include "test_helpers.h"

#include <string.h>

#include "xpcommon.h"

static int test_nonblocking_accept_and_stream_io(void)
{
    static char request[] = "socket request";
    static char response[] = "socket response";
    char buffer[32];
    sock_t listener;
    sock_t accepted;
    sock_t client;
    int port;

    TEST_CHECK(sock_startup() == SOCK_IS_OK);
    TEST_CHECK(sock_open_tcp_listener(&listener, "127.0.0.1", 0, 1)
	       == SOCK_IS_OK);
    TEST_CHECK(sock_set_non_blocking(&listener, 1) == SOCK_IS_OK);
    TEST_CHECK(sock_accept(&listener, &accepted) == SOCK_IS_ERROR);
    TEST_CHECK(sock_error_is_temporary(&accepted));

    port = sock_get_port(&listener);
    TEST_CHECK(port > 0);
    TEST_CHECK(sock_open_tcp(&client) == SOCK_IS_OK);
    TEST_CHECK(sock_connect(&client, "127.0.0.1", port) == SOCK_IS_OK);
    TEST_CHECK(sock_accept(&listener, &accepted) == SOCK_IS_OK);

    TEST_CHECK(sock_write(&client, request, sizeof(request))
	       == (int)sizeof(request));
    memset(buffer, 0, sizeof(buffer));
    TEST_CHECK(sock_read(&accepted, buffer, sizeof(buffer))
	       == (int)sizeof(request));
    TEST_CHECK(memcmp(buffer, request, sizeof(request)) == 0);

    TEST_CHECK(sock_write(&accepted, response, sizeof(response))
	       == (int)sizeof(response));
    memset(buffer, 0, sizeof(buffer));
    TEST_CHECK(sock_read(&client, buffer, sizeof(buffer))
	       == (int)sizeof(response));
    TEST_CHECK(memcmp(buffer, response, sizeof(response)) == 0);

    TEST_CHECK(sock_close(&accepted) == SOCK_IS_OK);
    TEST_CHECK(sock_close(&client) == SOCK_IS_OK);
    TEST_CHECK(sock_close(&listener) == SOCK_IS_OK);
    sock_cleanup();
    return 0;
}

static int test_tcp_connect_with_timeout(void)
{
    sock_t listener;
    sock_t accepted;
    sock_t client;
    int port;

    TEST_CHECK(sock_startup() == SOCK_IS_OK);
    TEST_CHECK(sock_open_tcp_listener(&listener, "127.0.0.1", 0, 1)
	       == SOCK_IS_OK);
    port = sock_get_port(&listener);
    TEST_CHECK(port > 0);
    TEST_CHECK(sock_open_tcp_bound(&client, "127.0.0.1", 0) == SOCK_IS_OK);
    TEST_CHECK(sock_connect_with_timeout(&client, "127.0.0.1", port, 2)
	       == SOCK_IS_OK);
    TEST_CHECK(sock_accept(&listener, &accepted) == SOCK_IS_OK);
    TEST_CHECK(sock_close(&accepted) == SOCK_IS_OK);
    TEST_CHECK(sock_close(&client) == SOCK_IS_OK);
    TEST_CHECK(sock_close(&listener) == SOCK_IS_OK);

    TEST_CHECK(sock_open_tcp_bound(&client, "127.0.0.1", 0) == SOCK_IS_OK);
    errno = 0;
    TEST_CHECK(sock_connect_with_timeout(&client, "127.0.0.1", port, 2)
	       == SOCK_IS_ERROR);
    TEST_CHECK(errno == ECONNREFUSED || errno == ETIMEDOUT);
    TEST_CHECK(sock_close(&client) == SOCK_IS_OK);

    TEST_CHECK(sock_open_tcp_bound(&client, "127.0.0.1", 0) == SOCK_IS_OK);
    errno = 0;
    TEST_CHECK(sock_connect_with_timeout(&client, "127.0.0.1", port, 0)
	       == SOCK_IS_ERROR);
    TEST_CHECK(errno == EINVAL);
    TEST_CHECK(sock_close(&client) == SOCK_IS_OK);
    sock_cleanup();
    return 0;
}

int main(void)
{
    TEST_CHECK(test_nonblocking_accept_and_stream_io() == 0);
    TEST_CHECK(test_tcp_connect_with_timeout() == 0);
    return 0;
}
