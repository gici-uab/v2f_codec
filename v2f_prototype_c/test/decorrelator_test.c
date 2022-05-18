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

/**
 * Test that all decorrelation modes are lossless.
 *
 * @req V2F-1.1
 */
void test_decorrelator_lossless(void);

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
                    &decorrelator, mode, max_sample_value, 1024));
        }

        CU_ASSERT_EQUAL_FATAL(
                v2f_decorrelator_create(NULL, V2F_C_DECORRELATOR_MODE_NONE,
                                        max_sample_value, 1024),
                V2F_E_INVALID_PARAMETER);

        CU_ASSERT_EQUAL_FATAL(
                v2f_decorrelator_create(&decorrelator,
                                        V2F_C_DECORRELATOR_MODE_NONE,
                                        0, 1024),
                V2F_E_INVALID_PARAMETER);

        CU_ASSERT_EQUAL_FATAL(
                v2f_decorrelator_create(NULL, V2F_C_DECORRELATOR_MODE_NONE,
                                        max_sample_value, 1024),
                V2F_E_INVALID_PARAMETER);

        CU_ASSERT_EQUAL_FATAL(
                v2f_decorrelator_create(&decorrelator,
                                        V2F_C_DECORRELATOR_MODE_COUNT,
                                        max_sample_value, 1024),
                V2F_E_INVALID_PARAMETER);

        CU_ASSERT_EQUAL_FATAL(
                v2f_decorrelator_create(&decorrelator,
                                        V2F_C_DECORRELATOR_MODE_COUNT + 1,
                                        max_sample_value, 1024),
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

void test_decorrelator_lossless(void) {
    uint64_t sample_count = 1024*1024;
    uint64_t samples_per_row = 1024;
    v2f_sample_t max_sample_value = 255;
    v2f_sample_t* original_samples = malloc(sizeof(v2f_sample_t)*sample_count);
    v2f_sample_t* copy_samples = malloc(sizeof(v2f_sample_t)*sample_count);
    CU_ASSERT_FATAL(original_samples != NULL);
    CU_ASSERT_FATAL(copy_samples != NULL);
    for (uint32_t sample_index=0; sample_index < sample_count; sample_index++) {
        original_samples[sample_index] = (v2f_sample_t) (((sample_index * 73) + 12) % (max_sample_value + 1));
        copy_samples[sample_index] = original_samples[sample_index];
    }

    for (uint32_t mode_id=0; mode_id < V2F_C_DECORRELATOR_MODE_COUNT; mode_id++) {
        v2f_decorrelator_t decorrelator;
        CU_ASSERT_EQUAL_FATAL(v2f_decorrelator_create(&decorrelator, mode_id, max_sample_value, samples_per_row),
                              V2F_E_NONE);

        CU_ASSERT_EQUAL_FATAL(v2f_decorrelator_decorrelate_block(&decorrelator, copy_samples, sample_count),
                              V2F_E_NONE);

        CU_ASSERT_EQUAL_FATAL(v2f_decorrelator_invert_block(&decorrelator, copy_samples, sample_count),
                              V2F_E_NONE);

        for (uint32_t sample_index=0; sample_index<sample_count; sample_index++) {
            if (original_samples[sample_index] != copy_samples[sample_index]) {
                printf("Found error for mode %u and sample index %u (%u vs %u)!\n",
                       mode_id, sample_index, original_samples[sample_index], copy_samples[sample_index]);
                CU_ASSERT_EQUAL_FATAL(original_samples[sample_index], copy_samples[sample_index]);
            }
        }
        CU_ASSERT_EQUAL_FATAL(
                memcmp(original_samples, copy_samples, sizeof(v2f_sample_t)*sample_count),
                0);
    }

}

CU_START_REGISTRATION(decorrelator)
    CU_QADD_TEST(test_decorrelator_create)
    CU_QADD_TEST(test_decorrelator_lossless)
    CU_QADD_TEST(test_decorrelator_prediction_mapping)
CU_END_REGISTRATION()
