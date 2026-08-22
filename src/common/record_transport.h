/*
 * XPilot NG, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef RECORD_TRANSPORT_H
#define RECORD_TRANSPORT_H

#include <stddef.h>

#include "socklib.h"

/** Maximum payload carried by one logical XPilot transport record. */
#define RECORD_TRANSPORT_MAX_PAYLOAD 65535

/** Result of attempting to receive one logical record. */
typedef enum {
    /** A transport or malformed-input error occurred. */
    RECORD_RECEIVE_ERROR = -1,
    /** No complete logical record is currently available. */
    RECORD_RECEIVE_EMPTY = 0,
    /** One complete logical record was copied to the destination. */
    RECORD_RECEIVE_READY = 1,
    /** The peer closed the transport after all queued records were read. */
    RECORD_RECEIVE_CLOSED = 2
} record_receive_result_t;

/** Retention policy used when transport output is backpressured. */
typedef enum {
    /** The caller must retry until the record is accepted or the session ends. */
    RECORD_DELIVERY_REQUIRED = 0,
    /** The transport may discard the record instead of growing its queue. */
    RECORD_DELIVERY_TRANSIENT = 1
} record_delivery_t;

/** Result of submitting one logical record. */
typedef enum {
    /** A transport or invalid-record error occurred. */
    RECORD_SEND_ERROR = -1,
    /** A required record was not accepted and must be retried unchanged. */
    RECORD_SEND_BACKPRESSURED = 0,
    /** The transport accepted ownership of a copy of the record. */
    RECORD_SEND_ACCEPTED = 1,
    /** A transient record was intentionally discarded under backpressure. */
    RECORD_SEND_DROPPED = 2,
    /** The transport is closed and cannot accept another record. */
    RECORD_SEND_CLOSED = 3
} record_send_result_t;

/** Result of advancing already accepted transport output. */
typedef enum {
    /** A transport output error occurred. */
    RECORD_FLUSH_ERROR = -1,
    /** No accepted output remains pending. */
    RECORD_FLUSH_IDLE = 0,
    /** Accepted output remains pending because the transport is backpressured. */
    RECORD_FLUSH_PENDING = 1,
    /** The transport is closed. */
    RECORD_FLUSH_CLOSED = 2
} record_flush_result_t;

/** Opaque ordered, reliable logical-record transport. */
typedef struct record_transport record_transport_t;

/** Callback used to receive one logical record from a transport backend. */
typedef record_receive_result_t (*record_transport_receive_fn)(
    void *context, char *destination, size_t capacity, size_t *length);

/** Callback used to submit one logical record to a transport backend. */
typedef record_send_result_t (*record_transport_send_fn)(
    void *context, const char *payload, size_t length,
    record_delivery_t delivery);

/** Callback used to advance accepted transport output without waiting. */
typedef record_flush_result_t (*record_transport_flush_fn)(void *context);

/** Callback used to close a transport backend. */
typedef void (*record_transport_close_fn)(void *context);

/** Callback used to destroy a transport backend context after it is closed. */
typedef void (*record_transport_destroy_fn)(void *context);

/** Callback used by a native scheduler to obtain an optional handle. */
typedef socket_handle_t (*record_transport_native_handle_fn)(void *context);

/** Operations supplied by a logical-record transport backend. */
typedef struct {
    /** Receive one complete payload without a transport-specific header. */
    record_transport_receive_fn receive;
    /** Submit one complete payload with the requested retention policy. */
    record_transport_send_fn send;
    /** Advance previously accepted output without blocking. */
    record_transport_flush_fn flush;
    /** Close the backend. This operation must be idempotent. */
    record_transport_close_fn close_backend;
    /** Release the backend context after close has been called. */
    record_transport_destroy_fn destroy;
    /** Return a native handle, or SOCK_FD_INVALID when unavailable. */
    record_transport_native_handle_fn native_handle;
} record_transport_operations_t;

/**
 * Create a logical-record transport around caller-supplied backend operations.
 *
 * @param operations Complete backend operation table.
 * @param context Backend context passed to every operation.
 * @return Allocated transport, or NULL on invalid input or allocation failure.
 *
 * @remarks Record_transport_destroy() closes the backend and then invokes its
 * destroy callback. The context is owned by the resulting transport.
 */
record_transport_t *Record_transport_create(
    const record_transport_operations_t *operations, void *context);

/**
 * Create two connected in-memory logical-record transports.
 *
 * @param maximum_records Maximum queued records in each direction.
 * @param maximum_bytes Maximum queued payload bytes in each direction.
 * @param first Receives the first endpoint.
 * @param second Receives the second endpoint.
 * @return Zero on success, or -1 on invalid input or allocation failure.
 */
int Record_transport_create_memory_pair(size_t maximum_records,
                                        size_t maximum_bytes,
                                        record_transport_t **first,
                                        record_transport_t **second);

/**
 * Wrap an already connected native TCP byte stream as logical records.
 *
 * @param connected_socket Connected socket whose ownership is transferred on
 * success.
 * @param receive_capacity Largest accepted incoming record payload.
 * @param send_capacity Largest accepted outgoing record payload.
 * @return Allocated transport, or NULL on invalid input or allocation failure.
 *
 * @remarks The native adapter adds and removes the two-byte network-order TCP
 * length prefix. Callers only observe payload bytes.
 */
record_transport_t *Record_transport_create_tcp(sock_t *connected_socket,
                                                size_t receive_capacity,
                                                size_t send_capacity);

/** Receive at most one complete logical record without waiting. */
record_receive_result_t Record_transport_receive(
    record_transport_t *transport, char *destination, size_t capacity,
    size_t *length);

/** Submit one complete logical record without waiting. */
record_send_result_t Record_transport_send(record_transport_t *transport,
                                           const char *payload,
                                           size_t length,
                                           record_delivery_t delivery);

/** Advance previously accepted output without waiting. */
record_flush_result_t Record_transport_flush(record_transport_t *transport);

/** Close the transport. Repeated calls are allowed. */
void Record_transport_close(record_transport_t *transport);

/** Close and destroy the transport. NULL is accepted. */
void Record_transport_destroy(record_transport_t *transport);

/**
 * Return a handle for a native waiting wrapper.
 *
 * @return Nonnegative native handle, or SOCK_FD_INVALID when unavailable.
 */
socket_handle_t Record_transport_native_handle(record_transport_t *transport);

#endif /* RECORD_TRANSPORT_H */
