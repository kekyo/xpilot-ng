/*
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2026 XPilot NG contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "xpcommon.h"

#include <wslay/wslay.h>

#include "secure_random.h"
#include "websocket_http.h"
#include "websocket_transport.h"

#define WEBSOCKET_HANDSHAKE_OUTPUT_CAPACITY 1024

typedef enum {
    WEBSOCKET_STATE_CLIENT_REQUEST,
    WEBSOCKET_STATE_SERVER_REQUEST,
    WEBSOCKET_STATE_CLIENT_RESPONSE,
    WEBSOCKET_STATE_SERVER_RESPONSE,
    WEBSOCKET_STATE_OPEN,
    WEBSOCKET_STATE_CLOSED,
    WEBSOCKET_STATE_FAILED
} websocket_state_t;

typedef struct websocket_record {
    struct websocket_record *next;
    size_t length;
    char payload[];
} websocket_record_t;

typedef struct {
    sock_t socket;
    websocket_state_t state;
    bool server;
    bool close_after_handshake;
    bool peer_eof;
    size_t receive_capacity;
    size_t send_capacity;
    char handshake_input[WEBSOCKET_HTTP_HEADER_CAPACITY];
    size_t handshake_input_length;
    size_t prefetched_offset;
    char handshake_output[WEBSOCKET_HANDSHAKE_OUTPUT_CAPACITY];
    size_t handshake_output_length;
    size_t handshake_output_offset;
    char expected_accept[WEBSOCKET_HTTP_ACCEPT_CAPACITY];
    wslay_event_context_ptr event;
    websocket_record_t *record_head;
    websocket_record_t *record_tail;
} websocket_transport_context_t;

static int WebSocket_socket_write(websocket_transport_context_t *context,
                                  const unsigned char *data, size_t length)
{
#ifdef MSG_NOSIGNAL
    return (int)send(context->socket.fd, (const char *)data, (int)length,
                     MSG_NOSIGNAL);
#else
    return sock_write(&context->socket, (char *)data, (int)length);
#endif
}

static bool WebSocket_write_error_is_temporary(
    const websocket_transport_context_t *context)
{
#ifdef MSG_NOSIGNAL
    (void)context;
    return errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN
        || errno == EINPROGRESS || errno == EALREADY;
#else
    return sock_error_is_temporary(&context->socket);
#endif
}

static void WebSocket_clear_records(websocket_transport_context_t *context)
{
    websocket_record_t *record = context->record_head;

    while (record != NULL) {
        websocket_record_t *next = record->next;

        free(record);
        record = next;
    }
    context->record_head = NULL;
    context->record_tail = NULL;
}

static void WebSocket_close_socket(websocket_transport_context_t *context)
{
    if (context->socket.fd != SOCK_FD_INVALID)
        sock_close(&context->socket);
}

static void WebSocket_fail(websocket_transport_context_t *context)
{
    context->state = WEBSOCKET_STATE_FAILED;
    WebSocket_close_socket(context);
}

static void WebSocket_finish(websocket_transport_context_t *context)
{
    context->state = WEBSOCKET_STATE_CLOSED;
    WebSocket_close_socket(context);
}

static ssize_t WebSocket_receive_callback(wslay_event_context_ptr event,
                                          uint8_t *buffer, size_t length,
                                          int flags, void *user_data)
{
    websocket_transport_context_t *context = user_data;
    size_t prefetched_length;
    int received;

    (void)flags;
    /* wslay_event_recv() keeps reading until the callback reports that it
     * would block.  Once at least one application record is ready, leave
     * further socket data for the next receive call so a delayed consumer
     * cannot turn a valid burst into an artificial queue overflow. */
    if (context->record_head != NULL) {
        wslay_event_set_error(event, WSLAY_ERR_WOULDBLOCK);
        return -1;
    }
    prefetched_length = context->handshake_input_length
        - context->prefetched_offset;
    if (prefetched_length > 0) {
        size_t copied = prefetched_length < length
            ? prefetched_length : length;

        memcpy(buffer, context->handshake_input
               + context->prefetched_offset, copied);
        context->prefetched_offset += copied;
        return (ssize_t)copied;
    }
    received = sock_read(&context->socket, (char *)buffer, (int)length);
    if (received > 0)
        return received;
    if (received == 0) {
        context->peer_eof = true;
        wslay_event_set_error(event, WSLAY_ERR_WOULDBLOCK);
        return -1;
    }
    wslay_event_set_error(
        event, sock_error_is_temporary(&context->socket)
            ? WSLAY_ERR_WOULDBLOCK : WSLAY_ERR_CALLBACK_FAILURE);
    return -1;
}

