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
#include "../src/v2f_file.h"
#include "../src/v2f_build.h"
#include "test_samples.h"

/// Exercise file I/O
void test_sample_io(void);

/**
 * Test that entropy coders/decoders can be properly dumped and loaded
 */
void test_minimal_forest_dump(void);

/**
 * Test that compressor/decompressor pairs (codecs) can be properly dumped and loaded
 */
void test_minimal_codec_dump(void);

void test_sample_io(void) {
    for (uint32_t i = 0; i < V2F_C_TEST_SAMPLE_COUNT; i++) {
        v2f_test_sample_t *sample_info = &(all_test_samples[i]);

        for (uint8_t bytes_per_sample = V2F_C_MIN_BYTES_PER_SAMPLE;
             bytes_per_sample <= V2F_C_MAX_BYTES_PER_SAMPLE;
             bytes_per_sample++) {

            for (uint32_t block_size = V2F_C_MIN_BLOCK_SIZE;
                 block_size <= V2F_C_MAX_BLOCK_SIZE;
                 block_size += V2F_C_MAX_BLOCK_SIZE - V2F_C_MIN_BLOCK_SIZE) {

                uint64_t remaining_samples =
                        sample_info->bytes / bytes_per_sample;
                if (remaining_samples == 0) {
                    continue; // LCOV_EXCL_LINE
                }

                FILE *input_file = fopen(sample_info->path, "r");
                CU_ASSERT_NOT_EQUAL_FATAL(input_file, NULL);

                FILE *copy = tmpfile();
                CU_ASSERT_NOT_EQUAL_FATAL(copy, NULL);
                copy_file(input_file, copy);
                CU_ASSERT_EQUAL_FATAL(fseeko(copy, SEEK_SET, 0), 0);

                FILE *output_file = tmpfile();
                CU_ASSERT_NOT_EQUAL_FATAL(output_file, NULL);

                v2f_sample_t *sample_buffer =
                        malloc(sizeof(v2f_sample_t) * remaining_samples);

                while (remaining_samples > 0) {
                    uint64_t effective_block_size =
                            remaining_samples >= block_size ?
                            block_size : remaining_samples;

                    FAIL_IF_FAIL(v2f_file_read_big_endian(
                            copy, sample_buffer, effective_block_size,
                            bytes_per_sample, NULL));

                    remaining_samples -= effective_block_size;

                    FAIL_IF_FAIL(v2f_file_write_big_endian(
                            output_file, sample_buffer, effective_block_size,
                            bytes_per_sample));
                }

                fclose(input_file);
                fclose(output_file);
                fclose(copy);
                free(sample_buffer);
            }
        }
    }
}

void test_minimal_forest_dump(void) {
    v2f_entropy_coder_t coder;
    v2f_entropy_decoder_t decoder;

    for (uint8_t bytes_per_index = V2F_C_MIN_BYTES_PER_WORD;
         bytes_per_index <= V2F_C_MAX_BYTES_PER_WORD;
        bytes_per_index++) {
        
        FAIL_IF_FAIL(
                v2f_build_minimal_forest(bytes_per_index, &coder, &decoder));

        FILE* tmp = tmpfile();
        CU_ASSERT_NOT_EQUAL_FATAL(tmp, NULL);

        FAIL_IF_FAIL(v2f_file_write_forest(tmp, &coder, &decoder, 1));

        FAIL_IF_FAIL(v2f_build_destroy_minimal_forest(&coder, &decoder));

        v2f_entropy_coder_t new_coder;
        v2f_entropy_decoder_t new_decoder;

        CU_ASSERT_EQUAL_FATAL(fseeko(tmp, SEEK_SET, 0), 0);
        FAIL_IF_FAIL(v2f_file_read_forest(tmp, &new_coder, &new_decoder));
        FAIL_IF_FAIL(v2f_file_destroy_read_forest(&new_coder, &new_decoder));

        fclose(tmp);
    }
}

void test_minimal_codec_dump(void) {
    v2f_compressor_t compressor1;
    v2f_decompressor_t decompressor1;
    v2f_compressor_t compressor2;
    v2f_decompressor_t decompressor2;

    for (uint8_t bytes_per_word = V2F_C_MIN_BYTES_PER_WORD;
        bytes_per_word <= V2F_C_MAX_BYTES_PER_WORD;
        bytes_per_word++) {

        FAIL_IF_FAIL(v2f_build_minimal_codec(
                bytes_per_word, &compressor1, &decompressor1));

        FILE* output_file = tmpfile();
        CU_ASSERT_PTR_NOT_NULL(output_file);
        FAIL_IF_FAIL(v2f_file_write_codec(output_file, &compressor1, &decompressor1));
        fflush(output_file);

        CU_ASSERT_EQUAL_FATAL(fseeko(output_file, SEEK_SET, 0), 0);
        FAIL_IF_FAIL(v2f_file_read_codec(output_file, &compressor2, &decompressor2));

        FAIL_IF_FAIL(v2f_build_destroy_minimal_codec(&compressor1, &decompressor1));
        FAIL_IF_FAIL(v2f_file_destroy_read_codec(&compressor2, &decompressor2));

        fclose(output_file);
    }
}

CU_START_REGISTRATION(file)
    CU_QADD_TEST(test_sample_io)
    CU_QADD_TEST(test_minimal_forest_dump)
    CU_QADD_TEST(test_minimal_codec_dump)
CU_END_REGISTRATION()
