/*
 * XPilot NG, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef RECORD_DRAIN_H
#define RECORD_DRAIN_H

#include <stddef.h>

#include "record_transport.h"

/** Callback that advances one logical input record. */
typedef record_receive_result_t (*record_drain_step_fn)(void *context);

/**
 * Process ready logical records without monopolizing one scheduler tick.
 *
 * @param step Callback that consumes at most one logical input record.
 * @param context Opaque value passed to step.
 * @param maximum_records Fairness limit for ready records in one call.
 * @return Number of ready records processed, or -1 for invalid arguments or
 *         an invalid callback result.
 *
 * @remarks Processing stops when step reports empty, closed, or error input,
 *          or after maximum_records ready records.
 */
int Record_drain_ready(record_drain_step_fn step, void *context,
                       size_t maximum_records);

#endif /* RECORD_DRAIN_H */
