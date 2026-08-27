#ifndef XPILOT_TEST_HELPERS_H
#define XPILOT_TEST_HELPERS_H

#include <stdio.h>

#define TEST_CHECK(condition)                                                \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "%s:%d: check failed: %s\n",                  \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

#endif
