/**
 * @file
 *
 * @brief Main interface to the compression routines.
 */

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/v2f.h"
#include "../src/log.h"
#include "../src/timer.h"

#include "bin_common.h"
#include "../src/common.h"
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
    bool samples_per_row_set = false;
    v2f_sample_t samples_per_row = 0;
    bool shadow_list_set = false;
    uint32_t *shadow_y_positions = NULL;
    uint32_t y_shadow_count = 0;
    bool time_file_set = false;
    char *time_file_path = NULL;

    // Optional argument parsing
    int opt;
    while ((opt = getopt(argc, argv, "q:s:d:t:w:y:hv")) != -1) {
        switch (opt) {
            case 'q':
                if (quantizer_mode_set) {
                    log_warning("Found repeated parameter q. Last value will prevail.");
                }
                if (parse_positive_integer(
                        optarg, &quantizer_mode, "quantizer_mode") != 0
                    || quantizer_mode >= V2F_C_QUANTIZER_MODE_COUNT) {
                    fprintf(stderr,
                            "Invalid quantizer mode. Invoke with -h for help.\n");
                    if (shadow_y_positions != NULL) {
                        free(shadow_y_positions);
                    }
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
                    if (shadow_y_positions != NULL) {
                        free(shadow_y_positions);
                    }
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
                    if (shadow_y_positions != NULL) {
                        free(shadow_y_positions);
                    }
                    return 1;
                }
                if (decorrelator_mode_set) {
                    fprintf(stderr, "The decorrelator mode can only be specified once.\n");
                    return 1;
                }
                decorrelator_mode_set = true;
                break;

            case 'w':
                if (samples_per_row_set) {
                    log_warning("Found repeated parameter w. Last value will prevail.");
                }
                if (parse_positive_integer(
                        optarg, &samples_per_row, "samples_per_row") != 0) {
                    fprintf(stderr,
                            "Invalid number of samples per row. Invoke with -h for help.\n");
                    if (shadow_y_positions != NULL) {
                        free(shadow_y_positions);
                    }
                    return 1;
                }
                samples_per_row_set = true;
                break;

            case 'y':
                if (shadow_list_set) {
                    log_warning("Found repeated parameter y. Last value will prevail.");
                    free(shadow_y_positions);
                    shadow_y_positions = NULL;
                }
                if (!samples_per_row_set || samples_per_row == 0) {
                    fprintf(stderr, "The -w argument must be provided before -y, "
                                    "and a non-zero number of samples per row must be specified.\n");
                    return 1;
                }
                if (parse_positive_integer_list(optarg, &shadow_y_positions, &y_shadow_count) !=
                    0) {
                    fprintf(stderr, "Could not parse y argument '%s'. "
                                    "It must be a comma-separated list of positive integers.\n",
                            optarg);
                    return 1;
                }
                if (y_shadow_count % 2 != 0) {
                    fprintf(stderr, "The -y argument accepts only an even number of integers.\n");
                    free(shadow_y_positions);
                    return 1;
                }
                y_shadow_count /= 2;
                for (uint32_t i = 0; i < y_shadow_count * 2 - 1; i++) {
                    if (shadow_y_positions[i] > shadow_y_positions[i + 1]) {
                        fprintf(stderr,
                                "The -y argument accepts only a non-decreasing list of integers.\n");
                        free(shadow_y_positions);
                        return 1;
                    }
                    if (i < y_shadow_count * 2 - 2 && i % 2 == 1
                        && shadow_y_positions[i] >= shadow_y_positions[i + 1]) {
                        fprintf(stderr,
                                "The -y argument does not accept overlapping shadow regions.\n");
                        free(shadow_y_positions);
                        return 1;
                    }
                }
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
                if (shadow_y_positions != NULL) {
                    free(shadow_y_positions);
                }
                return 64;
            case 'v':
                printf("Using %s version %s", argv[0], PROJECT_VERSION);
                if (shadow_y_positions != NULL) {
                    free(shadow_y_positions);
                }
                return 64;
            case '?':
                fprintf(stderr,
                        "Invalid option: -%c. Invoke with -h for help.\n",
                        optopt);
                if (shadow_y_positions != NULL) {
                    free(shadow_y_positions);
                }
                return 1;
            default: // LCOV_EXCL_LINE
                assert(false); // LCOV_EXCL_LINE
        }
    }

    if ((decorrelator_mode == V2F_C_DECORRELATOR_MODE_JPEG_LS
         || decorrelator_mode == V2F_C_DECORRELATOR_MODE_FGIJ)
        && !samples_per_row_set) {
        fprintf(stderr, "Error! The selected decorrelator mode requires "
                        "the -w parameter to be specified. Invoke with -h for help.");
        if (shadow_y_positions != NULL) {
            free(shadow_y_positions);
        }
        return 1;
    }

    // Mandatory argument
    if (optind + 3 != argc) {
        fprintf(stderr,
                "Invalid number of parameters. Invoke with -h for help.\n");
        if (shadow_y_positions != NULL) {
            free(shadow_y_positions);
        }
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
            decorrelator_mode_set, decorrelator_mode, samples_per_row,
            shadow_y_positions, y_shadow_count);

    // Report results
    log_info("Compression of %s completed with status %d.",
             raw_file_path, status);
    if (time_file_set) {
        FILE *time_file = fopen(time_file_path, "w");
        if (time_file == NULL) {
            log_error("Error: could not open file '%s' to store time information. "
                      "This did not affect the compression status.",
                      time_file_path);
        } else {
            timer_report_csv(time_file);
            log_info("Saved time information at '%s'", time_file_path);
            fclose(time_file);
        }
    } else {
        if (_LOG_LEVEL >= LOG_INFO_LEVEL) {
            timer_report_human(stdout);
        }
    }

    if (shadow_y_positions != NULL) {
        free(shadow_y_positions);
    }

    return status;
}
