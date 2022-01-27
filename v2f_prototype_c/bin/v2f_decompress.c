/**
 * @file
 *
 * @brief Main interface to the decompression routines.
 */

#include <assert.h>
#include <getopt.h>
#include <stdio.h>

#include "../src/v2f.h"
#include "../src/log.h"

#include "bin_common.h"
#include "v2f_decompress_usage.h"

/**
 * Entropy point to the decompressor.
 *
 * @param argc number of command line arguments.
 * @param argv command line arguments.
 * @return 0 when successful, a different value otherwise.
 */
int main(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    // Default argument values
    bool quantizer_mode_set = false;
    v2f_quantizer_mode_t quantizer_mode = V2F_C_QUANTIZER_MODE_NONE;
    bool step_size_set = false;
    v2f_sample_t step_size = 1;
    bool decorrelator_mode_set = false;
    v2f_decorrelator_mode_t decorrelator_mode = V2F_C_DECORRELATOR_MODE_LEFT;

    // Optional argument parsing
    int opt;
    while ((opt = getopt(argc, argv, "q:s:d:hv")) != -1) {
        switch (opt) {
            case 'q':
                if (parse_positive_integer(
                        optarg, &quantizer_mode, "quantizer_mode") != 0
                    || quantizer_mode > V2F_C_QUANTIZER_MODE_UNIFORM) {
                    fprintf(stderr, "Invalid quantizer mode. Invoke with -h for help.\n");
                    return 1;
                }
                quantizer_mode_set = true;
                break;

            case 's':
                if (parse_positive_integer(
                        optarg, &step_size, "step_size") != 0
                    || step_size > V2F_C_QUANTIZER_MODE_MAX_STEP_SIZE) {
                    fprintf(stderr, "Invalid step size. Invoke with -h for help.\n");
                    return 1;
                }
                step_size_set = true;
                break;

            case 'd':
                if (parse_positive_integer(
                        optarg, &decorrelator_mode, "decorrelator_mode") != 0
                    || decorrelator_mode >= V2F_C_DECORRELATOR_MODE_COUNT) {
                    fprintf(stderr, "Invalid decorrelator mode. Invoke with -h for help.\n");
                    return 1;
                }
                decorrelator_mode_set = true;
                break;

            case 'h':
                show_banner();
                puts(show_usage_string);
                return 64;
            case 'v':
                show_banner();
                printf("Using %s version %s", argv[0], PROJECT_VERSION);
                return 64;
            case '?':
                fprintf(stderr, "Invalid option: -%c. Invoke with -h for help.\n", optopt);
                return 1;
            default: // LCOV_EXCL_LINE
                assert(false); // LCOV_EXCL_LINE
        }
    }

    // Mandatory argument
    if (optind + 3 != argc) {
        fprintf(stderr, "Invalid number of parameters. Invoke with -h for help.\n");
        return 1;
    }

    // File paths
    char const *const compressed_file_path = argv[optind];
    char const *const header_file_path = argv[optind+1];
    char const *const reconstructed_file_path = argv[optind + 2];

    int status = v2f_file_decompress_from_path(
            compressed_file_path, header_file_path, reconstructed_file_path,
            quantizer_mode_set, quantizer_mode,
            step_size_set, step_size,
            decorrelator_mode_set, decorrelator_mode);

    log_info("Decompression completed with status %d.", status);

    return status;
}