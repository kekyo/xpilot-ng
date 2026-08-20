#include "test_helpers.h"

#include <errno.h>
#include <string.h>

#include "xpcommon.h"

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

static int open_connected_sockets(sock_t *sender, sock_t *receiver)
{
    sock_t listener;
    int port;

    TEST_CHECK(sock_open_tcp_listener(&listener, "127.0.0.1", 0, 1) == 0);
    port = sock_get_port(&listener);
    TEST_CHECK(port > 0);
    TEST_CHECK(sock_open_tcp(sender) == 0);
    TEST_CHECK(sock_connect(sender, "127.0.0.1", port) == 0);
    TEST_CHECK(sock_accept(&listener, receiver) == 0);
    TEST_CHECK(sock_close(&listener) == 0);
    TEST_CHECK(sock_set_tcp_nodelay(sender, 1) == 0);
    TEST_CHECK(sock_set_non_blocking(receiver, 1) == 0);
    return 0;
}

static int test_invalid_initialization(void)
{
    sockbuf_t buffer;

    errno = 0;
    TEST_CHECK(Sockbuf_init(&buffer, NULL, 64,
			   SOCKBUF_READ | SOCKBUF_DGRAM | SOCKBUF_FRAMED)
	       == -1);
    TEST_CHECK(errno == EINVAL);

    errno = 0;
    TEST_CHECK(Sockbuf_init(&buffer, NULL, 0,
			   SOCKBUF_READ | SOCKBUF_FRAMED) == -1);
    TEST_CHECK(errno == EMSGSIZE);
    return 0;
}

static int test_segmented_record(void)
{
    static const char record[] = { 0, 5, 'h', 'e', 'l', 'l', 'o' };
    sock_t send_socket;
    sock_t receive_socket;
    sockbuf_t reader;

    TEST_CHECK(open_connected_sockets(&send_socket, &receive_socket) == 0);
    TEST_CHECK(Sockbuf_init(&reader, &receive_socket, 64,
			   SOCKBUF_READ | SOCKBUF_FRAMED) == 0);

    TEST_CHECK(sock_write(&send_socket, (char *)record, 1) == 1);
    TEST_CHECK(sock_readable(&receive_socket) == 1);
    TEST_CHECK(Sockbuf_read(&reader) == 0);
    TEST_CHECK(Sockbuf_clear(&reader) == 0);

    TEST_CHECK(sock_write(&send_socket, (char *)record + 1, 3) == 3);
    TEST_CHECK(sock_readable(&receive_socket) == 1);
    TEST_CHECK(Sockbuf_read(&reader) == 0);
    TEST_CHECK(Sockbuf_clear(&reader) == 0);

    TEST_CHECK(sock_write(&send_socket, (char *)record + 4,
			  sizeof(record) - 4) == (int)sizeof(record) - 4);
    TEST_CHECK(sock_readable(&receive_socket) == 1);
    TEST_CHECK(Sockbuf_read(&reader) == 5);
    TEST_CHECK(reader.len == 5);
    TEST_CHECK(memcmp(reader.buf, "hello", 5) == 0);

    Sockbuf_cleanup(&reader);
    sock_close(&send_socket);
    sock_close(&receive_socket);
    return 0;
}

static int test_coalesced_records(void)
{
    static const char records[] = {
	0, 3, 'o', 'n', 'e',
	0, 3, 't', 'w', 'o'
    };
    sock_t send_socket;
    sock_t receive_socket;
    sockbuf_t reader;

    TEST_CHECK(open_connected_sockets(&send_socket, &receive_socket) == 0);
    TEST_CHECK(Sockbuf_init(&reader, &receive_socket, 64,
			   SOCKBUF_READ | SOCKBUF_FRAMED) == 0);
    TEST_CHECK(sock_write(&send_socket, (char *)records, sizeof(records))
	       == (int)sizeof(records));

    TEST_CHECK(sock_readable(&receive_socket) == 1);
    TEST_CHECK(Sockbuf_read(&reader) == 3);
    TEST_CHECK(memcmp(reader.buf, "one", 3) == 0);
    TEST_CHECK(Sockbuf_clear(&reader) == 0);
    TEST_CHECK(sock_readable(&receive_socket) == 1);
    TEST_CHECK(Sockbuf_read(&reader) == 3);
    TEST_CHECK(memcmp(reader.buf, "two", 3) == 0);

    Sockbuf_cleanup(&reader);
    sock_close(&send_socket);
    sock_close(&receive_socket);
    return 0;
}