static ssize_t WebSocket_send_callback(wslay_event_context_ptr event,
                                       const uint8_t *data, size_t length,
                                       int flags, void *user_data)
{
    websocket_transport_context_t *context = user_data;
    int written;

    (void)flags;
    written = WebSocket_socket_write(context, data, length);
    if (written > 0)
        return written;
    wslay_event_set_error(
        event, written < 0 && WebSocket_write_error_is_temporary(context)
            ? WSLAY_ERR_WOULDBLOCK : WSLAY_ERR_CALLBACK_FAILURE);
    return -1;
}

static int WebSocket_genmask_callback(wslay_event_context_ptr event,
                                      uint8_t *buffer, size_t length,
                                      void *user_data)
{
    (void)event;
    (void)user_data;
    return Secure_random_fill(buffer, length) ? 0 : -1;
}

static void WebSocket_queue_close(websocket_transport_context_t *context,
                                  uint16_t status_code)
{
    int status;

    if (context->event == NULL)
        return;
    status = wslay_event_queue_close(
        context->event, status_code, NULL, 0);
    if (status != 0 && status != WSLAY_ERR_NO_MORE_MSG) {
        WebSocket_fail(context);
        return;
    }
    wslay_event_shutdown_read(context->event);
}

static void WebSocket_message_callback(
    wslay_event_context_ptr event,
    const struct wslay_event_on_msg_recv_arg *message,
    void *user_data)
{
    websocket_transport_context_t *context = user_data;
    websocket_record_t *record;

    (void)event;
    if (message->opcode == WSLAY_TEXT_FRAME) {
        WebSocket_queue_close(context, WSLAY_CODE_UNSUPPORTED_DATA);
        return;
    }
    if (message->opcode != WSLAY_BINARY_FRAME)
        return;
    if (message->msg_length == 0
        || message->msg_length > context->receive_capacity) {
        WebSocket_queue_close(context, WSLAY_CODE_MESSAGE_TOO_BIG);
        return;
    }
    record = malloc(sizeof(*record) + message->msg_length);
    if (record == NULL) {
        WebSocket_fail(context);
        return;
    }
    record->next = NULL;
    record->length = message->msg_length;
    memcpy(record->payload, message->msg, message->msg_length);
    if (context->record_tail != NULL)
        context->record_tail->next = record;
    else
        context->record_head = record;
    context->record_tail = record;
}

static const struct wslay_event_callbacks websocket_callbacks = {
    WebSocket_receive_callback,
    WebSocket_send_callback,
    WebSocket_genmask_callback,
    NULL,
    NULL,
    NULL,
    WebSocket_message_callback
};

static int WebSocket_initialize_event(
    websocket_transport_context_t *context)
{
    int status = context->server
        ? wslay_event_context_server_init(
            &context->event, &websocket_callbacks, context)
        : wslay_event_context_client_init(
            &context->event, &websocket_callbacks, context);

    if (status != 0) {
        errno = ENOMEM;
        return -1;
    }
    wslay_event_config_set_max_recv_msg_length(
        context->event, context->receive_capacity);
    return 0;
}

