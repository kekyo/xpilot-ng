#include "test_helpers.h"

#include <errno.h>
#include <string.h>

#include "xpcommon.h"
#include "client_session.h"
#include "control_session.h"
#include "game_transport.h"
#include "record_session.h"
#include "record_transport.h"
#include "session_acceptor.h"
#include "session_protocol.h"

static session_game_request_t make_game_request(void)
{
    session_game_request_t request;

    memset(&request, 0, sizeof(request));
    request.polygon_version = GAME_PROTOCOL_TCP_POLYGON_VERSION;
    request.legacy_version = GAME_PROTOCOL_TCP_LEGACY_VERSION;
    strlcpy(request.user, "Jos\xc3\xa9", sizeof(request.user));
    strlcpy(request.nick, "test-nick", sizeof(request.nick));
    strlcpy(request.display, "\xe6\x97\xa5\xe6\x9c\xac", sizeof(request.display));
    strlcpy(request.host, "test-host", sizeof(request.host));
    request.team = TEAM_NOT_SET;
    return request;
}

static session_control_request_t make_control_request(void)
{
    session_control_request_t request;

    memset(&request, 0, sizeof(request));
    request.polygon_version = GAME_PROTOCOL_TCP_POLYGON_VERSION;
    request.legacy_version = GAME_PROTOCOL_TCP_LEGACY_VERSION;
    strlcpy(request.user, "test-owner", sizeof(request.user));
    request.command = OPTION_LIST_pack;
    strlcpy(request.argument, "shipMass", sizeof(request.argument));
    return request;
}

static int send_game_reply(record_session_t *session,
                           unsigned char status,
                           unsigned short selected_version,
                           const char *reason)
{
    char payload[SESSION_PROTOCOL_MAX_RECORD_SIZE];
    session_game_reply_t reply;
    size_t length;

    memset(&reply, 0, sizeof(reply));
    reply.status = status;
    reply.selected_version = selected_version;
    strlcpy(reply.reason, reason, sizeof(reply.reason));
    TEST_CHECK(Session_protocol_encode_game_reply(
                   payload, sizeof(payload), &length, &reply) == 0);
    TEST_CHECK(Record_session_send(
                   session, payload, length,
                   RECORD_DELIVERY_REQUIRED) == RECORD_SEND_ACCEPTED);
    return 0;
}

static int send_control_reply(record_session_t *session,
                              unsigned char command,
                              unsigned char status,
                              unsigned char more,
                              const char *text)
{
    char payload[SESSION_PROTOCOL_MAX_RECORD_SIZE];
    session_control_reply_t reply;
    size_t length;

    memset(&reply, 0, sizeof(reply));
    reply.command = command;
    reply.status = status;
    reply.more = more;
    strlcpy(reply.payload, text, sizeof(reply.payload));
    TEST_CHECK(Session_protocol_encode_control_reply(
                   payload, sizeof(payload), &length, &reply) == 0);
    TEST_CHECK(Record_session_send(
                   session, payload, length,
                   RECORD_DELIVERY_REQUIRED) == RECORD_SEND_ACCEPTED);
    return 0;
}

static session_token_t make_resume_token(void)
{
    session_token_t token = { {
        UINT32_C(0x01234567), UINT32_C(0x89abcdef),
        UINT32_C(0xfedcba98), UINT32_C(0x76543210)
    } };

    return token;
}

static int send_resume_reply(record_session_t *session,
                             unsigned char status, const char *reason)
{
    char payload[SESSION_PROTOCOL_MAX_RECORD_SIZE];
    session_resume_reply_t reply;
    size_t length;

    memset(&reply, 0, sizeof(reply));
    reply.status = status;
    strlcpy(reply.reason, reason, sizeof(reply.reason));
    TEST_CHECK(Session_protocol_encode_resume_reply(
                   payload, sizeof(payload), &length, &reply) == 0);
    TEST_CHECK(Record_session_send(
                   session, payload, length,
                   RECORD_DELIVERY_REQUIRED) == RECORD_SEND_ACCEPTED);
    return 0;
}

