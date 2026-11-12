#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdio.h>

/*
 * Minimal test harness: CHECK records a pass or a failure,
 * TEST_SUMMARY prints totals and returns the exit code.
 */

static int testsPassed = 0;
static int testsFailed = 0;

#define CHECK(condition, name)                                  \
    do                                                          \
    {                                                           \
        if (condition)                                          \
            testsPassed++;                                      \
        else                                                    \
        {                                                       \
            testsFailed++;                                      \
            printf("  FAIL: %s  (%s:%d)\n", name,               \
                   __FILE__, __LINE__);                         \
        }                                                       \
    } while (0)

#define SECTION(title) printf("\n[%s]\n", title)

#define TEST_SUMMARY(suite)                                     \
    (printf("\n%s: %d passed, %d failed\n",                     \
            suite, testsPassed, testsFailed),                   \
     testsFailed == 0 ? 0 : 1)

#endif
