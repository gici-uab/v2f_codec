/**
 * @file
 *
 * Test suite for the compressor.
 */

#include <stdio.h>
#include <unistd.h>

#include "CUExtension.h"
#include "test_common.h"

#include "../src/timer.h"
#include "../src/log.h"
#include "../src/v2f_compressor.h"
#include "../src/v2f_decompressor.h"
#include "../src/v2f_compressor.h"
#include "../src/v2f_entropy_coder.h"
#include "../src/v2f_entropy_decoder.h"
#include "../src/v2f_build.h"

/**
 * Exercise code creation and destruction functions.
 *
 * @req V2F-1.3
 */
void test_compressor_decompressor_create_destroy(void);

/**
 * Test the compression/decompression cycle step by step,
 * testing several parameters of the pipeline and a minimal entropy codec.
 *
 * This exercises all quantization step and decorrelation mode combinations
 * with a minimal V2F entropy coder.
 *
 * @req V2F-1.1, V2F-1.2, V2F-1.3, V2F-1.4, V2F-2.1
 */
void test_compression_decompression_steps(void);

/**
 * Test the compression/decompression cycle with
 * @ref v2f_compressor_t and @ref v2f_decompressor_t structures.
 *
 * @req V2F-1.1, V2F-1.2, V2F-1.3, V2F-1.4
 */
void test_compression_decompression_minimal_codec(void);

void test_compressor_decompressor_create_destroy(void) {
    v2f_quantizer_t quantizer;
    v2f_decorrelator_t decorrelator;
    v2f_entropy_coder_t entropy_coder;
    v2f_compressor_t compressor;


    FAIL_IF_FAIL(v2f_compressor_create(
            &compressor, &quantizer, &decorrelator, &entropy_coder));

    CU_ASSERT_EQUAL_FATAL(
            v2f_compressor_create(
                    &compressor, NULL, &decorrelator, &entropy_coder),
            V2F_E_INVALID_PARAMETER);

    CU_ASSERT_EQUAL_FATAL(
            v2f_compressor_create(
                    &compressor, &quantizer, NULL, &entropy_coder),
            V2F_E_INVALID_PARAMETER);

    CU_ASSERT_EQUAL_FATAL(
            v2f_compressor_create(
                    &compressor, &quantizer, &decorrelator, NULL),
            V2F_E_INVALID_PARAMETER);

    {
        v2f_compressor_t tmp_compressor;
        v2f_decompressor_t tmp_decompressor;
        CU_ASSERT_EQUAL_FATAL(
                v2f_build_destroy_minimal_codec(NULL, NULL),
                V2F_E_INVALID_PARAMETER);
        CU_ASSERT_EQUAL_FATAL(
                v2f_build_destroy_minimal_codec(&tmp_compressor, NULL),
                V2F_E_INVALID_PARAMETER);
        CU_ASSERT_EQUAL_FATAL(
                v2f_build_destroy_minimal_codec(NULL, &tmp_decompressor),
                V2F_E_INVALID_PARAMETER);


        v2f_entropy_coder_t tmp_entropy_coder;
        v2f_entropy_decoder_t tmp_entropy_decoder;
        CU_ASSERT_EQUAL_FATAL(
                v2f_build_minimal_forest(1, &tmp_entropy_coder, NULL),
                V2F_E_INVALID_PARAMETER);
        CU_ASSERT_EQUAL_FATAL(
                v2f_build_minimal_forest(1, NULL, &tmp_entropy_decoder),
                V2F_E_INVALID_PARAMETER);
        CU_ASSERT_EQUAL_FATAL(v2f_build_minimal_forest(1, NULL, NULL),
                              V2F_E_INVALID_PARAMETER);
        CU_ASSERT_EQUAL_FATAL(v2f_build_destroy_minimal_forest(NULL, NULL),
                              V2F_E_INVALID_PARAMETER);
        CU_ASSERT_EQUAL_FATAL(
                v2f_build_destroy_minimal_forest(&tmp_entropy_coder, NULL),
                V2F_E_INVALID_PARAMETER);
        CU_ASSERT_EQUAL_FATAL(
                v2f_build_destroy_minimal_forest(NULL, &tmp_entropy_decoder),
                V2F_E_INVALID_PARAMETER);

        CU_ASSERT_EQUAL_FATAL(
                v2f_decompressor_create(NULL, NULL, NULL, NULL),
                V2F_E_INVALID_PARAMETER);

        v2f_quantizer_t tmp_quantizer;
        CU_ASSERT_EQUAL_FATAL(
                v2f_decompressor_create(NULL, &tmp_quantizer, &decorrelator,
                                        &tmp_entropy_decoder),
                V2F_E_INVALID_PARAMETER);
        CU_ASSERT_EQUAL_FATAL(
                v2f_decompressor_create(&tmp_decompressor, NULL, &decorrelator,
                                        &tmp_entropy_decoder),
                V2F_E_INVALID_PARAMETER);
        CU_ASSERT_EQUAL_FATAL(
                v2f_decompressor_create(&tmp_decompressor, &tmp_quantizer,
                                        NULL, &tmp_entropy_decoder),
                V2F_E_INVALID_PARAMETER);
        CU_ASSERT_EQUAL_FATAL(
                v2f_decompressor_create(&tmp_decompressor, &tmp_quantizer,
                                        &decorrelator, NULL),
                V2F_E_INVALID_PARAMETER);
    }

}

