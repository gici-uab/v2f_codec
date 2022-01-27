/**
 * @file
 *
 * @brief Extension to the CUnit library to allow simplified test suite declaration/registration
 *   and automatic floating point exception checking in all tests.
 *
 * Usage:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * CU_START_REGISTRATION(suite_name)
 * CU_QADD_TEST(test_function_1)
 * CU_QADD_TEST(test_function_2)
 * // ...
 * CU_QADD_TEST(test_function_N)
 * CU_END_REGISTRATION()
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef CU_EXTENSION_H
#define CU_EXTENSION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>

#include <CUnit/CUError.h>

/**
 * Register a test suite.
 *
 * @note The @a register_<test_suite_name> function must have been defined in @ref suite_registration.h
 *
 * @param name name of the test suite to register
 */
#define CU_START_REGISTRATION(name) \
void register_ ## name (void); \
void register_ ## name (void) { \
CU_pSuite pSuite = CU_add_suite(#name, NULL, NULL); \
if (!pSuite) { fprintf(stderr, "%s: %s\n", __FILE__, CU_get_error_msg()); exit(EXIT_FAILURE); }

/**
 * Add a test to a test suite.
 *
 * Must appear between @ref CU_START_REGISTRATION and @ref CU_END_REGISTRATION
 *
 * @param test test function to add to the current suite
 */
#define CU_QADD_TEST(test) { \
CU_pTest pTest = CU_add_test(pSuite, #test, test); \
if (! pTest) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, CU_get_error_msg()); exit(EXIT_FAILURE); } \
}

/**
 * Finish the registration of the current test suite
 */
#define CU_END_REGISTRATION() }

/**
 * Like @ref RETURN_IF_FAIL, but fatally fails a test.
 */
#define FAIL_IF_FAIL(x) do { v2f_error_t return_value = (x); \
        if (return_value != V2F_E_NONE) { printf("FAIL_IF_FAIL: Error =  %d\n", return_value);} \
        CU_ASSERT_EQUAL_FATAL(return_value, V2F_E_NONE) } while(0)


#endif /* CU_EXTENSION_H */
