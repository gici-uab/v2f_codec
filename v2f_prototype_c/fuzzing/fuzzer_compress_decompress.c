/**
 * @file
 *
 * This fuzzer:
 *      1. Reads an input file, codec definition and other function parameters.
 *      2. Attempts compression.
 *      3. Upon successful compression, decompression is attempted.
 *      4. If lossless reconstruction is expected, it is asserted.
 *
 * The expected input format is:
 *      1. Number of bytes in the sample file: 4 bytes, unsigned big endian
 *      2. Number of chars of the header filename: 2 bytes, unsigned big endian
 *      3. Header name string of exactly that length, without trailing '\0'
 *      4. Samples: with length in bytes as defined in 1
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "../src/v2f.h"
#include "../src/v2f_file.h"
#include "../src/common.h"
#include "../src/log.h"
#include "fuzzing_common.h"

/// Minimum length allowed in header sample paths
#define MIN_HEADER_NAME_SIZE 6

/**
 * Run a single decoding instance from samples_file with a minimal entropy coder.
 *
 * @param samples_file file open for reading with samples to be compressed
 * @param header_file file open for reading with the V2F codec definition to use
 * @param compressed_file file open for reading and writing where compressed
 *   data are stored and then read for decompression.
 * @param reconstructed_file file open for reading and writing where the reconstructed
 *   data are stored.
 */
void run_one_case(FILE *samples_file, FILE *header_file,
                  FILE *compressed_file, FILE *reconstructed_file);

/**
 * Determine whether a given path is a directory. We don't want to open those.
 * @param path path to be verified
 * @return true if and only if the path points to a directory
 */
static bool is_regular_file(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return false;
    }
    return S_ISREG(statbuf.st_mode) != 0;
}

/**
 * Entry point for the fuzzer harness.
 * @param argc number of command line arguments
 * @param argv command line arguments
 * @return the return code
 */
int main(int argc, char *argv[]);

void run_one_case(FILE *samples_file, FILE *header_file,
                  FILE *compressed_file, FILE *reconstructed_file) {
    // Compress
    if (v2f_file_compress_from_file(samples_file, header_file, compressed_file,
                                    false, 0, false, 0, false, 0, 1, NULL, 0)
        != V2F_E_NONE) {
        log_info("Error compressing with the input data. That's fine.");
        return;
    }
    if (fseeko(header_file, 0, SEEK_SET) != 0) {
        log_info("I/O Error. That's fine here.");
        return;
    }
    if (fseeko(compressed_file, 0, SEEK_SET) != 0) {
        log_info("I/O Error. That's fine here.");
        return;
    }
    log_info("Successfully compressed with input data");

    // Decompress
    if (v2f_file_decompress_from_file(
            compressed_file, header_file, reconstructed_file,
            false, 0, false, 0, false, 0, 1) != 0) {
        log_error("Error decompressing. It should not have failed.");
        abort();
    }
    log_info("Successfully decompressed.");

    if (fseeko(samples_file, 0, SEEK_SET) != 0) {
        log_info("I/O error.");
        return;
    }
    if (fseeko(reconstructed_file, 0, SEEK_SET) != 0) {
        log_info("I/O error.");
        return;
    }

    const bool lossless_reconstruction = fuzzing_check_files_are_equal(
            samples_file, reconstructed_file);

    if (fseeko(header_file, 0, SEEK_SET) != 0) {
        log_info("I/O Error. That's fine here.");
        return;
    }
    v2f_compressor_t compressor;
    v2f_decompressor_t decompressor;
    v2f_file_read_codec(header_file, &compressor, &decompressor);
    
    if (compressor.quantizer->step_size == 1
        || compressor.quantizer->mode == V2F_C_QUANTIZER_MODE_NONE) {
        if (!lossless_reconstruction) {
            log_error(
                    "Expected lossless reconstruction but it did not happen!");
            abort();
        } else {
            log_info("Lossless reconstruction, as expected.");
        }
    } else {
        log_info("Lossless reconstruction: %d. Was not guaranteed.",
                 lossless_reconstruction);
    }
    v2f_file_destroy_read_codec(&compressor, &decompressor);
}

