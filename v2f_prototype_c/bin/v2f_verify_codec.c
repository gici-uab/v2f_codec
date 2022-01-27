/**
 * @file
 *
 * This binary loads a header file containing the description of a V2F
 * entropy forest: a coder and its corresponding entropy decoder.
 * When loaded, the validity of the V2F forest is verified, and
 * a small compression/decompression test its performed.
 */

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/log.h"
#include "../src/timer.h"
#include "../src/v2f_file.h"
#include "bin_common.h"
#include "v2f_verify_codec_usage.h" // May be automatically generated

/**
 * Print the help message for this tool.
 *
 * @param file the file where the message is to be printed
 */
static void print_help(FILE *file) {
    fputs(show_usage_string, file);
}

/**
 * Print information about a single coder tree node, recursively,
 * with visual indentation based on the selected level.
 *
 * @param file file where information is to be printed.
 * @param entropy_coder entropy coder to which the node belongs.
 * @param node node about which information is to be printed.
 * @param level visual indentation level to display a tree-like shape.
 * @param first_included_node if node is a root node, this value should
 *   be its index, since the i-th tree's root should not have defined
 *   output words for the i first symbols (all other nodes should).
 */
void print_coder_node_recursive(
        FILE *file,
        v2f_entropy_coder_t const *const entropy_coder,
        v2f_entropy_coder_entry_t *node,
        uint32_t level,
        uint32_t first_included_node);

void print_coder_node_recursive(
        FILE *file,
        v2f_entropy_coder_t const *const entropy_coder,
        v2f_entropy_coder_entry_t *node,
        uint32_t level,
        uint32_t first_included_node) {
    const uint32_t max_children_count = entropy_coder->max_expected_value + 1;
    const uint32_t max_indentation_length = 1024;
    char indentation_buffer[max_indentation_length + 1];
    const char indentation_char = ' ';
    const uint32_t chars_per_level = 4;
    const uint32_t indentation_length =
            level * chars_per_level <= max_indentation_length ?
            level * chars_per_level : max_indentation_length;
    memset(indentation_buffer, indentation_char, indentation_length);
    indentation_buffer[indentation_length] = '\0';
    fputs(indentation_buffer, file);

    if (node == NULL) {
        fprintf(file, " * (excluded branch in this tree state; "
                      "also excluded from the root's child count)\n");
    } else {
        fprintf(file, " + [%p:%u children] ", (void *) node, node->children_count);
        if (first_included_node == 0) {
            if (node->children_count == max_children_count) {
                fprintf(file, "(full, not included)\n");
            } else {
                fprintf(file, " included, word:");
                for (uint32_t i = 0; i < entropy_coder->bytes_per_word; i++) {
                    fprintf(file, " %x", node->word_bytes[i]);
                }
                fprintf(file, "\n");
            }
        } else {
            fprintf(file, "\n");
        }

        for (uint32_t i = 0; i < node->children_count; i++) {
            print_coder_node_recursive(file, entropy_coder,
                                       node->children_entries[i], level + 1,
                                       0);
        }
    }
    fflush(file);
}