static int test_protocol_round_trip_and_exact_record_validation(void)
{
    char payload[SESSION_PROTOCOL_MAX_RECORD_SIZE + 1];
    session_game_request_t request = make_game_request();
    session_open_t open;
    size_t length;

    TEST_CHECK(Session_protocol_encode_game_open(
                   payload, sizeof(payload), &length, &request) == 0);
    TEST_CHECK(Session_protocol_decode_open(payload, length, &open) == 0);
    TEST_CHECK(open.purpose == SESSION_PURPOSE_GAME);
    TEST_CHECK(open.request.game.polygon_version
               == GAME_PROTOCOL_TCP_POLYGON_VERSION);
    TEST_CHECK(open.request.game.legacy_version
               == GAME_PROTOCOL_TCP_LEGACY_VERSION);
    TEST_CHECK(strcmp(open.request.game.user, request.user) == 0);
    TEST_CHECK(strcmp(open.request.game.nick, request.nick) == 0);
    TEST_CHECK(strcmp(open.request.game.display, request.display) == 0);
    TEST_CHECK(strcmp(open.request.game.host, request.host) == 0);
    TEST_CHECK(open.request.game.team == TEAM_NOT_SET);

    payload[length] = 'x';
    errno = 0;
    TEST_CHECK(Session_protocol_decode_open(
                   payload, length + 1, &open) == -1);
    TEST_CHECK(errno == EPROTO);

    TEST_CHECK(Session_protocol_encode_game_open(
                   payload, sizeof(payload), &length, &request) == 0);
    payload[5] = 0;
    payload[6] = 0;
    errno = 0;
    TEST_CHECK(Session_protocol_decode_open(payload, length, &open) == -1);
    TEST_CHECK(errno == EPROTO);
    return 0;
}

static int test_resume_protocol_round_trip_and_exact_validation(void)
{
    char payload[SESSION_PROTOCOL_MAX_RECORD_SIZE + 1];
    session_resume_request_t request;
    session_resume_reply_t decoded_reply;
    session_resume_reply_t reply;
    session_open_t open;
    size_t length;

    request.token = make_resume_token();
    TEST_CHECK(Session_protocol_encode_resume_open(
                   payload, sizeof(payload), &length, &request) == 0);
    TEST_CHECK(Session_protocol_decode_open(payload, length, &open) == 0);
    TEST_CHECK(open.purpose == SESSION_PURPOSE_RESUME);
    TEST_CHECK(Session_token_equal(
                   &open.request.resume.token, &request.token));
    payload[length] = 'x';
    errno = 0;
    TEST_CHECK(Session_protocol_decode_open(
                   payload, length + 1, &open) == -1);
    TEST_CHECK(errno == EPROTO);

    memset(&reply, 0, sizeof(reply));
    reply.status = E_NOT_FOUND;
    strlcpy(reply.reason, "unknown session", sizeof(reply.reason));
    TEST_CHECK(Session_protocol_encode_resume_reply(
                   payload, sizeof(payload), &length, &reply) == 0);
    TEST_CHECK(Session_protocol_decode_resume_reply(
                   payload, length, &decoded_reply) == 0);
    TEST_CHECK(decoded_reply.status == E_NOT_FOUND);
    TEST_CHECK(strcmp(decoded_reply.reason, "unknown session") == 0);
    payload[length] = 'x';
    errno = 0;
    TEST_CHECK(Session_protocol_decode_resume_reply(
                   payload, length + 1, &decoded_reply) == -1);
    TEST_CHECK(errno == EPROTO);
    return 0;
}

