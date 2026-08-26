#include "test_helpers.h"

#include "micro_delay.h"
#include "micro_delay_test_support.h"

#include <limits.h>

#define MICROSECONDS_PER_SECOND 1000000UL

static unsigned long captured_seconds;
static unsigned long captured_microseconds;
static unsigned int call_count;
static int wait_result;

static int capture_wait(unsigned long seconds, unsigned long microseconds)
{
    captured_seconds = seconds;
    captured_microseconds = microseconds;
    call_count++;
    return wait_result;
}

static int check_delay(unsigned usec)
{
    unsigned long expected_seconds =
        (unsigned long)usec / MICROSECONDS_PER_SECOND;
    unsigned long expected_microseconds =
        (unsigned long)usec % MICROSECONDS_PER_SECOND;

    call_count = 0;
    wait_result = 0;
    TEST_CHECK(micro_delay(usec) == 0);
    TEST_CHECK(call_count == 1);
    TEST_CHECK(captured_seconds == expected_seconds);
    TEST_CHECK(captured_microseconds == expected_microseconds);
    return 0;
}

static int check_duration_conversion(void)
{
    TEST_CHECK(check_delay(0U) == 0);
    TEST_CHECK(check_delay(1U) == 0);
    TEST_CHECK(check_delay(999999U) == 0);
    TEST_CHECK(check_delay(1000000U) == 0);
    TEST_CHECK(check_delay(1000001U) == 0);
    TEST_CHECK(check_delay(UINT_MAX) == 0);
    return 0;
}

static int check_wait_result(void)
{
    call_count = 0;
    wait_result = 1;
    TEST_CHECK(micro_delay(250000U) == 0);
    TEST_CHECK(call_count == 1);

    wait_result = -1;
    TEST_CHECK(micro_delay(250000U) == -1);
    TEST_CHECK(call_count == 2);
    return 0;
}

int main(void)
{
    Micro_delay_set_wait_for_test(capture_wait);
    TEST_CHECK(check_duration_conversion() == 0);
    TEST_CHECK(check_wait_result() == 0);
    return 0;
}
