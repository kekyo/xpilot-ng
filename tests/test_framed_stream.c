#include "test_helpers.h"

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include "net.h"

static int initialize_socket(sock_t *sock, int fd)
{
    TEST_CHECK(sock_init(sock) == 0);
    sock->fd = fd;
    sock->flags |= SOCK_FLAG_TCP | SOCK_FLAG_CONNECT;
    TEST_CHECK(sock_set_non_blocking(sock, 1) == 0);
    return 0;
}

static int test_segmented_record(void)
{
    static const char record[] = { 0, 5, 'h', 'e', 'l', 'l', 'o' };
    int fds[2];
    sock_t receive_socket;
    sockbuf_t reader;

    TEST_CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TEST_CHECK(initialize_socket(&receive_socket, fds[1]) == 0);
    TEST_CHECK(Sockbuf_init(&reader, &receive_socket, 64,
                           SOCKBUF_READ | SOCKBUF_FRAMED) == 0);

    TEST_CHECK(write(fds[0], record, 1) == 1);
    TEST_CHECK(Sockbuf_read(&reader) == 0);
    TEST_CHECK(Sockbuf_clear(&reader) == 0);

    TEST_CHECK(write(fds[0], record + 1, 3) == 3);
    TEST_CHECK(Sockbuf_read(&reader) == 0);
    TEST_CHECK(Sockbuf_clear(&reader) == 0);

    TEST_CHECK(write(fds[0], record + 4, sizeof(record) - 4)
               == (ssize_t)(sizeof(record) - 4));
    TEST_CHECK(Sockbuf_read(&reader) == 5);
    TEST_CHECK(reader.len == 5);
    TEST_CHECK(memcmp(reader.buf, "hello", 5) == 0);

    Sockbuf_cleanup(&reader);
    close(fds[0]);
    close(fds[1]);
    return 0;
}

static int test_coalesced_records(void)
{
    static const char records[] = {
        0, 3, 'o', 'n', 'e',
        0, 3, 't', 'w', 'o'
    };
    int fds[2];
    sock_t receive_socket;
    sockbuf_t reader;

    TEST_CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TEST_CHECK(initialize_socket(&receive_socket, fds[1]) == 0);
    TEST_CHECK(Sockbuf_init(&reader, &receive_socket, 64,
                           SOCKBUF_READ | SOCKBUF_FRAMED) == 0);
    TEST_CHECK(write(fds[0], records, sizeof(records))
               == (ssize_t)sizeof(records));

    TEST_CHECK(Sockbuf_read(&reader) == 3);
    TEST_CHECK(memcmp(reader.buf, "one", 3) == 0);
    TEST_CHECK(Sockbuf_clear(&reader) == 0);
    TEST_CHECK(Sockbuf_read(&reader) == 3);
    TEST_CHECK(memcmp(reader.buf, "two", 3) == 0);

    Sockbuf_cleanup(&reader);
    close(fds[0]);
    close(fds[1]);
    return 0;
}

static int test_framed_write(void)
{
    static char first[] = "first";
    static char second[] = "second";
    int fds[2];
    sock_t send_socket;
    sock_t receive_socket;
    sockbuf_t writer;
    sockbuf_t reader;

    TEST_CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TEST_CHECK(initialize_socket(&send_socket, fds[0]) == 0);
    TEST_CHECK(initialize_socket(&receive_socket, fds[1]) == 0);
    TEST_CHECK(Sockbuf_init(&writer, &send_socket, 64,
                           SOCKBUF_WRITE | SOCKBUF_FRAMED) == 0);
    TEST_CHECK(Sockbuf_init(&reader, &receive_socket, 64,
                           SOCKBUF_READ | SOCKBUF_FRAMED) == 0);

    TEST_CHECK(Sockbuf_write(&writer, first, 5) == 5);
    TEST_CHECK(Sockbuf_flush(&writer) == 5);
    TEST_CHECK(Sockbuf_write(&writer, second, 6) == 6);
    TEST_CHECK(Sockbuf_flush(&writer) == 6);

    TEST_CHECK(Sockbuf_read(&reader) == 5);
    TEST_CHECK(memcmp(reader.buf, first, 5) == 0);
    TEST_CHECK(Sockbuf_clear(&reader) == 0);
    TEST_CHECK(Sockbuf_read(&reader) == 6);
    TEST_CHECK(memcmp(reader.buf, second, 6) == 0);

    Sockbuf_cleanup(&writer);
    Sockbuf_cleanup(&reader);
    close(fds[0]);
    close(fds[1]);
    return 0;
}

static int test_eof_is_error(void)
{
    int fds[2];
    sock_t receive_socket;
    sockbuf_t reader;

    TEST_CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TEST_CHECK(initialize_socket(&receive_socket, fds[1]) == 0);
    TEST_CHECK(Sockbuf_init(&reader, &receive_socket, 64,
                           SOCKBUF_READ | SOCKBUF_FRAMED) == 0);
    close(fds[0]);

    TEST_CHECK(Sockbuf_read(&reader) == -1);
    TEST_CHECK((reader.state & SOCKBUF_ERROR) != 0);

    Sockbuf_cleanup(&reader);
    close(fds[1]);
    return 0;
}

static int test_tcp_socket_connection(void)
{
    int option;
    int port;
    socklen_t option_size = sizeof(option);
    sock_t listener;
    sock_t client;
    sock_t accepted;

    TEST_CHECK(sock_open_tcp_listener(&listener, "127.0.0.1", 0) == 0);
    port = sock_get_port(&listener);
    TEST_CHECK(port > 0);
    TEST_CHECK(sock_open_tcp_bound(&client, "127.0.0.1", 0) == 0);
    TEST_CHECK(sock_connect(&client, "127.0.0.1", port) == 0);
    TEST_CHECK(sock_accept(&listener, &accepted) == 0);

    option = 0;
    TEST_CHECK(getsockopt(accepted.fd, SOL_SOCKET, SO_TYPE,
                          &option, &option_size) == 0);
    TEST_CHECK(option == SOCK_STREAM);
    TEST_CHECK(strcmp(sock_get_last_addr(&accepted), "127.0.0.1") == 0);

    TEST_CHECK(sock_set_tcp_nodelay(&accepted, 1) == 0);
    option = 0;
    option_size = sizeof(option);
    TEST_CHECK(getsockopt(accepted.fd, IPPROTO_TCP, TCP_NODELAY,
                          &option, &option_size) == 0);
    TEST_CHECK(option != 0);

    sock_close(&accepted);
    sock_close(&client);
    sock_close(&listener);
    return 0;
}

int main(void)
{
    TEST_CHECK(test_segmented_record() == 0);
    TEST_CHECK(test_coalesced_records() == 0);
    TEST_CHECK(test_framed_write() == 0);
    TEST_CHECK(test_eof_is_error() == 0);
    TEST_CHECK(test_tcp_socket_connection() == 0);
    return 0;
}