int main(int argc, char *argv[]) {
    // Optional argument parsing
    int opt;
    while ((opt = getopt(argc, argv, "hv")) != -1) {
        switch (opt) {
            case 'h':
                show_banner();
                print_help(stderr);
                return 64;
            case 'v':
                show_banner();
                print_help(stderr);
                return 64;
            case '?':
                fprintf(stderr, "Invalid option: -%c\n", optopt);
                print_help(stderr);
                return 1;
            default: // LCOV_EXCL_LINE
                assert(false); // LCOV_EXCL_LINE
        }
    }

    if (optind + 1 != argc) {
        log_error("Invalid number of positional arguments");
        print_help(stderr);
        return 1;
    }

    char const *const input_path = argv[optind];
    FILE *input_file = fopen(input_path, "r");
    if (input_file == NULL) {
        log_error("Cannot open file = %s", input_path);
        return 1;
    }
    log_debug("input_path = %s", input_path);

    // Load entropy codec pair (includes validation)
    v2f_compressor_t compressor;
    v2f_decompressor_t decompressor;
    timer_start("V2F codec loading");
    v2f_error_t status = v2f_file_read_codec(
            input_file, &compressor, &decompressor);
    timer_stop("V2F codec loading");
    if (status != V2F_E_NONE) {
        log_error("Error loading codec from %s: %s",
                  input_path, v2f_strerror(status));
        return 1;
    }
    assert(compressor.decorrelator == decompressor.decorrelator);
    assert(compressor.quantizer == decompressor.quantizer);

    compressor.quantizer->mode = V2F_C_QUANTIZER_MODE_UNIFORM;
    compressor.quantizer->step_size = 1;

    // Perform basic code/decode test

    // - Allocate space and define input samples
    const uint32_t sample_count =
            1024 * (compressor.entropy_coder->max_expected_value + 1);
    v2f_sample_t *samples =
            malloc(sizeof(v2f_sample_t) * sample_count);
    uint8_t *compressed_block =
            malloc(sizeof(uint8_t) * sample_count *
                   compressor.entropy_coder->bytes_per_word);
    v2f_sample_t *reconstructed_samples =
            malloc(sizeof(v2f_sample_t) * sample_count);
    if (samples == NULL || compressed_block == NULL ||
        reconstructed_samples == NULL) {
        if (samples != NULL) { free(samples); }
        if (compressed_block != NULL) { free(compressed_block); }
        if (reconstructed_samples != NULL) { free(reconstructed_samples); }
    }
    for (uint32_t i = 0; i < sample_count; i++) {
        samples[i] = i % (compressor.entropy_coder->max_expected_value + 1);
    }
    samples[1] = compressor.entropy_coder->max_expected_value - 1;
    samples[sample_count - 1] = compressor.entropy_coder->max_expected_value;
    log_info("Successfully loaded V2F codec from %s", input_path);
    log_info("\tExpected input: %d byte(s) per sample",
             (int) decompressor.entropy_decoder->bytes_per_sample);
    log_info("\tMax sample value (after quantization): %u",
             compressor.decorrelator->max_sample_value);
    log_info("\tOutput word size: %d byte(s)",
             (int) decompressor.entropy_decoder->bytes_per_word);
    {
        uint32_t different_trees = 1;
        void *last_root = decompressor.entropy_decoder->roots[0];
        for (uint32_t i = 1;
             i < decompressor.entropy_decoder->root_count; i++) {
            if (decompressor.entropy_decoder->roots[i] != last_root) {
                different_trees++;
            }
            last_root = (void *) decompressor.entropy_decoder->roots[i];
        }
        log_info(
                "\tThe V2F forest has %u trees. At most %u of these are different.",
                decompressor.entropy_decoder->root_count,
                different_trees);
    }
    uint32_t included_nodes = decompressor.entropy_decoder->roots[0]->root_included_count;
    bool any_different = false;
    for (uint32_t i = 1; i < decompressor.entropy_decoder->root_count; i++) {
        if (decompressor.entropy_decoder->roots[0]->root_included_count !=
            included_nodes) {
            any_different = true;
            break;
        }
    }
    if (any_different) {
        log_info("\tThe first tree has %u included nodes. "
                 "Others have different amounts.", included_nodes);
    } else {
        log_info("\tAll trees have %u included nodes.", included_nodes);
    }
    const uint32_t optimal_included_nodes =
            (uint32_t) (UINT64_C(1)
                    << (8 * decompressor.entropy_decoder->bytes_per_word));
    if (any_different || (included_nodes != optimal_included_nodes)) {
        log_warning(
                "\tTree size is NOT optimal: all trees should have included exactly %u nodes.",
                optimal_included_nodes);
    } else {
        log_info("\tTree size IS optimal.");
    }

    log_info("\tQuantizer mode: %d.", compressor.quantizer->mode);
    log_info("\tQuantization step size: %d.", compressor.quantizer->step_size);
    log_info("\tDecorrelator mode: %d.", (int) compressor.decorrelator->mode);
    log_info("\tSamples per row: %lu.",
             compressor.decorrelator->samples_per_row);
    
    {


        // - Compress block
        uint64_t written_byte_count;
        timer_start("Block coding");
        status = v2f_compressor_compress_block(
                &compressor, samples, sample_count, compressed_block,
                &written_byte_count);
        timer_stop("Block coding");
        if (status != V2F_E_NONE) {
            log_error("Error compressing test block. Status = %d.",
                      (int) status);
            return 1;
        }

        // - Decompress block
        uint64_t written_sample_count;
        timer_start("Block decoding");
        status = v2f_decompressor_decompress_block(
                &decompressor, compressed_block, written_byte_count,
                sample_count, reconstructed_samples, &written_sample_count);
        timer_stop("Block decoding");
        if (status != V2F_E_NONE) {
            log_error("Error decompressing test block. Status = %d.",
                      (int) status);
            return 1;
        }

        // - Verify lossless
        if (compressor.quantizer->mode == V2F_C_QUANTIZER_MODE_NONE
            || compressor.quantizer->step_size == 1) {
            for (uint32_t i = 0; i < sample_count; i++) {
                v2f_sample_t abs_error = (v2f_sample_t) labs(
                        (int64_t) samples[i] -
                        (int64_t) reconstructed_samples[i]);
                if (abs_error != 0) {
                    fprintf(stderr,
                            "Error: the loaded V2F codec is not lossless.\n");

                    log_error("sample_count = %u", sample_count);
                    log_error("i = %u", i);
                    log_error("samples[i] = %u", samples[i]);
                    log_error("reconstructed_samples[i] = %u",
                              reconstructed_samples[i]);
                    log_error("written_sample_count = %lu",
                              written_sample_count);
                    log_error(
                            "sample index %u; original %u != reconstructed %u",
                            i, samples[i], reconstructed_samples[i]);

                    return (int) V2F_E_CORRUPTED_DATA;
                }
            }
        }

//        break;
    }

    log_info("Successfully compressed %u test samples with the loaded forest, "
             "exercising the maximum sample value.", sample_count);

    if (_LOG_LEVEL >= LOG_INFO_LEVEL) {
        log_info("A time report of this test is shown next:");
        timer_report_human(stdout);
    }

    if (_LOG_LEVEL >= LOG_DEBUG_LEVEL) {
        log_debug("The codec V2F forest contents are shown next:");
        log_debug("There are %u trees in the forest.",
                 compressor.entropy_coder->root_count);
        v2f_entropy_coder_entry_t *last_root = NULL;
        for (uint32_t root_index = 0;
             root_index < compressor.entropy_coder->root_count;
             root_index++) {
            if (compressor.entropy_coder->roots[root_index] == last_root) {
                break;
            }
            log_debug("Showing tree index %u (max index %u):\n",
                     root_index, compressor.entropy_coder->root_count - 1);
            print_coder_node_recursive(
                    stdout, compressor.entropy_coder,
                    compressor.entropy_coder->roots[root_index], 0,
                    root_index);
            printf("\n");
            last_root = compressor.entropy_coder->roots[root_index];
        }
    }

    // Clean up
    status = v2f_file_destroy_read_codec(&compressor, &decompressor);
    if (status != V2F_E_NONE) {
        log_error("Error destroying codec pair");
        log_error("status = %d", (int) status);
        return 1;
    }
    fclose(input_file);
    free(samples);
    free(reconstructed_samples);
    free(compressed_block);

    return 0;
}
