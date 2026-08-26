/*
 * XPilot Infinity, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "xpcommon.h"

#include <limits.h>

#include "record_drain.h"

int Record_drain_ready(record_drain_step_fn step, void *context,
                       size_t maximum_records)
{
    record_receive_result_t result;
    size_t processed_records = 0;

    if (step == NULL || maximum_records == 0 || maximum_records > INT_MAX) {
        errno = EINVAL;
        return -1;
    }

    while (processed_records < maximum_records) {
        result = step(context);
        switch (result) {
        case RECORD_RECEIVE_READY:
            processed_records++;
            break;
        case RECORD_RECEIVE_EMPTY:
        case RECORD_RECEIVE_CLOSED:
        case RECORD_RECEIVE_ERROR:
            return (int)processed_records;
        default:
            errno = EPROTO;
            return -1;
        }
    }
    return (int)processed_records;
}
