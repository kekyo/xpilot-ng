#ifndef MICRO_DELAY_H
#define MICRO_DELAY_H

/**
 * Pause the current thread for a number of microseconds.
 *
 * @param usec Requested delay in microseconds.
 * @return Zero after the timeout expires, or -1 if the platform wait fails.
 */
int micro_delay(unsigned usec);

#endif
