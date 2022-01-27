/**
 * @file
 *
 * @brief Main interface to the compression routines.
 */

#include <assert.h>
#include <getopt.h>
#include <stdio.h>

#include "../src/v2f.h"
#include "../src/log.h"
#include "../src/timer.h"

#include "bin_common.h"
#include "v2f_compress_usage.h"

/**
 * Entropy point to the compressor.
 *
 * @param argc number of command line arguments.
 * @param argv command line arguments.
 *
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
    bool time_file_set = false;
    char *time_file_path = NULL;

    // Optional argument parsing
    int opt;
    while ((opt = getopt(argc, argv, "q:s:d:t:hv")) != -1) {
        switch (opt) {
            case 'q':
                if (quantizer_mode_set) {
                    log_warning("Found repeated parameter q. Last value will prevail.");
                }
                if (parse_positive_integer(
                        optarg, &quantizer_mode, "quantizer_mode") != 0
                    || quantizer_mode > V2F_C_QUANTIZER_MODE_UNIFORM) {
                    fprintf(stderr,
                            "Invalid quantizer mode. Invoke with -h for help.\n");
                    return 1;
                }
                log_info("Found parameter q = %d", (int) quantizer_mode);
                quantizer_mode_set = true;
                break;

            case 's':
                if (step_size_set) {
                    log_warning("Found repeated parameter s. Last value will prevail.");
                }
                if (parse_positive_integer(
                        optarg, &step_size, "step_size") != 0
                    || step_size > V2F_C_QUANTIZER_MODE_MAX_STEP_SIZE) {
                    fprintf(stderr,
                            "Invalid step size. Invoke with -h for help.\n");
                    return 1;
                }
                step_size_set = true;
                break;

            case 'd':
                if (decorrelator_mode_set) {
                    log_warning("Found repeated parameter d. Last value will prevail.");
                }
                if (parse_positive_integer(
                        optarg, &decorrelator_mode, "decorrelator_mode") != 0
                    || decorrelator_mode >= V2F_C_DECORRELATOR_MODE_COUNT) {
                    fprintf(stderr,
                            "Invalid decorrelator mode. Invoke with -h for help.\n");
                    return 1;
                }
                decorrelator_mode_set = true;
                break;

            case 't':
                if (time_file_set) {
                    log_warning("Found repeated parameter t. Last value will prevail.");
                }
                time_file_path = optarg;
                time_file_set = true;
                break;

            case 'h':
                show_banner();
                puts(show_usage_string);
                return 64;
            case 'v':
                printf("Using %s version %s", argv[0], PROJECT_VERSION);
                return 64;
            case '?':
                fprintf(stderr,
                        "Invalid option: -%c. Invoke with -h for help.\n",
                        optopt);
                return 1;
            default: // LCOV_EXCL_LINE
                assert(false); // LCOV_EXCL_LINE
        }
    }

    // Mandatory argument
    if (optind + 3 != argc) {
        fprintf(stderr,
                "Invalid number of parameters. Invoke with -h for help.\n");
        return 1;
    }

    // File paths
    char const *const raw_file_path = argv[optind];
    char const *const header_file_path = argv[optind + 1];
    char const *const output_file_path = argv[optind + 2];

    // Perform compression
    const int status = v2f_file_compress_from_path(
            raw_file_path, header_file_path, output_file_path,
            quantizer_mode_set, quantizer_mode,
            step_size_set, step_size,
            decorrelator_mode_set, decorrelator_mode);

    // Report results
    log_info("Compression of %s completed with status %d.",
             raw_file_path, status);
    if (time_file_set) {
        FILE* time_file = fopen(time_file_path, "w");
        if (time_file == NULL) {
            log_error("Error: could not open file '%s' to store time information. "
                      "This did not affect the compression status.",
                      time_file_path);
        } else {
            timer_report_csv(time_file);
            log_info("Saved time information at '%s'", time_file_path);
        }
    } else {
        if (_LOG_LEVEL >= LOG_INFO_LEVEL) {
            timer_report_human(stdout);
        }
    }

    return status;
}