static int test_resume_exchange_transfers_transport_to_stable_session(void)
{
    static const char continued[] = "continued gameplay";
    char received[SESSION_PROTOCOL_MAX_RECORD_SIZE];
    session_token_t token = make_resume_token();
    session_open_t open;
    record_session_id_t stable_id;
    record_session_t *client_session;
    record_session_t *incoming_session;
    record_session_t *stable_session;
    record_transport_t *abandoned_client_transport;
    record_transport_t *abandoned_server_transport;
    record_transport_t *client_transport;
    record_transport_t *server_transport;
    client_resume_t *resume;
    session_acceptor_t *acceptor;
    size_t received_length;

    TEST_CHECK(Record_transport_create_memory_pair(
                   4, SESSION_PROTOCOL_MAX_RECORD_SIZE * 4,
                   &abandoned_client_transport,
                   &abandoned_server_transport) == 0);
    stable_session = Record_session_create(abandoned_server_transport);
    TEST_CHECK(stable_session != NULL);
    stable_id = Record_session_id(stable_session);
    Record_session_close(stable_session);
    TEST_CHECK(Record_transport_receive(
                   abandoned_client_transport, received, sizeof(received),
                   &received_length) == RECORD_RECEIVE_CLOSED);

    TEST_CHECK(Record_transport_create_memory_pair(
                   4, SESSION_PROTOCOL_MAX_RECORD_SIZE * 4,
                   &client_transport, &server_transport) == 0);
    client_session = Record_session_create(client_transport);
    incoming_session = Record_session_create(server_transport);
    TEST_CHECK(client_session != NULL);
    TEST_CHECK(incoming_session != NULL);
    resume = Client_resume_create(client_session, &token);
    acceptor = Session_acceptor_create(incoming_session);
    TEST_CHECK(resume != NULL);
    TEST_CHECK(acceptor != NULL);

    TEST_CHECK(Client_resume_step(resume) == CLIENT_RESUME_PENDING);
    TEST_CHECK(Session_acceptor_step(
                   acceptor, &open) == SESSION_ACCEPTOR_RESUME_READY);
    TEST_CHECK(Session_token_equal(&open.request.resume.token, &token));
    incoming_session = Session_acceptor_take_session(acceptor);
    TEST_CHECK(incoming_session != NULL);
    TEST_CHECK(send_resume_reply(
                   incoming_session, SUCCESS, "resumed") == 0);
    TEST_CHECK(Record_session_replace_from(
                   stable_session, incoming_session) == 0);
    incoming_session = NULL;
    TEST_CHECK(Record_session_id(stable_session) == stable_id);
    TEST_CHECK(Client_resume_step(resume) == CLIENT_RESUME_ACCEPTED);

    TEST_CHECK(Record_session_send(
                   stable_session, continued, sizeof(continued) - 1,
                   RECORD_DELIVERY_REQUIRED) == RECORD_SEND_ACCEPTED);
    TEST_CHECK(Record_session_receive(
                   client_session, received, sizeof(received),
                   &received_length) == RECORD_RECEIVE_READY);
    TEST_CHECK(received_length == sizeof(continued) - 1);
    TEST_CHECK(memcmp(received, continued, received_length) == 0);

    Session_acceptor_destroy(acceptor);
    Client_resume_destroy(resume);
    Record_session_destroy(incoming_session);
    Record_session_destroy(stable_session);
    Record_session_destroy(client_session);
    Record_transport_destroy(abandoned_client_transport);
    return 0;
}

static int test_game_admission_survives_transport_replacement(void)
{
    static const char occupied[] = "occupied";
    static const char confirmation[] = "confirmation";
    char received[SESSION_PROTOCOL_MAX_RECORD_SIZE];
    session_game_request_t request = make_game_request();
    session_open_t open;
    record_session_id_t client_id;
    record_session_t *client_record;
    record_session_t *server_record;
    record_transport_t *first_client_transport;
    record_transport_t *first_server_transport;
    record_transport_t *second_client_transport;
    record_transport_t *second_server_transport;
    client_session_t *client;
    session_acceptor_t *acceptor;
    size_t received_length;

    TEST_CHECK(Record_transport_create_memory_pair(
                   1, SESSION_PROTOCOL_MAX_RECORD_SIZE,
                   &first_client_transport, &first_server_transport) == 0);
    client_record = Record_session_create(first_client_transport);
    TEST_CHECK(client_record != NULL);
    client_id = Record_session_id(client_record);
    TEST_CHECK(Record_session_send(
                   client_record, occupied, sizeof(occupied) - 1,
                   RECORD_DELIVERY_REQUIRED) == RECORD_SEND_ACCEPTED);
    client = Client_session_create(client_record, &request);
    TEST_CHECK(client != NULL);
    TEST_CHECK(Client_session_step(
                   client, received, sizeof(received),
                   &received_length) == CLIENT_SESSION_PENDING);

    TEST_CHECK(Record_transport_create_memory_pair(
                   2, SESSION_PROTOCOL_MAX_RECORD_SIZE * 2,
                   &second_client_transport, &second_server_transport) == 0);
    TEST_CHECK(Record_session_replace_transport(
                   client_record, second_client_transport) == 0);
    TEST_CHECK(Record_session_id(client_record) == client_id);
    TEST_CHECK(Record_transport_receive(
                   first_server_transport, received, sizeof(received),
                   &received_length) == RECORD_RECEIVE_READY);
    TEST_CHECK(Record_transport_receive(
                   first_server_transport, received, sizeof(received),
                   &received_length) == RECORD_RECEIVE_CLOSED);

    server_record = Record_session_create(second_server_transport);
    TEST_CHECK(server_record != NULL);
    acceptor = Session_acceptor_create(server_record);
    TEST_CHECK(acceptor != NULL);
    TEST_CHECK(Client_session_step(
                   client, received, sizeof(received),
                   &received_length) == CLIENT_SESSION_PENDING);
    TEST_CHECK(Session_acceptor_step(
                   acceptor, &open) == SESSION_ACCEPTOR_GAME_READY);
    TEST_CHECK(strcmp(open.request.game.user, request.user) == 0);
    TEST_CHECK(strcmp(open.request.game.nick, request.nick) == 0);
    server_record = Session_acceptor_take_session(acceptor);
    TEST_CHECK(server_record != NULL);

    TEST_CHECK(send_game_reply(
                   server_record, SUCCESS,
                   GAME_PROTOCOL_TCP_POLYGON_VERSION, "accepted") == 0);
    TEST_CHECK(Client_session_step(
                   client, received, sizeof(received),
                   &received_length) == CLIENT_SESSION_PENDING);
    TEST_CHECK(Client_session_selected_version(client)
               == GAME_PROTOCOL_TCP_POLYGON_VERSION);
    TEST_CHECK(Record_session_send(
                   server_record, confirmation, sizeof(confirmation) - 1,
                   RECORD_DELIVERY_REQUIRED) == RECORD_SEND_ACCEPTED);
    TEST_CHECK(Client_session_step(
                   client, received, sizeof(received),
                   &received_length) == CLIENT_SESSION_CONFIRMATION_READY);
    TEST_CHECK(received_length == sizeof(confirmation) - 1);
    TEST_CHECK(memcmp(received, confirmation, received_length) == 0);

    Session_acceptor_destroy(acceptor);
    Client_session_destroy(client);
    Record_session_destroy(server_record);
    Record_session_destroy(client_record);
    Record_transport_destroy(first_server_transport);
    return 0;
}