static record_flush_result_t WebSocket_flush_handshake(
    websocket_transport_context_t *context)
{
    while (context->handshake_output_offset
           < context->handshake_output_length) {
        size_t remaining = context->handshake_output_length
            - context->handshake_output_offset;
        int written = WebSocket_socket_write(
            context,
            (const unsigned char *)context->handshake_output
                + context->handshake_output_offset,
            remaining);

        if (written < 0) {
            if (WebSocket_write_error_is_temporary(context))
                return RECORD_FLUSH_PENDING;
            WebSocket_fail(context);
            return RECORD_FLUSH_ERROR;
        }
        if (written == 0) {
            errno = EIO;
            WebSocket_fail(context);
            return RECORD_FLUSH_ERROR;
        }
        context->handshake_output_offset += (size_t)written;
    }

    context->handshake_output_length = 0;
    context->handshake_output_offset = 0;
    if (context->state == WEBSOCKET_STATE_CLIENT_REQUEST) {
        context->state = WEBSOCKET_STATE_CLIENT_RESPONSE;
        return RECORD_FLUSH_IDLE;
    }
    if (context->state == WEBSOCKET_STATE_SERVER_RESPONSE) {
        if (context->close_after_handshake) {
            WebSocket_finish(context);
            return RECORD_FLUSH_CLOSED;
        }
        context->state = WEBSOCKET_STATE_OPEN;
    }
    return RECORD_FLUSH_IDLE;
}

static record_flush_result_t WebSocket_flush_frames(
    websocket_transport_context_t *context)
{
    if (context->event == NULL)
        return RECORD_FLUSH_ERROR;
    if (wslay_event_want_write(context->event)
        && wslay_event_send(context->event) != 0) {
        WebSocket_fail(context);
        return RECORD_FLUSH_ERROR;
    }
    if (!wslay_event_want_read(context->event)
        && !wslay_event_want_write(context->event)) {
        WebSocket_finish(context);
        return RECORD_FLUSH_CLOSED;
    }
    return wslay_event_want_write(context->event)
        ? RECORD_FLUSH_PENDING : RECORD_FLUSH_IDLE;
}

static record_flush_result_t WebSocket_flush(void *user_data)
{
    websocket_transport_context_t *context = user_data;

    if (context->state == WEBSOCKET_STATE_FAILED)
        return RECORD_FLUSH_ERROR;
    if (context->state == WEBSOCKET_STATE_CLOSED)
        return RECORD_FLUSH_CLOSED;
    if (context->state == WEBSOCKET_STATE_CLIENT_REQUEST
        || context->state == WEBSOCKET_STATE_SERVER_RESPONSE)
        return WebSocket_flush_handshake(context);
    if (context->state == WEBSOCKET_STATE_OPEN)
        return WebSocket_flush_frames(context);
    return RECORD_FLUSH_IDLE;
}

static int WebSocket_queue_server_response(
    websocket_transport_context_t *context,
    websocket_http_status_t status,
    const websocket_http_server_request_t *request)
{
    const char *accept = status == WEBSOCKET_HTTP_SWITCHING_PROTOCOLS
        ? request->accept : NULL;

    if (WebSocket_http_create_server_response(
            context->handshake_output,
            sizeof(context->handshake_output),
            &context->handshake_output_length,
            status, accept) == -1)
        return -1;
    context->handshake_output_offset = 0;
    context->close_after_handshake =
        status != WEBSOCKET_HTTP_SWITCHING_PROTOCOLS;
    if (!context->close_after_handshake
        && WebSocket_initialize_event(context) == -1)
        return -1;
    context->state = WEBSOCKET_STATE_SERVER_RESPONSE;
    return 0;
}

static int WebSocket_process_server_handshake(
    websocket_transport_context_t *context)
{
    websocket_http_server_request_t request;
    websocket_http_status_t status = WebSocket_http_parse_server_request(
        context->handshake_input, context->handshake_input_length, &request);

    if (status == WEBSOCKET_HTTP_INCOMPLETE) {
        if (context->handshake_input_length
            < sizeof(context->handshake_input))
            return 0;
        memset(&request, 0, sizeof(request));
        status = WEBSOCKET_HTTP_BAD_REQUEST;
    }
    context->prefetched_offset = request.consumed;
    return WebSocket_queue_server_response(context, status, &request) == 0
        ? 1 : -1;
}

