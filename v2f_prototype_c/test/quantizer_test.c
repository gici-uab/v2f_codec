/**
 * @file
 *
 * Test suite for the file interface module.
 */

#include <stdio.h>
#include <unistd.h>

#include "CUExtension.h"
#include "test_common.h"

#include "../src/timer.h"
#include "../src/log.h"
#include "../src/v2f_quantizer.h"

/**
 * Exercise quantizer creation.
 *
 * @req V2F-2.1
 */

void test_quantizer_create(void);

void test_quantizer_create(void) {

    printf("\n:: BEGIN - errors expected ----------------------\n");

    v2f_quantizer_t quantizer;

    for (int mode = V2F_C_QUANTIZER_MODE_NONE;
         mode < V2F_C_QUANTIZER_MODE_COUNT;
         mode++) {
        for (v2f_sample_t step_size = 1;
             step_size <= V2F_C_QUANTIZER_MODE_MAX_STEP_SIZE;
            step_size++) {
            if (mode == V2F_C_QUANTIZER_MODE_NONE && step_size > 1) {
                continue;
            }

            FAIL_IF_FAIL(v2f_quantizer_create(&quantizer, mode, step_size, V2F_C_MAX_SAMPLE_VALUE));
        }
    }

    CU_ASSERT_EQUAL_FATAL(
            v2f_quantizer_create(NULL, V2F_C_QUANTIZER_MODE_NONE, 1, V2F_C_MAX_SAMPLE_VALUE),
            V2F_E_INVALID_PARAMETER);

    CU_ASSERT_EQUAL_FATAL(
            v2f_quantizer_create(&quantizer, V2F_C_QUANTIZER_MODE_UNIFORM + 1, 1, V2F_C_MAX_SAMPLE_VALUE),
            V2F_E_INVALID_PARAMETER);

    CU_ASSERT_EQUAL_FATAL(
            v2f_quantizer_create(&quantizer, V2F_C_QUANTIZER_MODE_NONE, 0, V2F_C_MAX_SAMPLE_VALUE),
            V2F_E_INVALID_PARAMETER);

    CU_ASSERT_EQUAL_FATAL(
            v2f_quantizer_create(&quantizer, V2F_C_QUANTIZER_MODE_NONE, V2F_C_QUANTIZER_MODE_MAX_STEP_SIZE + 1,
                                 V2F_C_MAX_SAMPLE_VALUE),
            V2F_E_INVALID_PARAMETER);

    CU_ASSERT_EQUAL_FATAL(
            v2f_quantizer_create(&quantizer, V2F_C_QUANTIZER_MODE_NONE, V2F_C_QUANTIZER_MODE_MAX_STEP_SIZE,
                                 V2F_C_MAX_SAMPLE_VALUE + 1),
            V2F_E_INVALID_PARAMETER);

    printf(":: END - errors expected ------------------------\n");
}

CU_START_REGISTRATION(quantizer)
    CU_QADD_TEST(test_quantizer_create)
CU_END_REGISTRATION()
