/**
 * @file
 *
 * Fuzzer harness that exercises the entropy decoding stage.
 */

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "../src/v2f.h"
#include "../src/v2f_build.h"
#include "../src/v2f_file.h"
#include "../src/common.h"
#include "fuzzing_common.h"

/**
 * Run a single decoding instance from input
 *
 * @param input file open for reading with compressed data to be decoded
 * @param sample_count number of samples to be decoded
 * @param bytes_per_word number of bytes per compressed codeword
 * @param sample_buffer pre-allocated buffer to store the decodes samples
 * @param compressed_buffer buffer of compressed codewords to be decoded
 */
void run_one_case(
        FILE *input,
        uint32_t sample_count,
        uint8_t bytes_per_word,
        v2f_sample_t *const sample_buffer,
        uint8_t *const compressed_buffer);

/**
 * Entropy point to the fuzzer's harness.
 * @param argc number of command line arguments
 * @param argv command line arguments
 * @return status code
 */
int main(int argc, char *argv[]);

void run_one_case(
        FILE *input,
        uint32_t sample_count,
        uint8_t bytes_per_word,
        v2f_sample_t *const sample_buffer,
        uint8_t *const compressed_buffer) {
    v2f_entropy_coder_t coder;
    v2f_entropy_decoder_t decoder;
    ABORT_IF_FAIL(v2f_build_minimal_forest(bytes_per_word, &coder, &decoder));

    uint32_t remaining_samples = sample_count;
    while (remaining_samples > 0) {
        uint32_t effective_block_size =
                remaining_samples >= V2F_C_MAX_BLOCK_SIZE ?
                V2F_C_MAX_BLOCK_SIZE : remaining_samples;

        if (fread(compressed_buffer, bytes_per_word, effective_block_size, input)
            != effective_block_size) {
            break;
        }

        uint64_t written_sample_count;
        ABORT_IF_FAIL(v2f_entropy_decoder_decompress_block(
                &decoder, compressed_buffer,
                effective_block_size * bytes_per_word,
                sample_buffer,
                sample_count,
                &written_sample_count));

        remaining_samples -= effective_block_size;
    }

    ABORT_IF_FAIL(v2f_build_destroy_minimal_forest(&coder, &decoder));
}

int main(int argc, char *argv[]) {
    V2F_SILENCE_UNUSED(argc);
    V2F_SILENCE_UNUSED(argv);

    setvbuf(stdin, NULL, _IONBF, 0);
    clearerr(stdin);

    const uint32_t max_sample_count = V2F_C_MAX_BLOCK_SIZE * 5;

    uint32_t sample_count;
    uint8_t bytes_per_word;
    if (fuzzing_get_samples_and_bytes_per_sample(
            stdin, &sample_count, &bytes_per_word) != V2F_E_NONE) {
        return 1;
    }
    if (bytes_per_word < V2F_C_MIN_BYTES_PER_WORD || bytes_per_word > V2F_C_MAX_BYTES_PER_WORD) {
        return 1;
    }
    if (sample_count > max_sample_count) {
        return 1;
    }

    FILE *input;
    ABORT_IF_FAIL(v2f_fuzzing_assert_temp_file_created(&input));
    copy_file(stdin, input);

    v2f_sample_t *sample_buffer = malloc(
            sizeof(v2f_sample_t) * V2F_C_MAX_BLOCK_SIZE);
    uint8_t *output_buffer = malloc(
            sizeof(uint8_t) * V2F_C_MAX_BLOCK_SIZE * bytes_per_word);
    if (output_buffer == NULL || sample_buffer == NULL) {
        if (sample_buffer) {
            free(sample_buffer);
        }
        if (output_buffer) {
            free(output_buffer);
        }
        return 1;
    }

    while (__AFL_LOOP(10000)) {
        assert(fseeko(input, SEEK_SET, 0) == 0);
        run_one_case(
                input, sample_count, bytes_per_word,
                sample_buffer, output_buffer);
    }

    fclose(input);
    free(sample_buffer);
    free(output_buffer);

    return 0;
}
