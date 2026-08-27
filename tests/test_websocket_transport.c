#include "test_helpers.h"

#include <stdio.h>
#include <string.h>

#include "xpcommon.h"
#include "record_transport.h"
#include "websocket_http.h"
#include "websocket_transport.h"

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

static int write_all(sock_t *socket, const unsigned char *bytes,
                     size_t length)
{
    size_t offset = 0;

    while (offset < length) {
        int written = sock_write(socket, (char *)bytes + offset,
                                 (int)(length - offset));

        TEST_CHECK(written > 0);
        offset += (size_t)written;
    }
    return 0;
}

static int read_available(sock_t *socket, unsigned char *bytes,
                          size_t capacity, size_t *length)
{
    int received;

    *length = 0;
    while (*length < capacity) {
        received = sock_read(socket, (char *)bytes + *length,
                             (int)(capacity - *length));
        if (received < 0) {
            if (sock_error_is_temporary(socket))
                break;
            return -1;
        }
        if (received == 0)
            break;
        *length += (size_t)received;
    }
    return 0;
}

static int find_header_value(const char *headers, const char *name,
                             char *value, size_t capacity);

static int drive_server_handshake(record_transport_t *server,
                                  sock_t *raw_client,
                                  const char *request,
                                  char *response, size_t response_capacity)
{
    char ignored[64];
    size_t ignored_length;
    size_t response_length = 0;
    int step;

    TEST_CHECK(write_all(raw_client, (const unsigned char *)request,
                         strlen(request)) == 0);
    for (step = 0; step < 100 && response_length == 0; step++) {
        record_receive_result_t receive_result = Record_transport_receive(
            server, ignored, sizeof(ignored), &ignored_length);
        record_flush_result_t flush_result = Record_transport_flush(server);

        TEST_CHECK(receive_result == RECORD_RECEIVE_EMPTY
                   || receive_result == RECORD_RECEIVE_CLOSED);
        TEST_CHECK(flush_result == RECORD_FLUSH_IDLE
                   || flush_result == RECORD_FLUSH_PENDING
                   || flush_result == RECORD_FLUSH_CLOSED);
        TEST_CHECK(read_available(raw_client, (unsigned char *)response,
                                  response_capacity - 1,
                                  &response_length) == 0);
    }
    TEST_CHECK(response_length > 0);
    response[response_length] = '\0';
    return 0;
}

static int write_masked_frame(sock_t *socket, bool final,
                              unsigned char opcode,
                              const unsigned char *payload, size_t length)
{
    static const unsigned char mask[4] = { 0x12, 0x34, 0x56, 0x78 };
    unsigned char frame[2 + sizeof(mask) + 125];
    size_t index;

    TEST_CHECK(length <= 125);
    frame[0] = (unsigned char)((final ? 0x80 : 0) | opcode);
    frame[1] = (unsigned char)(0x80 | length);
    memcpy(frame + 2, mask, sizeof(mask));
    for (index = 0; index < length; index++)
        frame[6 + index] = payload[index] ^ mask[index % sizeof(mask)];
    return write_all(socket, frame, 6 + length);
}

