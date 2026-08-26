/*
 * XPilot Infinity, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "xpcommon.h"

#include "record_transport.h"

struct record_transport {
    record_transport_operations_t operations;
    void *context;
};

typedef struct memory_record {
    struct memory_record *next;
    size_t length;
    char payload[];
} memory_record_t;

typedef struct {
    memory_record_t *head;
    memory_record_t *tail;
    size_t record_count;
    size_t byte_count;
    bool writer_closed;
} memory_record_queue_t;

typedef struct {
    memory_record_queue_t incoming[2];
    size_t maximum_records;
    size_t maximum_bytes;
    int references;
    bool endpoint_closed[2];
} memory_transport_pair_t;

typedef struct {
    memory_transport_pair_t *pair;
    int index;
    bool closed;
} memory_transport_context_t;

typedef struct {
    sock_t socket;
    sockbuf_t input;
    sockbuf_t output;
    bool input_initialized;
    bool output_initialized;
    bool terminal;
    bool failed;
} tcp_transport_context_t;

static void Memory_queue_clear(memory_record_queue_t *queue)
{
    memory_record_t *record = queue->head;

    while (record != NULL) {
        memory_record_t *next = record->next;

        free(record);
        record = next;
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->record_count = 0;
    queue->byte_count = 0;
}

static record_receive_result_t Memory_receive(void *context,
                                               char *destination,
                                               size_t capacity,
                                               size_t *length)
{
    memory_transport_context_t *endpoint = context;
    memory_record_queue_t *queue;
    memory_record_t *record;

    if (endpoint->closed)
        return RECORD_RECEIVE_CLOSED;
    queue = &endpoint->pair->incoming[endpoint->index];
    record = queue->head;
    if (record == NULL)
        return queue->writer_closed
            ? RECORD_RECEIVE_CLOSED : RECORD_RECEIVE_EMPTY;
    if (record->length > capacity) {
        errno = EMSGSIZE;
        return RECORD_RECEIVE_ERROR;
    }

    memcpy(destination, record->payload, record->length);
    *length = record->length;
    queue->head = record->next;
    if (queue->head == NULL)
        queue->tail = NULL;
    queue->record_count--;
    queue->byte_count -= record->length;
    free(record);
    return RECORD_RECEIVE_READY;
}

static record_send_result_t Memory_send(void *context, const char *payload,
                                        size_t length,
                                        record_delivery_t delivery)
{
    memory_transport_context_t *endpoint = context;
    memory_transport_pair_t *pair = endpoint->pair;
    int peer_index = 1 - endpoint->index;
    memory_record_queue_t *queue = &pair->incoming[peer_index];
    memory_record_t *record;

    if (endpoint->closed || pair->endpoint_closed[peer_index])
        return RECORD_SEND_CLOSED;
    if (queue->record_count >= pair->maximum_records
        || length > pair->maximum_bytes - queue->byte_count)
        return delivery == RECORD_DELIVERY_TRANSIENT
            ? RECORD_SEND_DROPPED : RECORD_SEND_BACKPRESSURED;

    record = malloc(sizeof(*record) + length);
    if (record == NULL)
        return RECORD_SEND_ERROR;
    record->next = NULL;
    record->length = length;
    memcpy(record->payload, payload, length);
    if (queue->tail != NULL)
        queue->tail->next = record;
    else
        queue->head = record;
    queue->tail = record;
    queue->record_count++;
    queue->byte_count += length;
    return RECORD_SEND_ACCEPTED;
}

static record_flush_result_t Memory_flush(void *context)
{
    memory_transport_context_t *endpoint = context;

    return endpoint->closed ? RECORD_FLUSH_CLOSED : RECORD_FLUSH_IDLE;
}

static void Memory_close(void *context)
{
    memory_transport_context_t *endpoint = context;
    memory_transport_pair_t *pair;
    int peer_index;

    if (endpoint->closed)
        return;
    endpoint->closed = true;
    pair = endpoint->pair;
    peer_index = 1 - endpoint->index;
    pair->endpoint_closed[endpoint->index] = true;
    pair->incoming[peer_index].writer_closed = true;
    Memory_queue_clear(&pair->incoming[endpoint->index]);
}

static void Memory_destroy(void *context)
{
    memory_transport_context_t *endpoint = context;
    memory_transport_pair_t *pair = endpoint->pair;

    pair->references--;
    free(endpoint);
    if (pair->references == 0) {
        Memory_queue_clear(&pair->incoming[0]);
        Memory_queue_clear(&pair->incoming[1]);
        free(pair);
    }
}

static socket_handle_t Memory_native_handle(void *context)
{
    (void)context;
    return SOCK_FD_INVALID;
}

static const record_transport_operations_t memory_operations = {
    Memory_receive,
    Memory_send,
    Memory_flush,
    Memory_close,
    Memory_destroy,
    Memory_native_handle
};

static bool Tcp_output_pending(const tcp_transport_context_t *context)
{
    return context->output.len > 0
        || context->output.frame_output_offset
            < context->output.frame_output_len;
}

static record_receive_result_t Tcp_receive(void *context, char *destination,
                                            size_t capacity, size_t *length)
{
    tcp_transport_context_t *tcp = context;
    bool partial;
    int status;

    if (tcp->terminal)
        return tcp->failed ? RECORD_RECEIVE_ERROR : RECORD_RECEIVE_CLOSED;
    status = Sockbuf_read(&tcp->input);
    if (status < 0) {
        partial = tcp->input.frame_header_len > 0
            || tcp->input.frame_received > 0;
        if (errno == ECONNRESET && !partial) {
            tcp->terminal = true;
            return RECORD_RECEIVE_CLOSED;
        }
        tcp->terminal = true;
        tcp->failed = true;
        return RECORD_RECEIVE_ERROR;
    }
    if (status == 0)
        return RECORD_RECEIVE_EMPTY;
    if ((size_t)tcp->input.len > capacity) {
        errno = EMSGSIZE;
        tcp->terminal = true;
        tcp->failed = true;
        return RECORD_RECEIVE_ERROR;
    }

    memcpy(destination, tcp->input.buf, (size_t)tcp->input.len);
    *length = (size_t)tcp->input.len;
    Sockbuf_clear(&tcp->input);
    return RECORD_RECEIVE_READY;
}

static record_flush_result_t Tcp_flush(void *context)
{
    tcp_transport_context_t *tcp = context;

    if (tcp->terminal)
        return tcp->failed ? RECORD_FLUSH_ERROR : RECORD_FLUSH_CLOSED;
    if (!Tcp_output_pending(tcp))
        return RECORD_FLUSH_IDLE;
    if (Sockbuf_flush(&tcp->output) < 0) {
        tcp->terminal = true;
        tcp->failed = true;
        return RECORD_FLUSH_ERROR;
    }
    return Tcp_output_pending(tcp)
        ? RECORD_FLUSH_PENDING : RECORD_FLUSH_IDLE;
}

static record_send_result_t Tcp_send(void *context, const char *payload,
                                     size_t length,
                                     record_delivery_t delivery)
{
    tcp_transport_context_t *tcp = context;
    record_flush_result_t flush_result;

    if (tcp->terminal)
        return tcp->failed ? RECORD_SEND_ERROR : RECORD_SEND_CLOSED;
    flush_result = Tcp_flush(tcp);
    if (flush_result == RECORD_FLUSH_ERROR)
        return RECORD_SEND_ERROR;
    if (flush_result == RECORD_FLUSH_CLOSED)
        return RECORD_SEND_CLOSED;
    if (flush_result == RECORD_FLUSH_PENDING)
        return delivery == RECORD_DELIVERY_TRANSIENT
            ? RECORD_SEND_DROPPED : RECORD_SEND_BACKPRESSURED;
    if (length > (size_t)tcp->output.size) {
        errno = EMSGSIZE;
        return RECORD_SEND_ERROR;
    }

    Sockbuf_clear(&tcp->output);
    memcpy(tcp->output.buf, payload, length);
    tcp->output.len = (int)length;
    if (Sockbuf_flush(&tcp->output) < 0) {
        tcp->terminal = true;
        tcp->failed = true;
        return RECORD_SEND_ERROR;
    }
    return RECORD_SEND_ACCEPTED;
}

static void Tcp_close(void *context)
{
    tcp_transport_context_t *tcp = context;

    tcp->terminal = true;
    if (tcp->socket.fd != SOCK_FD_INVALID)
        sock_close(&tcp->socket);
}

static void Tcp_destroy(void *context)
{
    tcp_transport_context_t *tcp = context;

    if (tcp->output_initialized)
        Sockbuf_cleanup(&tcp->output);
    if (tcp->input_initialized)
        Sockbuf_cleanup(&tcp->input);
    free(tcp);
}

static socket_handle_t Tcp_native_handle(void *context)
{
    tcp_transport_context_t *tcp = context;

    return tcp->socket.fd;
}

static const record_transport_operations_t tcp_operations = {
    Tcp_receive,
    Tcp_send,
    Tcp_flush,
    Tcp_close,
    Tcp_destroy,
    Tcp_native_handle
};

record_transport_t *Record_transport_create(
    const record_transport_operations_t *operations, void *context)
{
    record_transport_t *transport;

    if (operations == NULL || context == NULL
        || operations->receive == NULL || operations->send == NULL
        || operations->flush == NULL || operations->close_backend == NULL
        || operations->destroy == NULL) {
        errno = EINVAL;
        return NULL;
    }
    transport = malloc(sizeof(*transport));
    if (transport == NULL)
        return NULL;
    transport->operations = *operations;
    transport->context = context;
    return transport;
}

int Record_transport_create_memory_pair(size_t maximum_records,
                                        size_t maximum_bytes,
                                        record_transport_t **first,
                                        record_transport_t **second)
{
    memory_transport_pair_t *pair;
    memory_transport_context_t *first_context;
    memory_transport_context_t *second_context;
    record_transport_t *first_transport;
    record_transport_t *second_transport;

    if (maximum_records == 0 || maximum_bytes == 0
        || first == NULL || second == NULL) {
        errno = EINVAL;
        return -1;
    }
    pair = calloc(1, sizeof(*pair));
    first_context = calloc(1, sizeof(*first_context));
    second_context = calloc(1, sizeof(*second_context));
    if (pair == NULL || first_context == NULL || second_context == NULL) {
        free(second_context);
        free(first_context);
        free(pair);
        return -1;
    }

    pair->maximum_records = maximum_records;
    pair->maximum_bytes = maximum_bytes;
    pair->references = 2;
    first_context->pair = pair;
    first_context->index = 0;
    second_context->pair = pair;
    second_context->index = 1;
    first_transport = Record_transport_create(&memory_operations,
                                               first_context);
    second_transport = Record_transport_create(&memory_operations,
                                                second_context);
    if (first_transport == NULL || second_transport == NULL) {
        free(second_transport);
        free(first_transport);
        free(second_context);
        free(first_context);
        free(pair);
        return -1;
    }
    *first = first_transport;
    *second = second_transport;
    return 0;
}

record_transport_t *Record_transport_create_tcp(sock_t *connected_socket,
                                                size_t receive_capacity,
                                                size_t send_capacity)
{
    tcp_transport_context_t *context;
    record_transport_t *transport;

    if (connected_socket == NULL
        || connected_socket->fd == SOCK_FD_INVALID
        || receive_capacity == 0
        || receive_capacity > RECORD_TRANSPORT_MAX_PAYLOAD
        || send_capacity == 0
        || send_capacity > RECORD_TRANSPORT_MAX_PAYLOAD) {
        errno = EINVAL;
        return NULL;
    }
    context = calloc(1, sizeof(*context));
    if (context == NULL)
        return NULL;
    context->socket = *connected_socket;
    if (Sockbuf_init(&context->input, &context->socket, receive_capacity,
                     SOCKBUF_READ | SOCKBUF_FRAMED) == -1)
        goto failure;
    context->input_initialized = true;
    if (Sockbuf_init(&context->output, &context->socket, send_capacity,
                     SOCKBUF_WRITE | SOCKBUF_FRAMED) == -1)
        goto failure;
    context->output_initialized = true;
    transport = Record_transport_create(&tcp_operations, context);
    if (transport == NULL)
        goto failure;

    sock_init(connected_socket);
    return transport;

failure:
    if (context->output_initialized)
        Sockbuf_cleanup(&context->output);
    if (context->input_initialized)
        Sockbuf_cleanup(&context->input);
    free(context);
    return NULL;
}

record_receive_result_t Record_transport_receive(
    record_transport_t *transport, char *destination, size_t capacity,
    size_t *length)
{
    if (transport == NULL || destination == NULL || capacity == 0
        || length == NULL) {
        errno = EINVAL;
        return RECORD_RECEIVE_ERROR;
    }
    *length = 0;
    return transport->operations.receive(
        transport->context, destination, capacity, length);
}

record_send_result_t Record_transport_send(record_transport_t *transport,
                                           const char *payload,
                                           size_t length,
                                           record_delivery_t delivery)
{
    if (transport == NULL || payload == NULL || length == 0
        || length > RECORD_TRANSPORT_MAX_PAYLOAD
        || (delivery != RECORD_DELIVERY_REQUIRED
            && delivery != RECORD_DELIVERY_TRANSIENT)) {
        errno = EINVAL;
        return RECORD_SEND_ERROR;
    }
    return transport->operations.send(
        transport->context, payload, length, delivery);
}

record_flush_result_t Record_transport_flush(record_transport_t *transport)
{
    if (transport == NULL) {
        errno = EINVAL;
        return RECORD_FLUSH_ERROR;
    }
    return transport->operations.flush(transport->context);
}

void Record_transport_close(record_transport_t *transport)
{
    if (transport != NULL)
        transport->operations.close_backend(transport->context);
}

void Record_transport_destroy(record_transport_t *transport)
{
    if (transport == NULL)
        return;
    transport->operations.close_backend(transport->context);
    transport->operations.destroy(transport->context);
    free(transport);
}

socket_handle_t Record_transport_native_handle(record_transport_t *transport)
{
    if (transport == NULL || transport->operations.native_handle == NULL)
        return SOCK_FD_INVALID;
    return transport->operations.native_handle(transport->context);
}
