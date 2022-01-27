/**
 * @file
 *
 * Fuzzer harness that exercises the entropy coding stage.
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "../src/v2f.h"
#include "../src/v2f_build.h"
#include "../src/v2f_file.h"
#include "../src/common.h"
#include "fuzzing_common.h"

/**
 * Run a single decoding instance from samples_file with a minimal entropy coder.
 *
 * @param samples_file file open for reading with the data to be compressed
 * @param sample_count number of samples to be read
 * @param bytes_per_sample number of bytes per samples
 * @param input_samples pre-allocated array of samples_file samples
 * @param compressed_bytes pre-allocated array of compressed bytes
 * @param reconstructed_samples pre-allocated array of reconstructed samples.
 */
void run_one_case(FILE *samples_file, uint32_t sample_count, uint8_t bytes_per_sample,
                  v2f_sample_t *const input_samples,
                  uint8_t *const compressed_bytes,
                  v2f_sample_t *const reconstructed_samples);


/**
 * Entry point for the fuzzer harness.
 * @param argc number of command line arguments
 * @param argv command line arguments
 * @return the return code
 */
int main(int argc, char *argv[]);

void run_one_case(FILE *input, uint32_t sample_count, uint8_t bytes_per_sample,
                  v2f_sample_t *const input_samples,
                  uint8_t *const compressed_bytes,
                  v2f_sample_t *const reconstructed_samples) {
    v2f_entropy_coder_t coder;
    v2f_entropy_decoder_t decoder;
    ABORT_IF_FAIL(v2f_build_minimal_forest(bytes_per_sample, &coder, &decoder));

    assert(bytes_per_sample >= 1);
    assert(bytes_per_sample <= 2);
    const v2f_sample_t max_sample_value = (v2f_sample_t) ((UINT64_C(1) << (8*bytes_per_sample)) - 1);

    uint32_t remaining_samples = sample_count;
    while (remaining_samples > 0) {
        uint32_t effective_block_size =
                remaining_samples >= V2F_C_MAX_BLOCK_SIZE ?
                V2F_C_MAX_BLOCK_SIZE : remaining_samples;

        uint64_t read_count;
        if (v2f_file_read_big_endian(
                input, input_samples, effective_block_size, bytes_per_sample,
                &read_count) != V2F_E_NONE) {
            break;
        }
        if (read_count != effective_block_size) {
            break;
        }

        uint64_t written_byte_count;
        ABORT_IF_FAIL(v2f_entropy_coder_compress_block(
                &coder, input_samples, effective_block_size, compressed_bytes,
                &written_byte_count));

        uint64_t written_sample_count;
        ABORT_IF_FAIL(v2f_entropy_decoder_decompress_block(
                &decoder, compressed_bytes, written_byte_count,
                reconstructed_samples, max_sample_value, &written_sample_count));

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
    uint8_t bytes_per_sample;
    if (fuzzing_get_samples_and_bytes_per_sample(
            stdin, &sample_count, &bytes_per_sample) != V2F_E_NONE) {
        return 1;
    }
    if (bytes_per_sample < V2F_C_MIN_BYTES_PER_SAMPLE
        || bytes_per_sample > V2F_C_MAX_BYTES_PER_SAMPLE) {
        return 1;
    }
    if (sample_count > max_sample_count) {
        return 1;
    }

    FILE *input;
    ABORT_IF_FAIL(v2f_fuzzing_assert_temp_file_created(&input));
    copy_file(stdin, input);

    uint8_t *byte_buffer = malloc(
            sizeof(uint8_t) * V2F_C_MAX_BLOCK_SIZE * bytes_per_sample);
    v2f_sample_t *input_sample_buffer = malloc(
            sizeof(v2f_sample_t) * V2F_C_MAX_BLOCK_SIZE);
    v2f_sample_t *reconstructed_sample_buffer = malloc(
            sizeof(v2f_sample_t) * V2F_C_MAX_BLOCK_SIZE);

    if (byte_buffer == NULL
        || input_sample_buffer == NULL
        || reconstructed_sample_buffer == NULL) {
        if (input_sample_buffer) {
            free(input_sample_buffer);
        }
        if (reconstructed_sample_buffer) {
            free(reconstructed_sample_buffer);
        }
        if (byte_buffer) {
            free(byte_buffer);
        }
        return 1;
    }

    while (__AFL_LOOP(10000)) {
        assert(fseeko(input, SEEK_SET, 0) == 0);
        run_one_case(
                input, sample_count, bytes_per_sample,
                input_sample_buffer, byte_buffer, reconstructed_sample_buffer);
    }

    fclose(input);
    free(input_sample_buffer);
    free(reconstructed_sample_buffer);
    free(byte_buffer);

    return 0;
}