void test_compression_decompression_steps(void) {
    timer_reset();

    for (v2f_quantizer_mode_t quantizer_mode = V2F_C_QUANTIZER_MODE_NONE;
         quantizer_mode < V2F_C_QUANTIZER_MODE_COUNT;
         quantizer_mode++) {
        for (v2f_decorrelator_mode_t decorrelator_mode = V2F_C_DECORRELATOR_MODE_NONE;
             decorrelator_mode < V2F_C_DECORRELATOR_MODE_COUNT;
             decorrelator_mode++) {
            for (v2f_sample_t qstep = 1; qstep <= 5; qstep++) {
                for (uint8_t bytes_per_word = V2F_C_MIN_BYTES_PER_WORD;
                     bytes_per_word <= V2F_C_MAX_BYTES_PER_WORD;
                     bytes_per_word++) {

                    if (quantizer_mode == V2F_C_QUANTIZER_MODE_NONE &&
                        qstep > 1) {
                        continue;
                    }

                    const v2f_sample_t max_sample_value =
                            (v2f_sample_t) ((1 << (8 * bytes_per_word)) - 1);

                    // Allocate and initialize entries
                    const uint32_t entry_count = 1024;
                    const uint64_t repetition_count = 32;
                    CU_ASSERT_FATAL(entry_count - 1 <= UINT16_MAX);
                    const uint64_t sample_count =
                            repetition_count * entry_count;
                    v2f_sample_t *original_samples = malloc(
                            sizeof(v2f_sample_t) * sample_count);
                    v2f_sample_t *samples = malloc(
                            sizeof(v2f_sample_t) * sample_count);
                    v2f_sample_t *reconstructed_samples = malloc(
                            sizeof(v2f_sample_t) * sample_count);
                    CU_ASSERT_NOT_EQUAL_FATAL(original_samples, NULL);
                    CU_ASSERT_NOT_EQUAL_FATAL(samples, NULL);
                    CU_ASSERT_NOT_EQUAL_FATAL(reconstructed_samples, NULL);
                    for (uint64_t i = 0; i < repetition_count; i++) {
                        for (uint64_t j = 0; j < entry_count; j++) {
                            samples[i * entry_count + j] =
                                    (v2f_sample_t) (j %
                                                    (max_sample_value + 1));
                            original_samples[i * entry_count + j] = samples[
                                    i * entry_count + j];
                        }
                    }
                    // Make sure to use the whole range
                    samples[1] = max_sample_value;
                    original_samples[1] = max_sample_value;
                    uint8_t *output_buffer = malloc(
                            sizeof(uint8_t) * bytes_per_word * sample_count);
                    CU_ASSERT_NOT_EQUAL_FATAL(output_buffer, NULL);
                    uint64_t written_byte_count;

                    // Build quantizer
                    v2f_quantizer_t quantizer;
                    FAIL_IF_FAIL(v2f_quantizer_create(
                            &quantizer, quantizer_mode, qstep, max_sample_value));

                    // Build decorrelator
                    v2f_decorrelator_t decorrelator;
                    FAIL_IF_FAIL(v2f_decorrelator_create(
                            &decorrelator, decorrelator_mode,
                            max_sample_value));
                    decorrelator.samples_per_row = 512;

                    // Build entropy codec
                    v2f_entropy_coder_t entropy_coder;
                    v2f_entropy_decoder_t entropy_decoder;
                    FAIL_IF_FAIL(v2f_build_minimal_forest(
                            bytes_per_word, &entropy_coder, &entropy_decoder));

                    // Build compressor and compress
                    v2f_compressor_t compressor;
                    FAIL_IF_FAIL(v2f_compressor_create(&compressor, &quantizer,
                                                       &decorrelator,
                                                       &entropy_coder));
                    FAIL_IF_FAIL(v2f_compressor_compress_block(
                            &compressor, samples, sample_count, output_buffer,
                            &written_byte_count));
                    CU_ASSERT_EQUAL_FATAL(
                            sample_count * bytes_per_word, written_byte_count);

                    // Build decompressor and decompress
                    v2f_decompressor_t decompressor;
                    FAIL_IF_FAIL(
                            v2f_decompressor_create(&decompressor, &quantizer,
                                                    &decorrelator,
                                                    &entropy_decoder));
                    uint64_t written_sample_count;
                    v2f_decompressor_decompress_block(
                            &decompressor, output_buffer, written_byte_count,
                            sample_count, reconstructed_samples,
                            &written_sample_count);
                    CU_ASSERT_EQUAL_FATAL(written_sample_count, sample_count);

                    // Compare original and reconstructed samples
                    for (uint64_t i = 0; i < sample_count; i++) {
                        CU_ASSERT_FATAL(original_samples[i] <= INT32_MAX);
                        CU_ASSERT_FATAL(reconstructed_samples[i] <= INT32_MAX);
                        const int64_t error = abs(
                                (int32_t) original_samples[i]
                                - (int32_t) reconstructed_samples[i]);
                        if (qstep == 1 ||
                            quantizer_mode == V2F_C_QUANTIZER_MODE_NONE) {
                                CU_ASSERT_EQUAL_FATAL(error, 0);
                        } else {
                                CU_ASSERT_FATAL(error <= ((qstep / 2) + 1));
                        }
                    }

                    // Clean up
                    free(original_samples);
                    free(samples);
                    free(reconstructed_samples);
                    free(output_buffer);
                    v2f_build_destroy_minimal_forest(&entropy_coder,
                                                     &entropy_decoder);
                }
            }
        }
    }
}

