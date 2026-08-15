#include "test_helpers.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include "net.h"

typedef struct {
    char bytes[128];
    int length;
    int budget;
} controlled_writer_t;

static controlled_writer_t controlled_writer;

static int write_with_budget(sock_t *sock, char *buf, int len)
{
    int written;

    (void)sock;
    if (controlled_writer.budget == 0) {
	errno = EAGAIN;
	return -1;
    }
    written = len < controlled_writer.budget
	? len : controlled_writer.budget;
    memcpy(controlled_writer.bytes + controlled_writer.length,
	   buf, (size_t)written);
    controlled_writer.length += written;
    controlled_writer.budget -= written;
    return written;
}

static int write_fatal_without_errno(sock_t *sock, char *buf, int len)
{
    (void)sock;
    (void)buf;
    (void)len;
    errno = 0;
    return -1;
}

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

static int test_partial_write_keeps_one_record(void)
{
    static char first[] = "first";
    static char second[] = "second";
    static char third[] = "third";
    static const char expected[] = {
        0, 5, 'f', 'i', 'r', 's', 't',
        0, 5, 't', 'h', 'i', 'r', 'd'
    };
    sock_t send_socket;
    sockbuf_t writer;

    memset(&controlled_writer, 0, sizeof(controlled_writer));
    TEST_CHECK(sock_init(&send_socket) == 0);
    TEST_CHECK(Sockbuf_init(&writer, &send_socket, 64,
                           SOCKBUF_WRITE | SOCKBUF_FRAMED) == 0);

    controlled_writer.budget = 3;
    TEST_CHECK(Sockbuf_write(&writer, first, 5) == 5);
    TEST_CHECK(Sockbuf_flush_framed(&writer, write_with_budget) == 5);
    TEST_CHECK(writer.frame_output_offset == 3);

    TEST_CHECK(Sockbuf_write(&writer, second, 6) == 6);
    TEST_CHECK(Sockbuf_flush_framed(&writer, write_with_budget) == 0);
    TEST_CHECK(errno == EAGAIN);
    TEST_CHECK(writer.len == 0);
    TEST_CHECK(writer.frame_output_offset == 3);

    controlled_writer.budget = 64;
    TEST_CHECK(Sockbuf_flush_framed(&writer, write_with_budget) == 0);
    TEST_CHECK(Sockbuf_write(&writer, third, 5) == 5);
    TEST_CHECK(Sockbuf_flush_framed(&writer, write_with_budget) == 5);
    TEST_CHECK(controlled_writer.length == (int)sizeof(expected));
    TEST_CHECK(memcmp(controlled_writer.bytes, expected,
                      sizeof(expected)) == 0);

    Sockbuf_cleanup(&writer);
    return 0;
}

static int test_fatal_write_without_errno_is_error(void)
{
    static char payload[] = "payload";
    sock_t send_socket;
    sockbuf_t writer;

    TEST_CHECK(sock_init(&send_socket) == 0);
    TEST_CHECK(Sockbuf_init(&writer, &send_socket, 64,
                           SOCKBUF_WRITE | SOCKBUF_FRAMED) == 0);
    TEST_CHECK(Sockbuf_write(&writer, payload, 7) == 7);
    TEST_CHECK(Sockbuf_flush_framed(&writer,
                                    write_fatal_without_errno) == -1);
    TEST_CHECK(errno == EIO);
    TEST_CHECK((writer.state & SOCKBUF_ERROR) != 0);

    Sockbuf_cleanup(&writer);
    return 0;
}

static int test_invalid_frame_length_is_error(void)
{
    static const char empty_record[] = { 0, 0 };
    int fds[2];
    sock_t receive_socket;
    sockbuf_t reader;

    TEST_CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    TEST_CHECK(initialize_socket(&receive_socket, fds[1]) == 0);
    TEST_CHECK(Sockbuf_init(&reader, &receive_socket, 64,
                           SOCKBUF_READ | SOCKBUF_FRAMED) == 0);
    TEST_CHECK(write(fds[0], empty_record, sizeof(empty_record))
               == (ssize_t)sizeof(empty_record));

    TEST_CHECK(Sockbuf_read(&reader) == -1);
    TEST_CHECK(errno == EMSGSIZE);
    TEST_CHECK((reader.state & SOCKBUF_ERROR) != 0);

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
    TEST_CHECK(test_partial_write_keeps_one_record() == 0);
    TEST_CHECK(test_fatal_write_without_errno_is_error() == 0);
    TEST_CHECK(test_invalid_frame_length_is_error() == 0);
    TEST_CHECK(test_eof_is_error() == 0);
    TEST_CHECK(test_tcp_socket_connection() == 0);
    return 0;
}
