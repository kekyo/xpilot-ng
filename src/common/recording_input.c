/*
 * XPilot Infinity, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "xpcommon.h"

#include "recording_input.h"

typedef enum {
    RECORDING_INPUT_EMPTY = -1,
    RECORDING_INPUT_CLOSED = -2,
    RECORDING_INPUT_ERROR = -3
} recording_input_event_t;

static int Recording_input_invalid(int error_number)
{
    errno = error_number;
    return -1;
}

static int Recording_input_is_valid(const recording_input_t *recording)
{
    return recording != NULL
        && recording->events != NULL
        && recording->event_capacity > 0
        && recording->event_count <= recording->event_capacity
        && recording->event_position <= recording->event_count
        && (recording->payload_capacity == 0
            || recording->payloads != NULL)
        && recording->payload_length <= recording->payload_capacity
        && recording->payload_position <= recording->payload_length;
}

int Recording_input_initialize(recording_input_t *recording,
                               int32_t *events, size_t event_capacity,
                               char *payloads, size_t payload_capacity)
{
    if (recording == NULL || events == NULL || event_capacity == 0
        || (payload_capacity > 0 && payloads == NULL))
        return Recording_input_invalid(EINVAL);

    recording->events = events;
    recording->event_capacity = event_capacity;
    recording->event_count = 0;
    recording->event_position = 0;
    recording->payloads = payloads;
    recording->payload_capacity = payload_capacity;
    recording->payload_length = 0;
    recording->payload_position = 0;
    return 0;
}

int Recording_input_capture(recording_input_t *recording,
                            record_receive_result_t result,
                            const char *payload, size_t length)
{
    int32_t event;

    if (!Recording_input_is_valid(recording))
        return Recording_input_invalid(EINVAL);
    switch (result) {
    case RECORD_RECEIVE_READY:
        if (length > RECORD_TRANSPORT_MAX_PAYLOAD)
            return Recording_input_invalid(EMSGSIZE);
        if (length > 0 && payload == NULL)
            return Recording_input_invalid(EINVAL);
        event = (int32_t)length;
        break;
    case RECORD_RECEIVE_EMPTY:
        if (length != 0)
            return Recording_input_invalid(EINVAL);
        event = RECORDING_INPUT_EMPTY;
        break;
    case RECORD_RECEIVE_CLOSED:
        if (length != 0)
            return Recording_input_invalid(EINVAL);
        event = RECORDING_INPUT_CLOSED;
        break;
    case RECORD_RECEIVE_ERROR:
        if (length != 0)
            return Recording_input_invalid(EINVAL);
        event = RECORDING_INPUT_ERROR;
        break;
    default:
        return Recording_input_invalid(EINVAL);
    }

    if (recording->event_count == recording->event_capacity
        || length > recording->payload_capacity
                        - recording->payload_length)
        return Recording_input_invalid(ENOBUFS);

    recording->events[recording->event_count++] = event;
    if (length > 0) {
        memcpy(recording->payloads + recording->payload_length,
               payload, length);
        recording->payload_length += length;
    }
    return 0;
}

void Recording_input_rewind(recording_input_t *recording)
{
    if (recording == NULL)
        return;
    recording->event_position = 0;
    recording->payload_position = 0;
}

record_receive_result_t Recording_input_replay(
    recording_input_t *recording, char *destination, size_t capacity,
    size_t *length)
{
    int32_t event;

    if (length == NULL || !Recording_input_is_valid(recording)
        || (capacity > 0 && destination == NULL)) {
        errno = EINVAL;
        return RECORD_RECEIVE_ERROR;
    }
    *length = 0;
    if (recording->event_position == recording->event_count)
        return RECORD_RECEIVE_EMPTY;

    event = recording->events[recording->event_position];
    if (event >= 0) {
        size_t payload_length = (size_t)event;

        if (payload_length > RECORD_TRANSPORT_MAX_PAYLOAD
            || payload_length > recording->payload_length
                                    - recording->payload_position) {
            errno = EPROTO;
            return RECORD_RECEIVE_ERROR;
        }
        if (payload_length > capacity) {
            errno = EMSGSIZE;
            return RECORD_RECEIVE_ERROR;
        }
        if (payload_length > 0) {
            memcpy(destination,
                   recording->payloads + recording->payload_position,
                   payload_length);
        }
        recording->event_position++;
        recording->payload_position += payload_length;
        *length = payload_length;
        return RECORD_RECEIVE_READY;
    }

    switch (event) {
    case RECORDING_INPUT_EMPTY:
        recording->event_position++;
        return RECORD_RECEIVE_EMPTY;
    case RECORDING_INPUT_CLOSED:
        recording->event_position++;
        return RECORD_RECEIVE_CLOSED;
    case RECORDING_INPUT_ERROR:
        recording->event_position++;
        errno = EIO;
        return RECORD_RECEIVE_ERROR;
    default:
        errno = EPROTO;
        return RECORD_RECEIVE_ERROR;
    }
}
