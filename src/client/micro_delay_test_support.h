#ifndef MICRO_DELAY_TEST_SUPPORT_H
#define MICRO_DELAY_TEST_SUPPORT_H

#ifdef XPILOT_MICRO_DELAY_TEST_HOOKS

/** Test replacement for the platform wait boundary. */
typedef int (*MicroDelayWaitForTest)(unsigned long seconds,
                                     unsigned long microseconds);

/**
 * Replace the platform wait boundary for a test process.
 *
 * @param wait_function Non-NULL function invoked by micro_delay().
 */
void Micro_delay_set_wait_for_test(MicroDelayWaitForTest wait_function);

#endif

#endif
