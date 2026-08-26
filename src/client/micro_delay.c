#include "xpcommon.h"

#include "micro_delay.h"

#ifdef XPILOT_MICRO_DELAY_TEST_HOOKS
# include "micro_delay_test_support.h"
#endif

#define MICROSECONDS_PER_SECOND 1000000UL

#ifdef _WINDOWS
# define MICROSECONDS_PER_MILLISECOND 1000UL

static int Wait_for_duration(unsigned long seconds,
                             unsigned long microseconds)
{
    unsigned long milliseconds = seconds * 1000UL
        + microseconds / MICROSECONDS_PER_MILLISECOND;

    if (microseconds % MICROSECONDS_PER_MILLISECOND != 0)
        milliseconds++;
    Sleep((DWORD)milliseconds);
    return 0;
}
#else
static int Wait_for_duration(unsigned long seconds,
                             unsigned long microseconds)
{
    struct timeval timeout;

    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;
    return select(0, NULL, NULL, NULL, &timeout);
}
#endif

#ifdef XPILOT_MICRO_DELAY_TEST_HOOKS
static MicroDelayWaitForTest wait_for_duration = Wait_for_duration;

void Micro_delay_set_wait_for_test(MicroDelayWaitForTest wait_function)
{
    wait_for_duration = wait_function;
}
#endif

int micro_delay(unsigned usec)
{
    unsigned long seconds =
        (unsigned long)usec / MICROSECONDS_PER_SECOND;
    unsigned long microseconds =
        (unsigned long)usec % MICROSECONDS_PER_SECOND;
    int result;

#ifdef XPILOT_MICRO_DELAY_TEST_HOOKS
    result = wait_for_duration(seconds, microseconds);
#else
    result = Wait_for_duration(seconds, microseconds);
#endif
    return result < 0 ? -1 : 0;
}