static int test_control_session_returns_one_reply_per_step(void)
{
    session_control_request_t request = make_control_request();
    session_control_reply_t reply;
    session_open_t open;
    record_session_t *client_record;
    record_session_t *server_record;
    record_transport_t *client_transport;
    record_transport_t *server_transport;
    control_session_t *client;
    session_acceptor_t *acceptor;

    TEST_CHECK(Record_transport_create_memory_pair(
                   4, SESSION_PROTOCOL_MAX_RECORD_SIZE * 4,
                   &client_transport, &server_transport) == 0);
    client_record = Record_session_create(client_transport);
    server_record = Record_session_create(server_transport);
    TEST_CHECK(client_record != NULL);
    TEST_CHECK(server_record != NULL);
    client = Control_session_create(client_record, &request);
    acceptor = Session_acceptor_create(server_record);
    TEST_CHECK(client != NULL);
    TEST_CHECK(acceptor != NULL);

    TEST_CHECK(Control_session_step(
                   client, &reply) == CONTROL_SESSION_PENDING);
    TEST_CHECK(Session_acceptor_step(
                   acceptor, &open) == SESSION_ACCEPTOR_CONTROL_READY);
    TEST_CHECK(strcmp(open.request.control.user, request.user) == 0);
    TEST_CHECK(open.request.control.command == request.command);
    TEST_CHECK(strcmp(open.request.control.argument, request.argument) == 0);
    server_record = Session_acceptor_take_session(acceptor);
    TEST_CHECK(server_record != NULL);

    TEST_CHECK(send_control_reply(
                   server_record, request.command, SUCCESS, 1,
                   "first reply\n") == 0);
    TEST_CHECK(Control_session_step(
                   client, &reply) == CONTROL_SESSION_REPLY_READY);
    TEST_CHECK(reply.more == 1);
    TEST_CHECK(strcmp(reply.payload, "first reply\n") == 0);
    TEST_CHECK(send_control_reply(
                   server_record, request.command, SUCCESS, 0,
                   "last reply") == 0);
    TEST_CHECK(Control_session_step(
                   client, &reply) == CONTROL_SESSION_FINISHED);
    TEST_CHECK(reply.more == 0);
    TEST_CHECK(strcmp(reply.payload, "last reply") == 0);

    Session_acceptor_destroy(acceptor);
    Control_session_destroy(client);
    Record_session_destroy(server_record);
    Record_session_destroy(client_record);
    return 0;
}

static int open_connected_sockets(sock_t *client, sock_t *server)
{
    sock_t listener;
    int port;

    TEST_CHECK(sock_open_tcp_listener(&listener, "127.0.0.1", 0, 1) == 0);
    port = sock_get_port(&listener);
    TEST_CHECK(port > 0);
    TEST_CHECK(sock_open_tcp(client) == 0);
    TEST_CHECK(sock_connect(client, "127.0.0.1", port) == 0);
    TEST_CHECK(sock_accept(&listener, server) == 0);
    TEST_CHECK(sock_close(&listener) == 0);
    TEST_CHECK(sock_set_tcp_nodelay(client, 1) == 0);
    TEST_CHECK(sock_set_tcp_nodelay(server, 1) == 0);
    TEST_CHECK(sock_set_non_blocking(client, 1) == 0);
    TEST_CHECK(sock_set_non_blocking(server, 1) == 0);
    return 0;
}