static int test_framed_write(void)
{
    static char first[] = "first";
    static char second[] = "second";
    sock_t send_socket;
    sock_t receive_socket;
    sockbuf_t writer;
    sockbuf_t reader;

    TEST_CHECK(open_connected_sockets(&send_socket, &receive_socket) == 0);
    TEST_CHECK(Sockbuf_init(&writer, &send_socket, 64,
			   SOCKBUF_WRITE | SOCKBUF_FRAMED) == 0);
    TEST_CHECK(Sockbuf_init(&reader, &receive_socket, 64,
			   SOCKBUF_READ | SOCKBUF_FRAMED) == 0);

    TEST_CHECK(Sockbuf_write(&writer, first, 5) == 5);
    TEST_CHECK(Sockbuf_flush(&writer) == 5);
    TEST_CHECK(Sockbuf_write(&writer, second, 6) == 6);
    TEST_CHECK(Sockbuf_flush(&writer) == 6);

    TEST_CHECK(sock_readable(&receive_socket) == 1);
    TEST_CHECK(Sockbuf_read(&reader) == 5);
    TEST_CHECK(memcmp(reader.buf, first, 5) == 0);
    TEST_CHECK(Sockbuf_clear(&reader) == 0);
    TEST_CHECK(sock_readable(&receive_socket) == 1);
    TEST_CHECK(Sockbuf_read(&reader) == 6);
    TEST_CHECK(memcmp(reader.buf, second, 6) == 0);

    Sockbuf_cleanup(&writer);
    Sockbuf_cleanup(&reader);
    sock_close(&send_socket);
    sock_close(&receive_socket);
    return 0;
}

static int test_partial_write_drops_only_blocked_update(void)
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

static int test_locked_packet_buffer_reports_capacity_exhaustion(void)
{
    char occupied[56] = { 0 };
    sockbuf_t writer;

    TEST_CHECK(Sockbuf_init(&writer, NULL, 64,
			   SOCKBUF_WRITE | SOCKBUF_LOCK) == 0);
    TEST_CHECK(Sockbuf_write(&writer, occupied, sizeof(occupied))
	       == (int)sizeof(occupied));
    TEST_CHECK(Packet_printf(&writer, "%c%ld%ld", 42, 100L, 200L) == 0);
    TEST_CHECK(writer.len == (int)sizeof(occupied));

    TEST_CHECK(Sockbuf_clear(&writer) == 0);
    TEST_CHECK(Packet_printf(&writer, "%c%ld%ld", 42, 100L, 200L) == 9);
    Sockbuf_cleanup(&writer);
    return 0;
}

static int test_invalid_frame_length_is_error(void)
{
    static const unsigned char empty_record[] = { 0, 0 };
    static const unsigned char oversized_record[] = { 0, 65 };
    sock_t send_socket;
    sock_t receive_socket;
    sockbuf_t reader;

    TEST_CHECK(open_connected_sockets(&send_socket, &receive_socket) == 0);
    TEST_CHECK(Sockbuf_init(&reader, &receive_socket, 64,
			   SOCKBUF_READ | SOCKBUF_FRAMED) == 0);
    TEST_CHECK(sock_write(&send_socket, (char *)empty_record,
			  sizeof(empty_record)) == (int)sizeof(empty_record));
    TEST_CHECK(sock_readable(&receive_socket) == 1);
    TEST_CHECK(Sockbuf_read(&reader) == -1);
    TEST_CHECK(errno == EMSGSIZE);
    TEST_CHECK((reader.state & SOCKBUF_ERROR) != 0);
    Sockbuf_cleanup(&reader);
    sock_close(&send_socket);
    sock_close(&receive_socket);

    TEST_CHECK(open_connected_sockets(&send_socket, &receive_socket) == 0);
    TEST_CHECK(Sockbuf_init(&reader, &receive_socket, 64,
			   SOCKBUF_READ | SOCKBUF_FRAMED) == 0);
    TEST_CHECK(sock_write(&send_socket, (char *)oversized_record,
			  sizeof(oversized_record))
	       == (int)sizeof(oversized_record));
    TEST_CHECK(sock_readable(&receive_socket) == 1);
    TEST_CHECK(Sockbuf_read(&reader) == -1);
    TEST_CHECK(errno == EMSGSIZE);
    TEST_CHECK((reader.state & SOCKBUF_ERROR) != 0);

    Sockbuf_cleanup(&reader);
    sock_close(&send_socket);
    sock_close(&receive_socket);
    return 0;
}