void test_compression_decompression_minimal_codec(void) {
    {
        v2f_compressor_t test_compressor;
        v2f_decompressor_t test_decompressor;
        CU_ASSERT_EQUAL_FATAL(
                v2f_build_minimal_codec(1, NULL, NULL),
                V2F_E_INVALID_PARAMETER);
        CU_ASSERT_EQUAL_FATAL(
                v2f_build_minimal_codec(1, &test_compressor, NULL),
                V2F_E_INVALID_PARAMETER);
        CU_ASSERT_EQUAL_FATAL(
                v2f_build_minimal_codec(1, NULL, &test_decompressor),
                V2F_E_INVALID_PARAMETER);
    }

    timer_reset();

    v2f_compressor_t compressor;
    v2f_decompressor_t decompressor;

    for (uint8_t bytes_per_word = V2F_C_MIN_BYTES_PER_WORD;
         bytes_per_word <= V2F_C_MAX_BYTES_PER_WORD;
         bytes_per_word++) {

        const v2f_sample_t max_sample_value =
                (v2f_sample_t) ((1 << (8 * bytes_per_word)) - 1);

        // Allocate and initialize entries
        const uint32_t entry_count = 1024;
        const uint64_t repetition_count = 32;
        CU_ASSERT_FATAL(entry_count - 1 <= UINT16_MAX);
        const uint64_t sample_count =
                repetition_count * entry_count;
        v2f_sample_t *original_samples = malloc(
                sizeof(v2f_sample_t) * sample_count);
        v2f_sample_t *samples = malloc(
                sizeof(v2f_sample_t) * sample_count);
        v2f_sample_t *reconstructed_samples = malloc(
                sizeof(v2f_sample_t) * sample_count);
        CU_ASSERT_NOT_EQUAL_FATAL(original_samples, NULL);
        CU_ASSERT_NOT_EQUAL_FATAL(samples, NULL);
        CU_ASSERT_NOT_EQUAL_FATAL(reconstructed_samples, NULL);
        for (uint64_t i = 0; i < repetition_count; i++) {
            for (uint64_t j = 0; j < entry_count; j++) {
                samples[i * entry_count + j] =
                        (v2f_sample_t) (j %
                                        (max_sample_value + 1));
                original_samples[i * entry_count + j] = samples[
                        i * entry_count + j];
            }
        }
        uint8_t *output_buffer = malloc(
                sizeof(uint8_t) * bytes_per_word * sample_count);
        CU_ASSERT_NOT_EQUAL_FATAL(output_buffer, NULL);
        uint64_t written_byte_count;

        // Build minimal compressor/decompressor
        FAIL_IF_FAIL(v2f_build_minimal_codec(
                bytes_per_word, &compressor, &decompressor));

        // Compress and decompress
        FAIL_IF_FAIL(v2f_compressor_compress_block(
                &compressor, samples, sample_count, output_buffer,
                &written_byte_count));
        CU_ASSERT_EQUAL_FATAL(
                sample_count * bytes_per_word, written_byte_count);

        uint64_t written_sample_count;
        v2f_decompressor_decompress_block(
                &decompressor, output_buffer, written_byte_count,
                sample_count, reconstructed_samples,
                &written_sample_count);
        CU_ASSERT_EQUAL_FATAL(written_sample_count, sample_count);

        // Compare original and reconstructed samples
        for (uint64_t i = 0; i < sample_count; i++) {
            CU_ASSERT_FATAL(original_samples[i] <= INT32_MAX);
            CU_ASSERT_FATAL(reconstructed_samples[i] <= INT32_MAX);
            CU_ASSERT_EQUAL_FATAL(original_samples[i],
                                  reconstructed_samples[i]);
        }

        // Clean up
        free(original_samples);
        free(samples);
        free(reconstructed_samples);
        free(output_buffer);
        v2f_build_destroy_minimal_codec(&compressor, &decompressor);
    }
}


CU_START_REGISTRATION(compressor_decompressor)
    CU_QADD_TEST(test_compressor_decompressor_create_destroy)
    CU_QADD_TEST(test_compression_decompression_steps)
    CU_QADD_TEST(test_compression_decompression_minimal_codec)
CU_END_REGISTRATION()
