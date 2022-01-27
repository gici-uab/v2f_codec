/**
 * @file
 *
 * Unit tests for the coder.
 */

#include <stdio.h>

#include "CUExtension.h"
#include "test_common.h"

#include "../src/v2f_build.h"
#include "../src/timer.h"

/// Evaluate parameter handling in the coder initialization and destruction
void test_create_destroy(void);

/**
 * Test basic coding and decoding with minimal V2F coder/decoder pairs.
 */
void test_coder_basic(void);

void test_create_destroy(void) {
    v2f_entropy_coder_t coder;
    const uint32_t entry_count = 256;
    const uint8_t bytes_per_index = 1;
    const uint8_t bytes_per_word = 1;
    CU_ASSERT_FATAL(entry_count <= UINT16_MAX);
    v2f_entropy_coder_entry_t entries[entry_count];
    const uint16_t root_count = 1;

    v2f_entropy_coder_entry_t *entry_pointers[entry_count];
    for (uint32_t i = 0; i < entry_count; i++) {
        entry_pointers[i] = &(entries[i]);
    }

    CU_ASSERT_FATAL(entry_count <= UINT16_MAX);
    v2f_entropy_coder_entry_t root = {
            .word_bytes = NULL,
            .children_entries = entry_pointers,
            .children_count = (uint16_t) entry_count,
            };
    v2f_entropy_coder_entry_t *roots[] = {&root};

    const v2f_sample_t max_expected_value = (v2f_sample_t)
            (UINT64_C(1) << (8 * bytes_per_index)) - 1;

    // Test create
    CU_ASSERT_EQUAL_FATAL(
            v2f_entropy_coder_create(&coder, max_expected_value,
                                     bytes_per_word,
                                     roots, root_count),
            V2F_E_NONE);
    for (uint32_t i = 0; i <= V2F_C_MAX_SAMPLE_VALUE + 1; i++) {
        if (i == 1) {
            i = V2F_C_MAX_SAMPLE_VALUE + 1;
        }
        CU_ASSERT_EQUAL_FATAL(
                v2f_entropy_coder_create(&coder, i, bytes_per_word,
                                         roots, root_count),
                V2F_E_INVALID_PARAMETER);
    }

    CU_ASSERT_EQUAL_FATAL(
            v2f_entropy_coder_create(&coder, max_expected_value, 0,
                                     roots, root_count),
            V2F_E_INVALID_PARAMETER);
    CU_ASSERT_EQUAL_FATAL(
            v2f_entropy_coder_create(&coder, max_expected_value,
                                     V2F_C_MAX_BYTES_PER_WORD + 1,
                                     roots, root_count),
            V2F_E_INVALID_PARAMETER);
    CU_ASSERT_EQUAL_FATAL(
            v2f_entropy_coder_create(&coder, max_expected_value,
                                     V2F_C_MIN_BYTES_PER_WORD - 1,
                                     roots, root_count),
            V2F_E_INVALID_PARAMETER);


    CU_ASSERT_EQUAL_FATAL(
            v2f_entropy_coder_create(&coder, max_expected_value,
                                     bytes_per_word,
                                     NULL, root_count),
            V2F_E_INVALID_PARAMETER);

    CU_ASSERT_EQUAL_FATAL(
            v2f_entropy_coder_create(&coder, max_expected_value,
                                     bytes_per_word,
                                     roots, 0),
            V2F_E_INVALID_PARAMETER);


    // Test destroy
    CU_ASSERT_EQUAL_FATAL(v2f_entropy_coder_destroy(NULL),
                          V2F_E_INVALID_PARAMETER);
    CU_ASSERT_EQUAL_FATAL(v2f_entropy_coder_destroy(&coder),
                          V2F_E_NONE);
    CU_ASSERT_EQUAL_FATAL(v2f_entropy_coder_destroy(NULL),
                          V2F_E_INVALID_PARAMETER);
}

void test_coder_basic(void) {
    timer_reset();

    for (uint8_t bytes_per_index = V2F_C_MIN_BYTES_PER_WORD;
         bytes_per_index <= V2F_C_MAX_BYTES_PER_WORD;
         bytes_per_index++) {

        const uint32_t entry_count = (uint32_t) (1 << (8 * bytes_per_index));
        const uint64_t repetition_count =
                bytes_per_index == 1 ? 128 * 128 : 256;
        CU_ASSERT_FATAL(entry_count - 1 <= UINT16_MAX);
        const uint64_t sample_count = repetition_count * entry_count;
        v2f_sample_t *samples = malloc(sizeof(v2f_sample_t) * sample_count);
        CU_ASSERT_NOT_EQUAL_FATAL(samples, NULL);
        for (uint64_t i = 0; i < repetition_count; i++) {
            for (uint64_t j = 0; j < entry_count; j++) {
                samples[i * entry_count + j] = (v2f_sample_t) j;
            }
        }
        uint8_t *output_buffer = malloc(
                sizeof(uint8_t) * bytes_per_index * sample_count);
        CU_ASSERT_NOT_EQUAL_FATAL(output_buffer, NULL);
        uint64_t written_byte_count;

        v2f_entropy_coder_t coder;
        v2f_entropy_decoder_t decoder;
        FAIL_IF_FAIL(
                v2f_build_minimal_forest(bytes_per_index, &coder, &decoder));

        char run_name[256];
        sprintf(run_name, "bytes_per_word = %d, samples = %lu",
                (int) bytes_per_index, sample_count);
        timer_start(run_name);
        FAIL_IF_FAIL(v2f_entropy_coder_compress_block(
                &coder, samples, sample_count, output_buffer,
                &written_byte_count));
        timer_stop(run_name);
        CU_ASSERT_EQUAL_FATAL(sample_count * bytes_per_index,
                              written_byte_count);

        v2f_sample_t *output_samples = malloc(
                sizeof(v2f_sample_t) * sample_count);
        uint64_t written_sample_count;
        FAIL_IF_FAIL(v2f_entropy_decoder_decompress_block(
                &decoder, output_buffer, written_byte_count,
                output_samples, sample_count, &written_sample_count));
        CU_ASSERT_EQUAL_FATAL(written_sample_count, sample_count);
        for (uint64_t i = 0; i < sample_count; i++) {
            CU_ASSERT_EQUAL_FATAL(output_samples[i], samples[i]);
        }

        free(samples);
        free(output_samples);
        free(output_buffer);
        v2f_build_destroy_minimal_forest(&coder, &decoder);
    }

//    timer_report_csv(stdout);
}

CU_START_REGISTRATION(entropy_codec)
    CU_QADD_TEST(test_create_destroy)
    CU_QADD_TEST(test_coder_basic)
CU_END_REGISTRATION()
