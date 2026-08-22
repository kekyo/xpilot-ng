/*
 * XPilot NG, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef RECORDING_INPUT_H
#define RECORDING_INPUT_H

#include <stddef.h>
#include <stdint.h>

#include "record_transport.h"

/**
 * Caller-owned storage and cursors for recorded logical transport input.
 *
 * @remarks Event values encode receive outcomes or ready-record lengths.
 *          Payload storage contains only complete record bodies. Native
 *          handles, byte-stream framing, and partial reads are not stored.
 */
typedef struct {
    /** Storage for receive outcomes and ready-record lengths. */
    int32_t *events;
    /** Maximum number of values in events. */
    size_t event_capacity;
    /** Number of recorded values in events. */
    size_t event_count;
    /** Index of the next event to replay. */
    size_t event_position;
    /** Storage containing consecutive ready-record payloads. */
    char *payloads;
    /** Maximum number of bytes in payloads. */
    size_t payload_capacity;
    /** Number of recorded bytes in payloads. */
    size_t payload_length;
    /** Offset of the next payload to replay. */
    size_t payload_position;
} recording_input_t;

/**
 * Initialize logical-input recording over caller-owned storage.
 *
 * @param recording State to initialize.
 * @param events Event storage retained by recording.
 * @param event_capacity Number of elements available in events.
 * @param payloads Payload storage retained by recording, or NULL when
 *        payload_capacity is zero.
 * @param payload_capacity Number of bytes available in payloads.
 * @return Zero on success, otherwise -1 with errno set to EINVAL.
 */
int Recording_input_initialize(recording_input_t *recording,
                               int32_t *events, size_t event_capacity,
                               char *payloads, size_t payload_capacity);

/**
 * Append one logical transport receive result atomically.
 *
 * @param recording Initialized recording state.
 * @param result Logical transport receive result to append.
 * @param payload Complete payload for RECORD_RECEIVE_READY.
 * @param length Payload size, including zero for an empty logical record.
 * @return Zero on success, otherwise -1.
 *
 * @remarks Capacity failure sets errno to ENOBUFS and leaves both recorded
 *          streams unchanged. Records larger than
 *          RECORD_TRANSPORT_MAX_PAYLOAD are rejected with EMSGSIZE.
 */
int Recording_input_capture(recording_input_t *recording,
                            record_receive_result_t result,
                            const char *payload, size_t length);

/**
 * Rewind both replay cursors without changing recorded data.
 *
 * @param recording Recording state to rewind. NULL is accepted.
 */
void Recording_input_rewind(recording_input_t *recording);

/**
 * Replay at most one recorded logical transport result.
 *
 * @param recording Initialized recording state.
 * @param destination Buffer receiving a ready-record payload, or NULL when
 *        capacity is zero.
 * @param capacity Size of destination in bytes.
 * @param length Receives the payload size, or zero for other results.
 * @return Recorded result, or RECORD_RECEIVE_EMPTY after the final event.
 *
 * @remarks A destination that is too small produces RECORD_RECEIVE_ERROR
 *          with errno set to EMSGSIZE and does not advance either cursor.
 */
record_receive_result_t Recording_input_replay(
    recording_input_t *recording, char *destination, size_t capacity,
    size_t *length);

#endif /* RECORDING_INPUT_H */