static int check_rfc_accept_value(void)
{
    char accept[WEBSOCKET_HTTP_ACCEPT_CAPACITY];

    TEST_CHECK(WebSocket_http_create_accept(
                   "dGhlIHNhbXBsZSBub25jZQ==", accept, sizeof(accept)));
    TEST_CHECK(strcmp(accept, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") == 0);
    TEST_CHECK(!WebSocket_http_create_accept("not-base64", accept,
                                             sizeof(accept)));
    TEST_CHECK(!WebSocket_http_create_accept(
                   "dGhlIHNhbXBsZSBub25jZQ==", accept,
                   WEBSOCKET_HTTP_ACCEPT_CAPACITY - 1));
    return 0;
}

static int check_subprotocol_is_case_sensitive(void)
{
    static const char request[] =
        "GET /xpilot HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Protocol: XPILOT-INFINITY-V1\r\n\r\n";
    static const char response[] =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n"
        "Sec-WebSocket-Protocol: XPILOT-INFINITY-V1\r\n\r\n";
    websocket_http_server_request_t parsed_request;
    websocket_http_client_response_t parsed_response;

    TEST_CHECK(WebSocket_http_parse_server_request(
                   request, sizeof(request) - 1, &parsed_request)
               == WEBSOCKET_HTTP_BAD_REQUEST);
    TEST_CHECK(WebSocket_http_parse_client_response(
                   response, sizeof(response) - 1,
                   "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", &parsed_response)
               == WEBSOCKET_HTTP_BAD_REQUEST);
    return 0;
}

static int check_native_client_and_server_exchange_records(void)
{
    static const char client_payload[] = "from-client";
    static const char server_payload[] = "from-server";
    sock_t client_socket;
    sock_t server_socket;
    record_transport_t *client;
    record_transport_t *server;
    char received[64];
    size_t received_length = 0;
    bool client_sent = false;
    bool server_received = false;
    bool server_sent = false;
    bool client_received = false;
    int step;

    TEST_CHECK(open_connected_sockets(&client_socket, &server_socket) == 0);
    client = Record_transport_create_websocket_client(
        &client_socket, "127.0.0.1", 15345, sizeof(received), 64);
    server = Record_transport_create_websocket_server(
        &server_socket, sizeof(received), 64);
    TEST_CHECK(client != NULL);
    TEST_CHECK(server != NULL);

    for (step = 0; step < 1000 && !client_received; step++) {
        record_send_result_t send_result;
        record_receive_result_t receive_result;

        TEST_CHECK(Record_transport_flush(client) >= RECORD_FLUSH_IDLE);
        TEST_CHECK(Record_transport_flush(server) >= RECORD_FLUSH_IDLE);

        if (!client_sent) {
            send_result = Record_transport_send(
                client, client_payload, sizeof(client_payload) - 1,
                RECORD_DELIVERY_REQUIRED);
            TEST_CHECK(send_result == RECORD_SEND_BACKPRESSURED
                       || send_result == RECORD_SEND_ACCEPTED);
            client_sent = send_result == RECORD_SEND_ACCEPTED;
        }
        receive_result = Record_transport_receive(
            server, received, sizeof(received), &received_length);
        TEST_CHECK(receive_result == RECORD_RECEIVE_EMPTY
                   || receive_result == RECORD_RECEIVE_READY);
        if (receive_result == RECORD_RECEIVE_READY) {
            TEST_CHECK(!server_received);
            TEST_CHECK(received_length == sizeof(client_payload) - 1);
            TEST_CHECK(memcmp(received, client_payload,
                              received_length) == 0);
            server_received = true;
        }

        if (server_received && !server_sent) {
            send_result = Record_transport_send(
                server, server_payload, sizeof(server_payload) - 1,
                RECORD_DELIVERY_REQUIRED);
            TEST_CHECK(send_result == RECORD_SEND_BACKPRESSURED
                       || send_result == RECORD_SEND_ACCEPTED);
            server_sent = send_result == RECORD_SEND_ACCEPTED;
        }
        if (server_sent) {
            receive_result = Record_transport_receive(
                client, received, sizeof(received), &received_length);
            TEST_CHECK(receive_result == RECORD_RECEIVE_EMPTY
                       || receive_result == RECORD_RECEIVE_READY);
            if (receive_result == RECORD_RECEIVE_READY) {
                TEST_CHECK(received_length == sizeof(server_payload) - 1);
                TEST_CHECK(memcmp(received, server_payload,
                                  received_length) == 0);
                client_received = true;
            }
        }
    }

    TEST_CHECK(client_sent);
    TEST_CHECK(server_received);
    TEST_CHECK(server_sent);
    TEST_CHECK(client_received);
    Record_transport_destroy(client);
    Record_transport_destroy(server);
    return 0;
}

static int check_native_client_receives_buffered_record_burst(void)
{
    enum { RECORD_COUNT = 16 };
    sock_t client_socket;
    sock_t raw_server;
    record_transport_t *client;
    unsigned char request[2048];
    unsigned char server_bytes[1024];
    char received[64];
    char key[64];
    char accept[WEBSOCKET_HTTP_ACCEPT_CAPACITY];
    char response[512];
    size_t request_length = 0;
    size_t server_length;
    size_t received_length = 0;
    int response_length;
    int received_count = 0;
    int step;

    TEST_CHECK(open_connected_sockets(&client_socket, &raw_server) == 0);
    client = Record_transport_create_websocket_client(
        &client_socket, "127.0.0.1", 15345, sizeof(received), 64);
    TEST_CHECK(client != NULL);

    for (step = 0; step < 100 && request_length == 0; step++) {
        TEST_CHECK(Record_transport_flush(client) >= RECORD_FLUSH_IDLE);
        TEST_CHECK(read_available(&raw_server, request,
                                  sizeof(request) - 1,
                                  &request_length) == 0);
    }
    TEST_CHECK(request_length > 0);
    request[request_length] = '\0';
    TEST_CHECK(find_header_value(
                   (char *)request, "Sec-WebSocket-Key: ",
                   key, sizeof(key)) == 0);
    TEST_CHECK(WebSocket_http_create_accept(key, accept, sizeof(accept)));
    response_length = snprintf(
        response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "Sec-WebSocket-Protocol: xpilot-infinity-v1\r\n"
        "\r\n", accept);
    TEST_CHECK(response_length > 0
               && (size_t)response_length < sizeof(response));
    memcpy(server_bytes, response, (size_t)response_length);
    server_length = (size_t)response_length;
    for (step = 0; step < RECORD_COUNT; step++) {
        server_bytes[server_length++] = 0x82;
        server_bytes[server_length++] = 1;
        server_bytes[server_length++] = (unsigned char)(step + 1);
    }
    TEST_CHECK(write_all(&raw_server, server_bytes, server_length) == 0);

    for (step = 0; step < 1000 && received_count < RECORD_COUNT; step++) {
        record_receive_result_t receive_result = Record_transport_receive(
            client, received, sizeof(received), &received_length);

        TEST_CHECK(receive_result == RECORD_RECEIVE_EMPTY
                   || receive_result == RECORD_RECEIVE_READY);
        if (receive_result == RECORD_RECEIVE_READY) {
            TEST_CHECK(received_length == 1);
            TEST_CHECK((unsigned char)received[0]
                       == (unsigned char)(received_count + 1));
            received_count++;
        }
    }
    TEST_CHECK(received_count == RECORD_COUNT);

    Record_transport_destroy(client);
    sock_close(&raw_server);
    return 0;
}

static int check_server_accepts_browser_style_frames(void)
{
    static const char request[] =
        "GET /xpilot HTTP/1.1\r\n"
        "Host: localhost:15345\r\n"
        "uPgRaDe: WebSocket\r\n"
        "Connection: keep-alive, Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Protocol: xpilot-infinity-v1\r\n"
        "\r\n";
    sock_t raw_client;
    sock_t server_socket;
    record_transport_t *server;
    char response[1024];
    char received[64];
    unsigned char pong[32];
    size_t received_length;
    size_t pong_length = 0;
    record_receive_result_t receive_result;
    int step;

    TEST_CHECK(open_connected_sockets(&raw_client, &server_socket) == 0);
    server = Record_transport_create_websocket_server(
        &server_socket, sizeof(received), sizeof(received));
    TEST_CHECK(server != NULL);
    TEST_CHECK(drive_server_handshake(
                   server, &raw_client, request,
                   response, sizeof(response)) == 0);
    TEST_CHECK(strstr(response, "HTTP/1.1 101 Switching Protocols\r\n")
               != NULL);
    TEST_CHECK(strstr(response,
                      "Sec-WebSocket-Accept: "
                      "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n") != NULL);
    TEST_CHECK(strstr(response,
                      "Sec-WebSocket-Protocol: xpilot-infinity-v1\r\n")
               != NULL);

    TEST_CHECK(write_masked_frame(
                   &raw_client, false, 0x2,
                   (const unsigned char *)"frag", 4) == 0);
    TEST_CHECK(write_masked_frame(
                   &raw_client, true, 0x0,
                   (const unsigned char *)"mented", 6) == 0);
    receive_result = RECORD_RECEIVE_EMPTY;
    for (step = 0; step < 100 && receive_result == RECORD_RECEIVE_EMPTY;
         step++) {
        receive_result = Record_transport_receive(
            server, received, sizeof(received), &received_length);
    }
    TEST_CHECK(receive_result == RECORD_RECEIVE_READY);
    TEST_CHECK(received_length == 10);
    TEST_CHECK(memcmp(received, "fragmented", 10) == 0);

    TEST_CHECK(write_masked_frame(
                   &raw_client, true, 0x9,
                   (const unsigned char *)"ok", 2) == 0);
    TEST_CHECK(Record_transport_receive(
                   server, received, sizeof(received),
                   &received_length) == RECORD_RECEIVE_EMPTY);
    TEST_CHECK(Record_transport_flush(server) == RECORD_FLUSH_IDLE);
    for (step = 0; step < 100 && pong_length == 0; step++)
        TEST_CHECK(read_available(&raw_client, pong, sizeof(pong),
                                  &pong_length) == 0);
    TEST_CHECK(pong_length == 4);
    TEST_CHECK(pong[0] == 0x8a);
    TEST_CHECK(pong[1] == 2);
    TEST_CHECK(memcmp(pong + 2, "ok", 2) == 0);

    Record_transport_destroy(server);
    sock_close(&raw_client);
    return 0;
}

static int check_server_rejects_unmasked_client_frame(void)
{
    static const char request[] =
        "GET /xpilot HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Protocol: xpilot-infinity-v1\r\n\r\n";
    static const unsigned char unmasked_frame[] = { 0x82, 0x01, 0x2a };
    sock_t raw_client;
    sock_t server_socket;
    record_transport_t *server;
    char response[1024];
    char received[64];
    unsigned char close_frame[32];
    size_t received_length;
    size_t close_length = 0;
    int step;

    TEST_CHECK(open_connected_sockets(&raw_client, &server_socket) == 0);
    server = Record_transport_create_websocket_server(
        &server_socket, sizeof(received), sizeof(received));
    TEST_CHECK(server != NULL);
    TEST_CHECK(drive_server_handshake(
                   server, &raw_client, request,
                   response, sizeof(response)) == 0);
    TEST_CHECK(write_all(&raw_client, unmasked_frame,
                         sizeof(unmasked_frame)) == 0);

    for (step = 0; step < 100 && close_length == 0; step++) {
        record_receive_result_t receive_result = Record_transport_receive(
            server, received, sizeof(received), &received_length);
        record_flush_result_t flush_result = Record_transport_flush(server);

        TEST_CHECK(receive_result == RECORD_RECEIVE_EMPTY
                   || receive_result == RECORD_RECEIVE_CLOSED);
        TEST_CHECK(flush_result == RECORD_FLUSH_IDLE
                   || flush_result == RECORD_FLUSH_CLOSED);
        TEST_CHECK(read_available(&raw_client, close_frame,
                                  sizeof(close_frame), &close_length) == 0);
    }
    TEST_CHECK(close_length == 4);
    TEST_CHECK(close_frame[0] == 0x88);
    TEST_CHECK(close_frame[1] == 2);
    TEST_CHECK(close_frame[2] == 0x03);
    TEST_CHECK(close_frame[3] == 0xea);

    Record_transport_destroy(server);
    sock_close(&raw_client);
    return 0;
}

static int find_header_value(const char *headers, const char *name,
                             char *value, size_t capacity)
{
    const char *start = strstr(headers, name);
    const char *end;
    size_t length;

    TEST_CHECK(start != NULL);
    start += strlen(name);
    end = strstr(start, "\r\n");
    TEST_CHECK(end != NULL);
    length = (size_t)(end - start);
    TEST_CHECK(length < capacity);
    memcpy(value, start, length);
    value[length] = '\0';
    return 0;
}

static int check_native_client_masks_frames(void)
{
    sock_t client_socket;
    sock_t raw_server;
    record_transport_t *client;
    unsigned char request[2048];
    unsigned char frame[64];
    size_t request_length = 0;
    size_t frame_length = 0;
    char key[64];
    char accept[WEBSOCKET_HTTP_ACCEPT_CAPACITY];
    char response[512];
    char ignored[64];
    size_t ignored_length;
    record_send_result_t send_result = RECORD_SEND_BACKPRESSURED;
    int response_length;
    int step;
    size_t index;

    TEST_CHECK(open_connected_sockets(&client_socket, &raw_server) == 0);
    client = Record_transport_create_websocket_client(
        &client_socket, "localhost", 15345, sizeof(ignored),
        sizeof(ignored));
    TEST_CHECK(client != NULL);
    for (step = 0; step < 100 && request_length == 0; step++) {
        TEST_CHECK(Record_transport_flush(client) >= RECORD_FLUSH_IDLE);
        TEST_CHECK(read_available(&raw_server, request,
                                  sizeof(request) - 1,
                                  &request_length) == 0);
    }
    TEST_CHECK(request_length > 0);
    request[request_length] = '\0';
    TEST_CHECK(strstr((char *)request, "GET /xpilot HTTP/1.1\r\n")
               != NULL);
    TEST_CHECK(find_header_value(
                   (char *)request, "Sec-WebSocket-Key: ",
                   key, sizeof(key)) == 0);
    TEST_CHECK(WebSocket_http_create_accept(key, accept, sizeof(accept)));
    response_length = snprintf(
        response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "Sec-WebSocket-Protocol: xpilot-infinity-v1\r\n"
        "\r\n", accept);
    TEST_CHECK(response_length > 0
               && (size_t)response_length < sizeof(response));
    TEST_CHECK(write_all(&raw_server, (unsigned char *)response,
                         (size_t)response_length) == 0);

    for (step = 0; step < 100 && send_result != RECORD_SEND_ACCEPTED;
         step++) {
        record_receive_result_t receive_result = Record_transport_receive(
            client, ignored, sizeof(ignored), &ignored_length);

        TEST_CHECK(receive_result == RECORD_RECEIVE_EMPTY);
        send_result = Record_transport_send(
            client, "masked", 6, RECORD_DELIVERY_REQUIRED);
        TEST_CHECK(send_result == RECORD_SEND_BACKPRESSURED
                   || send_result == RECORD_SEND_ACCEPTED);
        TEST_CHECK(Record_transport_flush(client) >= RECORD_FLUSH_IDLE);
    }
    TEST_CHECK(send_result == RECORD_SEND_ACCEPTED);
    for (step = 0; step < 100 && frame_length == 0; step++) {
        TEST_CHECK(Record_transport_flush(client) >= RECORD_FLUSH_IDLE);
        TEST_CHECK(read_available(&raw_server, frame, sizeof(frame),
                                  &frame_length) == 0);
    }
    TEST_CHECK(frame_length == 12);
    TEST_CHECK(frame[0] == 0x82);
    TEST_CHECK((frame[1] & 0x80) != 0);
    TEST_CHECK((frame[1] & 0x7f) == 6);
    for (index = 0; index < 6; index++)
        TEST_CHECK((frame[6 + index] ^ frame[2 + index % 4])
                   == (unsigned char)"masked"[index]);

    Record_transport_destroy(client);
    sock_close(&raw_server);
    return 0;
}

static int check_text_and_close_are_handled(void)
{
    static const char request[] =
        "GET /xpilot HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Protocol: xpilot-infinity-v1\r\n\r\n";
    sock_t raw_client;
    sock_t server_socket;
    record_transport_t *server;
    char response[1024];
    char received[64];
    unsigned char close_frame[32];
    size_t received_length;
    size_t close_length = 0;
    int step;

    TEST_CHECK(open_connected_sockets(&raw_client, &server_socket) == 0);
    server = Record_transport_create_websocket_server(
        &server_socket, sizeof(received), sizeof(received));
    TEST_CHECK(server != NULL);
    TEST_CHECK(drive_server_handshake(
                   server, &raw_client, request,
                   response, sizeof(response)) == 0);
    TEST_CHECK(write_masked_frame(
                   &raw_client, true, 0x1,
                   (const unsigned char *)"text", 4) == 0);
    {
        record_receive_result_t receive_result = Record_transport_receive(
            server, received, sizeof(received), &received_length);

        TEST_CHECK(receive_result == RECORD_RECEIVE_EMPTY
                   || receive_result == RECORD_RECEIVE_CLOSED);
    }
    for (step = 0; step < 100 && close_length == 0; step++) {
        record_flush_result_t flush_result = Record_transport_flush(server);

        TEST_CHECK(flush_result == RECORD_FLUSH_IDLE
                   || flush_result == RECORD_FLUSH_CLOSED);
        TEST_CHECK(read_available(&raw_client, close_frame,
                                  sizeof(close_frame), &close_length) == 0);
    }
    TEST_CHECK(close_length >= 4);
    TEST_CHECK(close_frame[0] == 0x88);
    TEST_CHECK((close_frame[1] & 0x80) == 0);
    TEST_CHECK(close_frame[2] == 0x03);
    TEST_CHECK(close_frame[3] == 0xeb);

    Record_transport_destroy(server);
    sock_close(&raw_client);
    return 0;
}

static int check_wrong_resource_is_rejected(void)
{
    static const char request[] =
        "GET /wrong HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Protocol: xpilot-infinity-v1\r\n\r\n";
    sock_t raw_client;
    sock_t server_socket;
    record_transport_t *server;
    char response[1024];

    TEST_CHECK(open_connected_sockets(&raw_client, &server_socket) == 0);
    server = Record_transport_create_websocket_server(
        &server_socket, 64, 64);
    TEST_CHECK(server != NULL);
    TEST_CHECK(drive_server_handshake(
                   server, &raw_client, request,
                   response, sizeof(response)) == 0);
    TEST_CHECK(strstr(response, "HTTP/1.1 404 Not Found\r\n") != NULL);

    Record_transport_destroy(server);
    sock_close(&raw_client);
    return 0;
}

int main(void)
{
    TEST_CHECK(sock_startup() == 0);
    TEST_CHECK(check_rfc_accept_value() == 0);
    TEST_CHECK(check_subprotocol_is_case_sensitive() == 0);
    TEST_CHECK(check_native_client_and_server_exchange_records() == 0);
    TEST_CHECK(check_native_client_receives_buffered_record_burst() == 0);
    TEST_CHECK(check_server_accepts_browser_style_frames() == 0);
    TEST_CHECK(check_server_rejects_unmasked_client_frame() == 0);
    TEST_CHECK(check_native_client_masks_frames() == 0);
    TEST_CHECK(check_text_and_close_are_handled() == 0);
    TEST_CHECK(check_wrong_resource_is_rejected() == 0);
    sock_cleanup();
    return 0;
}
