#include "test_helpers.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "recording_input.h"

static int test_logical_results_round_trip_without_transport_details(void)
{
    static const char payload[] = { 'A', '\0', 'B' };
    int32_t events[5];
    char data[sizeof(payload)];
    char received[sizeof(payload)];
    recording_input_t recording;
    size_t length;

    TEST_CHECK(Recording_input_initialize(
                   &recording, events, 5, data, sizeof(data)) == 0);
    TEST_CHECK(Recording_input_capture(
                   &recording, RECORD_RECEIVE_EMPTY, NULL, 0) == 0);
    TEST_CHECK(Recording_input_capture(
                   &recording, RECORD_RECEIVE_READY,
                   payload, sizeof(payload)) == 0);
    TEST_CHECK(Recording_input_capture(
                   &recording, RECORD_RECEIVE_READY, NULL, 0) == 0);
    TEST_CHECK(Recording_input_capture(
                   &recording, RECORD_RECEIVE_CLOSED, NULL, 0) == 0);
    TEST_CHECK(Recording_input_capture(
                   &recording, RECORD_RECEIVE_ERROR, NULL, 0) == 0);

    Recording_input_rewind(&recording);
    TEST_CHECK(Recording_input_replay(
                   &recording, received, sizeof(received), &length)
               == RECORD_RECEIVE_EMPTY);
    TEST_CHECK(length == 0);
    TEST_CHECK(Recording_input_replay(
                   &recording, received, sizeof(received), &length)
               == RECORD_RECEIVE_READY);
    TEST_CHECK(length == sizeof(payload));
    TEST_CHECK(memcmp(received, payload, length) == 0);
    TEST_CHECK(Recording_input_replay(
                   &recording, received, sizeof(received), &length)
               == RECORD_RECEIVE_READY);
    TEST_CHECK(length == 0);
    TEST_CHECK(Recording_input_replay(
                   &recording, received, sizeof(received), &length)
               == RECORD_RECEIVE_CLOSED);
    TEST_CHECK(length == 0);
    errno = 0;
    TEST_CHECK(Recording_input_replay(
                   &recording, received, sizeof(received), &length)
               == RECORD_RECEIVE_ERROR);
    TEST_CHECK(errno == EIO);
    TEST_CHECK(length == 0);
    return 0;
}

static int test_capacity_failures_do_not_consume_or_append_records(void)
{
    static const char payload[] = "data";
    int32_t events[1];
    char data[sizeof(payload) - 1];
    char too_small[sizeof(payload) - 2];
    char received[sizeof(payload) - 1];
    recording_input_t recording;
    size_t length = 99;

    TEST_CHECK(Recording_input_initialize(
                   &recording, events, 1, data, sizeof(data)) == 0);
    errno = 0;
    TEST_CHECK(Recording_input_capture(
                   &recording, RECORD_RECEIVE_READY,
                   payload, sizeof(payload)) == -1);
    TEST_CHECK(errno == ENOBUFS);
    TEST_CHECK(Recording_input_capture(
                   &recording, RECORD_RECEIVE_READY,
                   payload, sizeof(payload) - 1) == 0);
    errno = 0;
    TEST_CHECK(Recording_input_capture(
                   &recording, RECORD_RECEIVE_EMPTY, NULL, 0) == -1);
    TEST_CHECK(errno == ENOBUFS);

    Recording_input_rewind(&recording);
    errno = 0;
    TEST_CHECK(Recording_input_replay(
                   &recording, too_small, sizeof(too_small), &length)
               == RECORD_RECEIVE_ERROR);
    TEST_CHECK(errno == EMSGSIZE);
    TEST_CHECK(length == 0);
    TEST_CHECK(Recording_input_replay(
                   &recording, received, sizeof(received), &length)
               == RECORD_RECEIVE_READY);
    TEST_CHECK(length == sizeof(payload) - 1);
    TEST_CHECK(memcmp(received, payload, length) == 0);
    TEST_CHECK(Recording_input_replay(
                   &recording, received, sizeof(received), &length)
               == RECORD_RECEIVE_EMPTY);
    TEST_CHECK(length == 0);
    return 0;
}

int main(void)
{
    TEST_CHECK(
        test_logical_results_round_trip_without_transport_details() == 0);
    TEST_CHECK(
        test_capacity_failures_do_not_consume_or_append_records() == 0);
    return 0;
}
