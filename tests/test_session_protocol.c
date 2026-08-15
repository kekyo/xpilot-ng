#include "test_helpers.h"

#include <string.h>

#include "session.h"

static int initialize_buffer(sockbuf_t *buffer, int state)
{
    sock_t socket;

    TEST_CHECK(sock_init(&socket) == 0);
    TEST_CHECK(Sockbuf_init(buffer, &socket, SERVER_RECV_SIZE,
                           state | SOCKBUF_LOCK) == 0);
    return 0;
}

static int copy_packet(sockbuf_t *destination, const sockbuf_t *source)
{
    TEST_CHECK(source->len <= destination->size);
    memcpy(destination->buf, source->buf, (size_t)source->len);
    destination->len = source->len;
    destination->ptr = destination->buf;
    return 0;
}

static int test_game_open_round_trip(void)
{
    session_game_open_t sent;
    session_game_open_t received;
    sockbuf_t writer;
    sockbuf_t reader;

    memset(&sent, 0, sizeof(sent));
    sent.polygon_version = POLYGON_VERSION;
    sent.legacy_version = OLD_VERSION;
    strcpy(sent.user, "pilot");
    strcpy(sent.nick, "Ace");
    strcpy(sent.display, "webgl");
    strcpy(sent.host, "browser");
    sent.team = 2;

    TEST_CHECK(initialize_buffer(&writer, SOCKBUF_WRITE) == 0);
    TEST_CHECK(initialize_buffer(&reader, SOCKBUF_READ) == 0);
    TEST_CHECK(Session_encode_game_open(&writer, &sent) > 0);
    TEST_CHECK(copy_packet(&reader, &writer) == 0);
    TEST_CHECK(Session_decode_game_open(&reader, &received) > 0);
    TEST_CHECK(reader.ptr == reader.buf + reader.len);
    TEST_CHECK(received.polygon_version == POLYGON_VERSION);
    TEST_CHECK(received.legacy_version == OLD_VERSION);
    TEST_CHECK(strcmp(received.user, sent.user) == 0);
    TEST_CHECK(strcmp(received.nick, sent.nick) == 0);
    TEST_CHECK(strcmp(received.display, sent.display) == 0);
    TEST_CHECK(strcmp(received.host, sent.host) == 0);
    TEST_CHECK(received.team == sent.team);

    Sockbuf_cleanup(&reader);
    Sockbuf_cleanup(&writer);
    return 0;
}

static int test_control_open_round_trip(void)
{
    session_control_open_t sent;
    session_control_open_t received;
    sockbuf_t writer;
    sockbuf_t reader;

    memset(&sent, 0, sizeof(sent));
    sent.polygon_version = POLYGON_VERSION;
    sent.legacy_version = OLD_VERSION;
    strcpy(sent.user, "owner");
    sent.command = OPTION_TUNE_pack;
    strcpy(sent.argument, "gameSpeed:12");

    TEST_CHECK(initialize_buffer(&writer, SOCKBUF_WRITE) == 0);
    TEST_CHECK(initialize_buffer(&reader, SOCKBUF_READ) == 0);
    TEST_CHECK(Session_encode_control_open(&writer, &sent) > 0);
    TEST_CHECK(copy_packet(&reader, &writer) == 0);
    TEST_CHECK(Session_decode_control_open(&reader, &received) > 0);
    TEST_CHECK(reader.ptr == reader.buf + reader.len);
    TEST_CHECK(received.polygon_version == POLYGON_VERSION);
    TEST_CHECK(received.legacy_version == OLD_VERSION);
    TEST_CHECK(strcmp(received.user, sent.user) == 0);
    TEST_CHECK(received.command == sent.command);
    TEST_CHECK(strcmp(received.argument, sent.argument) == 0);

    Sockbuf_cleanup(&reader);
    Sockbuf_cleanup(&writer);
    return 0;
}

static int test_replies_round_trip(void)
{
    session_reply_t session_sent;
    session_reply_t session_received;
    session_control_reply_t control_sent;
    session_control_reply_t control_received;
    sockbuf_t writer;
    sockbuf_t reader;

    memset(&session_sent, 0, sizeof(session_sent));
    session_sent.status = SUCCESS;
    session_sent.selected_version = POLYGON_VERSION;
    strcpy(session_sent.reason, "accepted");

    TEST_CHECK(initialize_buffer(&writer, SOCKBUF_WRITE) == 0);
    TEST_CHECK(initialize_buffer(&reader, SOCKBUF_READ) == 0);
    TEST_CHECK(Session_encode_reply(&writer, &session_sent) > 0);
    TEST_CHECK(copy_packet(&reader, &writer) == 0);
    TEST_CHECK(Session_decode_reply(&reader, &session_received) > 0);
    TEST_CHECK(session_received.status == session_sent.status);
    TEST_CHECK(session_received.selected_version
               == session_sent.selected_version);
    TEST_CHECK(strcmp(session_received.reason, session_sent.reason) == 0);

    Sockbuf_clear(&writer);
    Sockbuf_clear(&reader);
    memset(&control_sent, 0, sizeof(control_sent));
    control_sent.command = OPTION_LIST_pack;
    control_sent.status = SUCCESS;
    control_sent.more = 1;
    strcpy(control_sent.payload, "gameSpeed:10");
    TEST_CHECK(Session_encode_control_reply(&writer, &control_sent) > 0);
    TEST_CHECK(copy_packet(&reader, &writer) == 0);
    TEST_CHECK(Session_decode_control_reply(&reader, &control_received) > 0);
    TEST_CHECK(control_received.command == control_sent.command);
    TEST_CHECK(control_received.status == control_sent.status);
    TEST_CHECK(control_received.more == control_sent.more);
    TEST_CHECK(strcmp(control_received.payload, control_sent.payload) == 0);

    Sockbuf_cleanup(&reader);
    Sockbuf_cleanup(&writer);
    return 0;
}

static int test_invalid_open_is_rejected(void)
{
    session_game_open_t sent;
    session_game_open_t received;
    sockbuf_t writer;
    sockbuf_t reader;

    memset(&sent, 0, sizeof(sent));
    sent.polygon_version = POLYGON_VERSION;
    sent.legacy_version = OLD_VERSION;
    strcpy(sent.user, "pilot");
    strcpy(sent.nick, "Ace");
    strcpy(sent.display, "webgl");
    strcpy(sent.host, "browser");

    TEST_CHECK(initialize_buffer(&writer, SOCKBUF_WRITE) == 0);
    TEST_CHECK(initialize_buffer(&reader, SOCKBUF_READ) == 0);
    TEST_CHECK(Session_encode_game_open(&writer, &sent) > 0);
    writer.buf[1] = 0;
    writer.buf[2] = 0;
    TEST_CHECK(copy_packet(&reader, &writer) == 0);
    TEST_CHECK(Session_decode_game_open(&reader, &received) == -1);

    Sockbuf_cleanup(&reader);
    Sockbuf_cleanup(&writer);
    return 0;
}

int main(void)
{
    TEST_CHECK(test_game_open_round_trip() == 0);
    TEST_CHECK(test_control_open_round_trip() == 0);
    TEST_CHECK(test_replies_round_trip() == 0);
    TEST_CHECK(test_invalid_open_is_rejected() == 0);
    return 0;
}
