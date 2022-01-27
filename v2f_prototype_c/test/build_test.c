/**
 * @file
 *
 *
 */

#include <sys/types.h>
#include <stdio.h>
#include <assert.h>

#include "CUExtension.h"
#include "test_common.h"

// TODO: add tests

#include "../src/v2f_build.h"
#include "../src/v2f_entropy_coder.h"
#include "../fuzzing/fuzzing_common.h"
#include "../src/errors.h"
#include "../src/timer.h"
#include "../src/log.h"

/**
 * Test the minimal entropy coder and decoder produced by
 * @ref v2f_build_minimal_forest.
 */
void test_build_minimal_entropy(void);

/**
 * Test the minimal compressor/decompressor pair produced by
 * @ref v2f_build_minimal_codec.
 */
void test_build_minimal_codec(void);

void test_build_minimal_entropy(void) {
    v2f_entropy_coder_t coder;
    v2f_entropy_decoder_t decoder;

    for (uint8_t bytes_per_word = V2F_C_MINIMAL_MIN_BYTES_PER_WORD;
         bytes_per_word <= V2F_C_MINIMAL_MAX_BYTES_PER_WORD;
         bytes_per_word++) {
        const uint32_t symbol_count = (uint32_t) (1 << (8 * bytes_per_word));
        const uint32_t expected_entry_count = symbol_count;
        
        FAIL_IF_FAIL(v2f_build_minimal_forest(bytes_per_word, &coder, &decoder));

        // Evaluate basic coder health
        CU_ASSERT_EQUAL_FATAL(coder.bytes_per_word, bytes_per_word);
        CU_ASSERT_EQUAL_FATAL(coder.root_count, symbol_count);
        CU_ASSERT_EQUAL_FATAL(coder.roots[0]->children_count, expected_entry_count);

        // Evaluate basic decoder health
        CU_ASSERT_EQUAL_FATAL(decoder.bytes_per_word, bytes_per_word);
        CU_ASSERT_EQUAL_FATAL(decoder.roots[0]->root_entry_count, expected_entry_count);
        for (uint32_t index = 0; index  < expected_entry_count; index++) {
            CU_ASSERT_EQUAL_FATAL(decoder.roots[0]->entries_by_index[index].sample_count, 1);
        }

        ABORT_IF_FAIL(v2f_build_destroy_minimal_forest(&coder, &decoder));
    }
}

void test_build_minimal_codec(void) {
    v2f_compressor_t compressor;
    v2f_decompressor_t decompressor;

    for (uint8_t bytes_per_word = V2F_C_MINIMAL_MIN_BYTES_PER_WORD;
         bytes_per_word <= V2F_C_MINIMAL_MAX_BYTES_PER_WORD;
         bytes_per_word++) {
        const uint32_t symbol_count = (uint32_t) (1 << (8 * bytes_per_word));
        const uint32_t expected_entry_count = symbol_count;

        FAIL_IF_FAIL(v2f_build_minimal_codec(
                bytes_per_word, &compressor, &decompressor));

        // Evaluate basic coder health
        CU_ASSERT_EQUAL_FATAL(
                compressor.entropy_coder->bytes_per_word, bytes_per_word);
        CU_ASSERT_EQUAL_FATAL(
                compressor.entropy_coder->root_count, symbol_count);
        CU_ASSERT_EQUAL_FATAL(
                compressor.entropy_coder->roots[0]->children_count, expected_entry_count);

        // Evaluate basic decoder health
        CU_ASSERT_EQUAL_FATAL(
                decompressor.entropy_decoder->bytes_per_word, bytes_per_word);
        CU_ASSERT_EQUAL_FATAL(
                decompressor.entropy_decoder->roots[0]->root_entry_count, expected_entry_count);
        for (uint32_t index = 0; index  < expected_entry_count; index++) {
            CU_ASSERT_EQUAL_FATAL(
                    decompressor.entropy_decoder->roots[
                            0]->entries_by_index[index].sample_count, 1);
        }

        ABORT_IF_FAIL(v2f_build_destroy_minimal_codec(
                &compressor, &decompressor));
    }
}

CU_START_REGISTRATION(build)
    CU_QADD_TEST(test_build_minimal_entropy)
    CU_QADD_TEST(test_build_minimal_codec)
CU_END_REGISTRATION()