static int WebSocket_process_client_handshake(
    websocket_transport_context_t *context)
{
    websocket_http_client_response_t response;
    websocket_http_status_t status = WebSocket_http_parse_client_response(
        context->handshake_input, context->handshake_input_length,
        context->expected_accept, &response);

    if (status == WEBSOCKET_HTTP_INCOMPLETE) {
        if (context->handshake_input_length
            < sizeof(context->handshake_input))
            return 0;
        errno = EMSGSIZE;
        return -1;
    }
    if (status != WEBSOCKET_HTTP_SWITCHING_PROTOCOLS) {
        errno = EPROTO;
        return -1;
    }
    context->prefetched_offset = response.consumed;
    if (WebSocket_initialize_event(context) == -1)
        return -1;
    context->state = WEBSOCKET_STATE_OPEN;
    return 1;
}

static int WebSocket_read_handshake(
    websocket_transport_context_t *context)
{
    for (;;) {
        size_t available = sizeof(context->handshake_input)
            - context->handshake_input_length;
        int received;
        int processed;

        if (available == 0)
            received = -1;
        else
            received = sock_read(
                &context->socket,
                context->handshake_input + context->handshake_input_length,
                (int)available);
        if (received > 0)
            context->handshake_input_length += (size_t)received;
        else if (received == 0) {
            WebSocket_finish(context);
            return -1;
        } else if (available > 0
                   && !sock_error_is_temporary(&context->socket)) {
            WebSocket_fail(context);
            return -1;
        }

        processed = context->server
            ? WebSocket_process_server_handshake(context)
            : WebSocket_process_client_handshake(context);
        if (processed < 0) {
            int error_number = errno;

            WebSocket_fail(context);
            errno = error_number;
            return -1;
        }
        if (processed > 0)
            return processed;
        if (received <= 0)
            return 0;
    }
}

static record_receive_result_t WebSocket_pop_record(
    websocket_transport_context_t *context, char *destination,
    size_t capacity, size_t *length)
{
    websocket_record_t *record = context->record_head;

    if (record == NULL)
        return RECORD_RECEIVE_EMPTY;
    if (record->length > capacity) {
        errno = EMSGSIZE;
        WebSocket_fail(context);
        return RECORD_RECEIVE_ERROR;
    }
    memcpy(destination, record->payload, record->length);
    *length = record->length;
    context->record_head = record->next;
    if (context->record_head == NULL)
        context->record_tail = NULL;
    free(record);
    return RECORD_RECEIVE_READY;
}

