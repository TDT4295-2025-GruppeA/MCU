/*
 * test_framework.h
 *
 *  Created on: Oct 2, 2025
 *      Author: jornik
 */

#ifndef INC_TEST_TEST_FRAMEWORK_H_
#define INC_TEST_TEST_FRAMEWORK_H_

#include <stdint.h>

// Test statistics
typedef struct {
    uint32_t tests_run;
    uint32_t tests_passed;
    uint32_t tests_failed;
} TestStats;

// Test macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            UART_Printf("  FAIL: %s (line %d)\r\n", message, __LINE__); \
            test_stats.tests_failed++; \
            return 0; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL(expected, actual, message) \
    TEST_ASSERT((expected) == (actual), message)

#define TEST_ASSERT_FLOAT_EQUAL(expected, actual, tolerance, message) \
    TEST_ASSERT(fabsf((expected) - (actual)) < (tolerance), message)

#define RUN_TEST(test_func) \
    do { \
        UART_Printf("Running: %s... ", #test_func); \
        test_stats.tests_run++; \
        if (test_func()) { \
            UART_Printf("PASS\r\n"); \
            test_stats.tests_passed++; \
        } \
    } while(0)

// Global test stats
extern TestStats test_stats;
extern void UART_Printf(const char* format, ...);

#endif /* INC_TEST_TEST_FRAMEWORK_H_ */
