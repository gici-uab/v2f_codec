/**
 * @file
 *
 * Test suite for the decorrelator.
 */

#include <stdio.h>
#include <unistd.h>

#include "CUExtension.h"
#include "test_common.h"

#include "../src/timer.h"
#include "../src/log.h"
#include "../src/v2f_decorrelator.h"

/**
 * Exercise decorrelator creation.
 *
 * @req V2F-1.1
 */
void test_decorrelator_create(void);

/**
 * Test the method for mapping positive and negative prediction
 * errors without expanding the dynamic range.
 *
 * * @req V2F-1.1.2
 */
void test_decorrelator_prediction_mapping(void);

void test_decorrelator_create(void) {
    printf("\n:: BEGIN - errors expected ----------------------\n");

    v2f_decorrelator_t decorrelator;
    for (v2f_sample_t max_sample_value = 255;
         max_sample_value <= 65535;
         max_sample_value += (65535 - 255)) {

        for (int mode = V2F_C_DECORRELATOR_MODE_NONE;
             mode < V2F_C_DECORRELATOR_MODE_COUNT;
             mode++) {
            FAIL_IF_FAIL(v2f_decorrelator_create(
                    &decorrelator, mode, max_sample_value));
        }

        CU_ASSERT_EQUAL_FATAL(
                v2f_decorrelator_create(NULL, V2F_C_DECORRELATOR_MODE_NONE,
                                        max_sample_value),
                V2F_E_INVALID_PARAMETER);

        CU_ASSERT_EQUAL_FATAL(
                v2f_decorrelator_create(&decorrelator,
                                        V2F_C_DECORRELATOR_MODE_NONE,
                                        0),
                V2F_E_INVALID_PARAMETER);

        CU_ASSERT_EQUAL_FATAL(
                v2f_decorrelator_create(NULL, V2F_C_DECORRELATOR_MODE_NONE,
                                        max_sample_value),
                V2F_E_INVALID_PARAMETER);

        CU_ASSERT_EQUAL_FATAL(
                v2f_decorrelator_create(&decorrelator,
                                        V2F_C_DECORRELATOR_MODE_COUNT,
                                        max_sample_value),
                V2F_E_INVALID_PARAMETER);

        CU_ASSERT_EQUAL_FATAL(
                v2f_decorrelator_create(&decorrelator,
                                        V2F_C_DECORRELATOR_MODE_COUNT + 1,
                                        max_sample_value),
                V2F_E_INVALID_PARAMETER);
    }

    printf(":: END - errors expected ------------------------\n");
}

void test_decorrelator_prediction_mapping(void) {
    for (v2f_sample_t max_sample_value = 255;
         max_sample_value <= 65535;
         max_sample_value += (65535 - 255)) {

        for (v2f_sample_t sample_value = 0;
            sample_value <= max_sample_value;
            sample_value++) {

            for (v2f_sample_t prediction = 0;
                 prediction <= max_sample_value;
                 prediction += (max_sample_value >> 3)) {
                v2f_sample_t coded = v2f_decorrelator_map_predicted_sample(
                        sample_value, prediction, max_sample_value);
                int64_t decoded = (int64_t) v2f_decorrelator_unmap_sample(
                        coded, prediction, max_sample_value);

                CU_ASSERT_EQUAL_FATAL(decoded, sample_value);
            }

            v2f_sample_t coded = v2f_decorrelator_map_predicted_sample(
                    sample_value, max_sample_value, max_sample_value);
            int64_t decoded = (int64_t) v2f_decorrelator_unmap_sample(
                    coded, max_sample_value, max_sample_value);
            CU_ASSERT_EQUAL_FATAL(decoded, sample_value);
        }
    }
}

CU_START_REGISTRATION(decorrelator)
    CU_QADD_TEST(test_decorrelator_create)
    CU_QADD_TEST(test_decorrelator_prediction_mapping)
CU_END_REGISTRATION()