static record_receive_result_t WebSocket_receive(
    void *user_data, char *destination, size_t capacity, size_t *length)
{
    websocket_transport_context_t *context = user_data;
    record_receive_result_t queued_result;
    record_flush_result_t flush_result;
    int handshake_result;

    queued_result = WebSocket_pop_record(
        context, destination, capacity, length);
    if (queued_result != RECORD_RECEIVE_EMPTY)
        return queued_result;
    if (context->state == WEBSOCKET_STATE_FAILED)
        return RECORD_RECEIVE_ERROR;
    if (context->state == WEBSOCKET_STATE_CLOSED)
        return RECORD_RECEIVE_CLOSED;

    flush_result = WebSocket_flush(context);
    if (flush_result == RECORD_FLUSH_ERROR)
        return RECORD_RECEIVE_ERROR;
    if (context->state == WEBSOCKET_STATE_CLIENT_RESPONSE
        || context->state == WEBSOCKET_STATE_SERVER_REQUEST) {
        handshake_result = WebSocket_read_handshake(context);
        if (handshake_result < 0)
            return context->state == WEBSOCKET_STATE_CLOSED
                ? RECORD_RECEIVE_CLOSED : RECORD_RECEIVE_ERROR;
        if (context->state == WEBSOCKET_STATE_SERVER_RESPONSE) {
            flush_result = WebSocket_flush(context);
            if (flush_result == RECORD_FLUSH_ERROR)
                return RECORD_RECEIVE_ERROR;
        }
    }
    if (context->state != WEBSOCKET_STATE_OPEN)
        return context->state == WEBSOCKET_STATE_CLOSED
            ? RECORD_RECEIVE_CLOSED : RECORD_RECEIVE_EMPTY;

    if (wslay_event_want_read(context->event)
        && wslay_event_recv(context->event) != 0) {
        WebSocket_fail(context);
        return RECORD_RECEIVE_ERROR;
    }
    if (context->state == WEBSOCKET_STATE_FAILED)
        return RECORD_RECEIVE_ERROR;
    flush_result = WebSocket_flush_frames(context);
    if (flush_result == RECORD_FLUSH_ERROR)
        return RECORD_RECEIVE_ERROR;
    queued_result = WebSocket_pop_record(
        context, destination, capacity, length);
    if (queued_result != RECORD_RECEIVE_EMPTY)
        return queued_result;
    if (context->state == WEBSOCKET_STATE_CLOSED)
        return RECORD_RECEIVE_CLOSED;
    if (context->peer_eof) {
        if (wslay_event_get_close_received(context->event)) {
            WebSocket_finish(context);
            return RECORD_RECEIVE_CLOSED;
        }
        errno = ECONNRESET;
        WebSocket_fail(context);
        return RECORD_RECEIVE_ERROR;
    }
    return RECORD_RECEIVE_EMPTY;
}

static record_send_result_t WebSocket_send(
    void *user_data, const char *payload, size_t length,
    record_delivery_t delivery)
{
    websocket_transport_context_t *context = user_data;
    record_flush_result_t flush_result;
    struct wslay_event_msg message;
    int status;

    if (context->state == WEBSOCKET_STATE_FAILED)
        return RECORD_SEND_ERROR;
    if (context->state == WEBSOCKET_STATE_CLOSED)
        return RECORD_SEND_CLOSED;
    flush_result = WebSocket_flush(context);
    if (flush_result == RECORD_FLUSH_ERROR)
        return RECORD_SEND_ERROR;
    if (flush_result == RECORD_FLUSH_CLOSED)
        return RECORD_SEND_CLOSED;
    if (context->state == WEBSOCKET_STATE_CLIENT_RESPONSE) {
        int handshake_result = WebSocket_read_handshake(context);

        if (handshake_result < 0)
            return context->state == WEBSOCKET_STATE_CLOSED
                ? RECORD_SEND_CLOSED : RECORD_SEND_ERROR;
    }
    if (context->state != WEBSOCKET_STATE_OPEN
        || flush_result == RECORD_FLUSH_PENDING
        || wslay_event_want_write(context->event))
        return delivery == RECORD_DELIVERY_TRANSIENT
            ? RECORD_SEND_DROPPED : RECORD_SEND_BACKPRESSURED;
    if (length > context->send_capacity) {
        errno = EMSGSIZE;
        return RECORD_SEND_ERROR;
    }

    message.opcode = WSLAY_BINARY_FRAME;
    message.msg = (const uint8_t *)payload;
    message.msg_length = length;
    /* wslay copies non-fragmented payloads while queueing, which preserves
     * Record_transport's accepted-record ownership contract. */
    status = wslay_event_queue_msg(context->event, &message);
    if (status == WSLAY_ERR_NO_MORE_MSG)
        return RECORD_SEND_CLOSED;
    if (status != 0) {
        errno = status == WSLAY_ERR_NOMEM ? ENOMEM : EINVAL;
        return RECORD_SEND_ERROR;
    }
    if (wslay_event_send(context->event) != 0) {
        WebSocket_fail(context);
        return RECORD_SEND_ERROR;
    }
    return RECORD_SEND_ACCEPTED;
}