int main(int argc, char *argv[]) {
    V2F_SILENCE_UNUSED(argc);
    V2F_SILENCE_UNUSED(argv);

    setvbuf(stdin, NULL, _IONBF, 0);
    clearerr(stdin);

    FILE *samples_file;
    ABORT_IF_FAIL(v2f_fuzzing_assert_temp_file_created(&samples_file));

    FILE *header_file;
    ABORT_IF_FAIL(v2f_fuzzing_assert_temp_file_created(&header_file));

    FILE *compressed_file;
    ABORT_IF_FAIL(v2f_fuzzing_assert_temp_file_created(&compressed_file));

    FILE *reconstructed_file;
    ABORT_IF_FAIL(v2f_fuzzing_assert_temp_file_created(&reconstructed_file));

    const uint32_t tmp_file_list_length = 4;
    FILE **tmp_file_list[4] = {
            &samples_file, &header_file, &compressed_file,
            &reconstructed_file};

    // Read stdin into the appropriate files
    // 1 - get sample file size in bytes (4 bytes)
    v2f_sample_t sample_file_size;
    if (v2f_file_read_big_endian(stdin, &sample_file_size, 1, 4, NULL)
        != V2F_E_NONE) {
        log_error("Error reading sample_file_size");
        return 1;
    }

    // 2 - get header name size in bytes (2 bytes)
    v2f_sample_t header_name_size;
    if (v2f_file_read_big_endian(stdin, &header_name_size, 1, 2, NULL)
        != V2F_E_NONE) {
        log_error("Error reading header_name_size");
        return 1;
    }
    if (header_name_size < MIN_HEADER_NAME_SIZE) {
        log_error("Found invalid header name size %u", header_name_size);
        return 1;
    }

    // 3 - figure out header path and read contents
    char header_name_buffer[UINT32_C(1) << (8 * 2)];
    if (fread(header_name_buffer, 1, header_name_size, stdin)
        != header_name_size) {
        log_error("Error reading header path");
        return 1;
    }
    header_name_buffer[header_name_size] = '\0';
    log_info("Vector requests header path '%s'", header_name_buffer);
    if (!is_regular_file(header_name_buffer)) {
        log_info(
                "Vector requested something different from a file to be opened. Refusing..");
        return 1;
    }

    FILE *original_header_file = fopen(header_name_buffer, "r");
    if (original_header_file == NULL) {
        log_info("Header path '%s' did not seem to exist", header_name_buffer);
        return 1;
    }
    copy_file(original_header_file, header_file);
    fclose(original_header_file);


    // 4 - read input samples
    {
        uint64_t remaining_size = sample_file_size;
        const uint64_t buffer_size = 1024 * 1024;
        uint8_t buffer[buffer_size];
        while (remaining_size > 0) {
            const size_t requested_size =
                    remaining_size < buffer_size ?
                    remaining_size : buffer_size;
            size_t read_size = fread(buffer, 1, requested_size, stdin);
            if (read_size <= 0) {
                break;
            }
            if (fwrite(buffer, 1, read_size, samples_file) != read_size) {
                log_error("I/O error when reading input samples");
                return 1;
            }
            assert(read_size <= remaining_size);
            remaining_size -= read_size;
        }
        if (remaining_size > 0) {
            log_error("I/O error when reading input samples");
            return 1;
        }
    }

    log_info("Loaded sample and header file name with sizes %u and %u, resp.",
             sample_file_size, header_name_size);

    while (__AFL_LOOP(10000)) {
        for (uint32_t i = 0; i < tmp_file_list_length; i++) {
            if (fseeko(*tmp_file_list[i], SEEK_SET, 0) != 0) {
                return 1;
            }
        }
        if (ftruncate(fileno(compressed_file), 0) != 0) {
            return 1;
        }
        if (ftruncate(fileno(reconstructed_file), 0) != 0) {
            return 1;
        }

        run_one_case(samples_file, header_file,
                     compressed_file, reconstructed_file);
    }

    for (uint32_t i = 0; i < tmp_file_list_length; i++) {
        fclose(*tmp_file_list[i]);
    }

    return 0;
}