static int test_eof_is_error(void)
{
    static const unsigned char partial_record[] = { 0, 4, 'x' };
    sock_t send_socket;
    sock_t receive_socket;
    sockbuf_t reader;

    TEST_CHECK(open_connected_sockets(&send_socket, &receive_socket) == 0);
    TEST_CHECK(Sockbuf_init(&reader, &receive_socket, 64,
			   SOCKBUF_READ | SOCKBUF_FRAMED) == 0);
    TEST_CHECK(sock_write(&send_socket, (char *)partial_record,
			  sizeof(partial_record)) == (int)sizeof(partial_record));
    sock_close(&send_socket);

    TEST_CHECK(sock_readable(&receive_socket) == 1);
    TEST_CHECK(Sockbuf_read(&reader) == -1);
    TEST_CHECK(errno == ECONNRESET);
    TEST_CHECK((reader.state & SOCKBUF_ERROR) != 0);

    Sockbuf_cleanup(&reader);
    sock_close(&receive_socket);
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

    TEST_CHECK(sock_open_tcp_listener(&listener, "127.0.0.1", 0, 8) == 0);
    port = sock_get_port(&listener);
    TEST_CHECK(port > 0);
    TEST_CHECK(sock_open_tcp_bound(&client, "127.0.0.1", 0) == 0);
    TEST_CHECK(sock_connect(&client, "127.0.0.1", port) == 0);
    TEST_CHECK(sock_accept(&listener, &accepted) == 0);

    option = 0;
    TEST_CHECK(getsockopt(accepted.fd, SOL_SOCKET, SO_TYPE,
#ifdef _WINDOWS
			  (char *)&option,
#else
			  &option,
#endif
			  &option_size) == 0);
    TEST_CHECK(option == SOCK_STREAM);
    TEST_CHECK(strcmp(sock_get_last_addr(&accepted), "127.0.0.1") == 0);

    TEST_CHECK(sock_set_tcp_nodelay(&accepted, 1) == 0);
    option = 0;
    option_size = sizeof(option);
    TEST_CHECK(getsockopt(accepted.fd, IPPROTO_TCP, TCP_NODELAY,
#ifdef _WINDOWS
			  (char *)&option,
#else
			  &option,
#endif
			  &option_size) == 0);
    TEST_CHECK(option != 0);

    sock_close(&accepted);
    sock_close(&client);
    sock_close(&listener);
    return 0;
}

int main(void)
{
    TEST_CHECK(sock_startup() == 0);
    TEST_CHECK(test_invalid_initialization() == 0);
    TEST_CHECK(test_segmented_record() == 0);
    TEST_CHECK(test_coalesced_records() == 0);
    TEST_CHECK(test_framed_write() == 0);
    TEST_CHECK(test_partial_write_drops_only_blocked_update() == 0);
    TEST_CHECK(test_fatal_write_without_errno_is_error() == 0);
    TEST_CHECK(test_locked_packet_buffer_reports_capacity_exhaustion() == 0);
    TEST_CHECK(test_invalid_frame_length_is_error() == 0);
    TEST_CHECK(test_eof_is_error() == 0);
    TEST_CHECK(test_tcp_socket_connection() == 0);
    sock_cleanup();
    return 0;
}