static int test_game_admission_uses_the_same_state_machine_over_tcp(void)
{
    static const char confirmation[] = "tcp-confirmation";
    char received[SESSION_PROTOCOL_MAX_RECORD_SIZE];
    session_game_request_t request = make_game_request();
    session_open_t open;
    record_session_t *client_record;
    record_session_t *server_record;
    record_transport_t *client_transport;
    record_transport_t *server_transport;
    client_session_t *client;
    session_acceptor_t *acceptor;
    client_session_result_t client_result = CLIENT_SESSION_PENDING;
    session_acceptor_result_t accept_result = SESSION_ACCEPTOR_PENDING;
    sock_t client_socket;
    sock_t server_socket;
    size_t received_length;
    int attempts;

    TEST_CHECK(open_connected_sockets(&client_socket, &server_socket) == 0);
    client_transport = Record_transport_create_tcp(
        &client_socket, SESSION_PROTOCOL_MAX_RECORD_SIZE,
        SESSION_PROTOCOL_MAX_RECORD_SIZE);
    server_transport = Record_transport_create_tcp(
        &server_socket, SESSION_PROTOCOL_MAX_RECORD_SIZE,
        SESSION_PROTOCOL_MAX_RECORD_SIZE);
    TEST_CHECK(client_transport != NULL);
    TEST_CHECK(server_transport != NULL);
    client_record = Record_session_create(client_transport);
    server_record = Record_session_create(server_transport);
    TEST_CHECK(client_record != NULL);
    TEST_CHECK(server_record != NULL);
    client = Client_session_create(client_record, &request);
    acceptor = Session_acceptor_create(server_record);
    TEST_CHECK(client != NULL);
    TEST_CHECK(acceptor != NULL);

    for (attempts = 0; attempts < 100
         && accept_result == SESSION_ACCEPTOR_PENDING; attempts++) {
        client_result = Client_session_step(
            client, received, sizeof(received), &received_length);
        TEST_CHECK(client_result == CLIENT_SESSION_PENDING);
        accept_result = Session_acceptor_step(acceptor, &open);
    }
    TEST_CHECK(accept_result == SESSION_ACCEPTOR_GAME_READY);
    TEST_CHECK(strcmp(open.request.game.host, request.host) == 0);
    server_record = Session_acceptor_take_session(acceptor);
    TEST_CHECK(server_record != NULL);
    TEST_CHECK(send_game_reply(
                   server_record, SUCCESS,
                   GAME_PROTOCOL_TCP_POLYGON_VERSION, "accepted") == 0);

    for (attempts = 0; attempts < 100
         && Client_session_selected_version(client) == 0; attempts++) {
        client_result = Client_session_step(
            client, received, sizeof(received), &received_length);
        TEST_CHECK(client_result == CLIENT_SESSION_PENDING);
    }
    TEST_CHECK(Client_session_selected_version(client)
               == GAME_PROTOCOL_TCP_POLYGON_VERSION);
    TEST_CHECK(Record_session_send(
                   server_record, confirmation, sizeof(confirmation) - 1,
                   RECORD_DELIVERY_REQUIRED) == RECORD_SEND_ACCEPTED);
    for (attempts = 0; attempts < 100
         && client_result == CLIENT_SESSION_PENDING; attempts++) {
        client_result = Client_session_step(
            client, received, sizeof(received), &received_length);
    }
    TEST_CHECK(client_result == CLIENT_SESSION_CONFIRMATION_READY);
    TEST_CHECK(received_length == sizeof(confirmation) - 1);
    TEST_CHECK(memcmp(received, confirmation, received_length) == 0);

    Session_acceptor_destroy(acceptor);
    Client_session_destroy(client);
    Record_session_destroy(server_record);
    Record_session_destroy(client_record);
    return 0;
}

int main(void)
{
    TEST_CHECK(sock_startup() == 0);
    TEST_CHECK(test_protocol_round_trip_and_exact_record_validation() == 0);
    TEST_CHECK(test_resume_protocol_round_trip_and_exact_validation() == 0);
    TEST_CHECK(test_resume_exchange_transfers_transport_to_stable_session()
               == 0);
    TEST_CHECK(test_game_admission_survives_transport_replacement() == 0);
    TEST_CHECK(test_control_session_returns_one_reply_per_step() == 0);
    TEST_CHECK(test_game_admission_uses_the_same_state_machine_over_tcp() == 0);
    sock_cleanup();
    return 0;
}