static void WebSocket_close(void *user_data)
{
    websocket_transport_context_t *context = user_data;

    if (context->state == WEBSOCKET_STATE_CLOSED
        || context->state == WEBSOCKET_STATE_FAILED)
        return;
    if (context->state == WEBSOCKET_STATE_OPEN && context->event != NULL) {
        int status = wslay_event_queue_close(
            context->event, WSLAY_CODE_NORMAL_CLOSURE, NULL, 0);

        if (status == 0 || status == WSLAY_ERR_NO_MORE_MSG)
            (void)wslay_event_send(context->event);
    }
    WebSocket_finish(context);
    WebSocket_clear_records(context);
}

static void WebSocket_destroy(void *user_data)
{
    websocket_transport_context_t *context = user_data;

    WebSocket_clear_records(context);
    if (context->event != NULL)
        wslay_event_context_free(context->event);
    free(context);
}

static socket_handle_t WebSocket_native_handle(void *user_data)
{
    websocket_transport_context_t *context = user_data;

    return context->socket.fd;
}

static const record_transport_operations_t websocket_operations = {
    WebSocket_receive,
    WebSocket_send,
    WebSocket_flush,
    WebSocket_close,
    WebSocket_destroy,
    WebSocket_native_handle
};

static websocket_transport_context_t *WebSocket_create_context(
    sock_t *connected_socket, size_t receive_capacity, size_t send_capacity,
    bool server)
{
    websocket_transport_context_t *context;

    if (connected_socket == NULL
        || connected_socket->fd == SOCK_FD_INVALID
        || receive_capacity == 0
        || receive_capacity > RECORD_TRANSPORT_MAX_PAYLOAD
        || send_capacity == 0
        || send_capacity > RECORD_TRANSPORT_MAX_PAYLOAD) {
        errno = EINVAL;
        return NULL;
    }
    if (sock_set_non_blocking(connected_socket, 1) == SOCK_IS_ERROR
        || sock_set_tcp_nodelay(connected_socket, 1) == SOCK_IS_ERROR)
        return NULL;
    context = calloc(1, sizeof(*context));
    if (context == NULL)
        return NULL;
    context->socket = *connected_socket;
    context->server = server;
    context->receive_capacity = receive_capacity;
    context->send_capacity = send_capacity;
    context->state = server
        ? WEBSOCKET_STATE_SERVER_REQUEST
        : WEBSOCKET_STATE_CLIENT_REQUEST;
    return context;
}

static record_transport_t *WebSocket_finish_create(
    websocket_transport_context_t *context, sock_t *connected_socket)
{
    record_transport_t *transport = Record_transport_create(
        &websocket_operations, context);

    if (transport == NULL) {
        free(context);
        return NULL;
    }
    sock_init(connected_socket);
    return transport;
}

record_transport_t *Record_transport_create_websocket_server(
    sock_t *connected_socket, size_t receive_capacity, size_t send_capacity)
{
    websocket_transport_context_t *context = WebSocket_create_context(
        connected_socket, receive_capacity, send_capacity, true);

    if (context == NULL)
        return NULL;
    return WebSocket_finish_create(context, connected_socket);
}

record_transport_t *Record_transport_create_websocket_client(
    sock_t *connected_socket, const char *host, int port,
    size_t receive_capacity, size_t send_capacity)
{
    websocket_transport_context_t *context;
    unsigned char nonce[WEBSOCKET_HTTP_NONCE_SIZE];

    if (host == NULL || port <= 0 || port > 65535) {
        errno = EINVAL;
        return NULL;
    }
    context = WebSocket_create_context(
        connected_socket, receive_capacity, send_capacity, false);
    if (context == NULL)
        return NULL;
    if (!Secure_random_fill(nonce, sizeof(nonce))
        || WebSocket_http_create_client_request(
            context->handshake_output,
            sizeof(context->handshake_output),
            &context->handshake_output_length,
            host, port, nonce,
            context->expected_accept,
            sizeof(context->expected_accept)) == -1) {
        free(context);
        return NULL;
    }
    return WebSocket_finish_create(context, connected_socket);
}
