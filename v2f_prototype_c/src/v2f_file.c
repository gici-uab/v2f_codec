/**
 * @file v2f_file.c
 * @author Miguel Hernández Cabronero <miguel.hernandez@uab.cat>
 * @date 17/02/2021
 *
 * Implementation of tools to handle data in files.
 */

#include "v2f_file.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "v2f_entropy_coder.h"
#include "v2f_entropy_decoder.h"
#include "log.h"
#include "timer.h"

v2f_error_t v2f_file_write_codec(
        FILE *output_file,
        v2f_compressor_t *const compressor,
        v2f_decompressor_t *const decompressor) {
    if (output_file == NULL || compressor == NULL || decompressor == NULL
        || compressor->quantizer == NULL || compressor->decorrelator == NULL
        || compressor->entropy_coder == NULL
        || decompressor->entropy_decoder == NULL) {
        return V2F_E_INVALID_PARAMETER;
    }
    if (compressor->quantizer != decompressor->quantizer
        || compressor->decorrelator != decompressor->decorrelator) {
        return V2F_E_INVALID_PARAMETER;
    }

    if (compressor->quantizer->mode >= V2F_C_QUANTIZER_MODE_COUNT
        || compressor->decorrelator->mode >= V2F_C_DECORRELATOR_MODE_COUNT
        || compressor->decorrelator->max_sample_value > V2F_C_MAX_SAMPLE_VALUE
        || compressor->decorrelator->max_sample_value < 1) {
        log_error("quantizer_mode = %u", compressor->quantizer->mode);
        log_error("decorrelator_mode = %u", compressor->decorrelator->mode);
        log_error("max_sample_value = %u", compressor->decorrelator->max_sample_value);
        return V2F_E_INVALID_PARAMETER;
    }

    v2f_sample_t value;

    // Write quantizer mode
    value = compressor->quantizer->mode;
    RETURN_IF_FAIL(v2f_file_write_big_endian(output_file, &value, 1, 1));

    // Write quantizer step size
    value = compressor->quantizer->step_size;
    RETURN_IF_FAIL(v2f_file_write_big_endian(output_file, &value, 1, 4));

    // Write decorrelator mode
    value = compressor->decorrelator->mode;
    RETURN_IF_FAIL(v2f_file_write_big_endian(output_file, &value, 1, 2));

    // Write decorrelator max input sample value
    value = compressor->decorrelator->max_sample_value;
    RETURN_IF_FAIL(v2f_file_write_big_endian(output_file, &value, 1, 4));

    // Write forest id (0: included explicitly)
    value = 0;
    RETURN_IF_FAIL(v2f_file_write_big_endian(output_file, &value, 1, 4));

    // Write forest
    RETURN_IF_FAIL(v2f_file_write_forest(
            output_file,
            compressor->entropy_coder,
            decompressor->entropy_decoder,
            0));

    return V2F_E_NONE;
}

v2f_error_t v2f_file_read_codec(
        FILE *input_file,
        v2f_compressor_t *const compressor,
        v2f_decompressor_t *const decompressor) {
    timer_start("v2f_file_read_codec");

    // Read quantizer and decorrelator parameters
    log_debug("ftello(input_file) before = %ld", ftello(input_file));
    v2f_sample_t quantizer_mode;
    RETURN_IF_FAIL(
            v2f_file_read_big_endian(input_file, &quantizer_mode, 1, 1, NULL));
    log_debug("quantizer_mode = %u", quantizer_mode);


    log_debug("ftello(input_file) before = %ld", ftello(input_file));
    v2f_sample_t step_size;
    RETURN_IF_FAIL(
            v2f_file_read_big_endian(input_file, &step_size, 1, 4, NULL));
    log_debug("step_size = %u", step_size);


    log_debug("ftello(input_file) before = %ld", ftello(input_file));
    v2f_sample_t decorrelator_mode;
    RETURN_IF_FAIL(
            v2f_file_read_big_endian(input_file, &decorrelator_mode, 1, 2,
                                     NULL));
    log_debug("decorrelator_mode = %u", decorrelator_mode);

    log_debug("ftello(input_file) before = %ld", ftello(input_file));
    v2f_sample_t max_sample_value;
    RETURN_IF_FAIL(
            v2f_file_read_big_endian(input_file, &max_sample_value, 1, 4,
                                     NULL));
    log_debug("max_sample_value = %u", max_sample_value);

    if (quantizer_mode >= V2F_C_QUANTIZER_MODE_COUNT
        || decorrelator_mode >= V2F_C_DECORRELATOR_MODE_COUNT
        || max_sample_value > V2F_C_MAX_SAMPLE_VALUE
        || max_sample_value < 1) {
        log_error("quantizer_mode = %u", quantizer_mode);
        log_error("decorrelator_mode = %u", decorrelator_mode);
        log_error("max_sample_value = %u", max_sample_value);
        return V2F_E_INVALID_PARAMETER;
    }

    // Allocate and initialize quantizer and decorrelator
    log_debug("allocating quantizer & decorrelator...");
    log_debug("ftello(input_file) before = %ld", ftello(input_file));
    v2f_quantizer_t *quantizer = malloc(sizeof(v2f_quantizer_t));
    v2f_decorrelator_t *decorrelator = malloc(sizeof(v2f_decorrelator_t));
    void *pointers[] = {quantizer, decorrelator};
    v2f_error_t allocation_status = V2F_E_NONE;
    for (uint32_t i = 0; i < sizeof(pointers) / sizeof(void *); i++) {
        if (pointers[i] == NULL) {
            free(pointers[i]);
            allocation_status = V2F_E_OUT_OF_MEMORY;
        }
    }
    RETURN_IF_FAIL(allocation_status);

    // Create quantizer and decorrelator
    log_debug("Initializing quantizer and decorrelator...");
    v2f_error_t quantizer_status = v2f_quantizer_create(
            quantizer, quantizer_mode, step_size, max_sample_value);
    v2f_error_t decorrelator_status = v2f_decorrelator_create(
            decorrelator, decorrelator_mode, max_sample_value);
    if (quantizer_status != V2F_E_NONE || decorrelator_status != V2F_E_NONE) {
        for (uint32_t i = 0; i < sizeof(pointers) / sizeof(void *); i++) {
            if (pointers[i] == NULL) {
                free(pointers[i]);
                allocation_status = V2F_E_OUT_OF_MEMORY;
            }
        }
        free(quantizer);
        free(decorrelator);

        log_error("quantizer_status = %u", quantizer_status);
        log_error("decorrelator_status = %u", decorrelator_status);

        return quantizer_status == V2F_E_NONE ?
               decorrelator_status : quantizer_status;
    }

    // Read or load entropy coder/decoder pair
    log_debug("Reading forest index...");
    v2f_sample_t forest_index;
    RETURN_IF_FAIL(
            v2f_file_read_big_endian(input_file, &forest_index, 1, 4, NULL));
    if (forest_index != 0) {
        log_error("Unsupported value forest_index = %u", forest_index);
        for (uint32_t i = 0; i < sizeof(pointers) / sizeof(void *); i++) {
            if (pointers[i] == NULL) {
                free(pointers[i]);
            }
        }
        free(quantizer);
        free(decorrelator);
        return V2F_E_FEATURE_NOT_IMPLEMENTED;
    }

    // Read forest
    log_debug("Reading forest...");
    v2f_entropy_coder_t *entropy_coder = malloc(sizeof(v2f_entropy_coder_t));
    v2f_entropy_decoder_t *entropy_decoder = malloc(
            sizeof(v2f_entropy_decoder_t));
    v2f_error_t forest_status = v2f_file_read_forest(
            input_file, entropy_coder, entropy_decoder);
    if (forest_status != V2F_E_NONE) {
        for (uint32_t i = 0; i < sizeof(pointers) / sizeof(void *); i++) {
            if (pointers[i] == NULL) {
                free(pointers[i]);
            }
        }
        free(quantizer);
        free(decorrelator);
        RETURN_IF_FAIL(forest_status);
    }

    // Initialize codec
    log_debug("Initializing codec...");
    v2f_error_t compressor_status = v2f_compressor_create(
            compressor, quantizer, decorrelator, entropy_coder);
    v2f_error_t decompressor_status =
            compressor_status != V2F_E_NONE ?
            compressor_status : v2f_decompressor_create(
                    decompressor, quantizer, decorrelator, entropy_decoder);
    if (compressor_status != V2F_E_NONE || decompressor_status != V2F_E_NONE) {
        for (uint32_t i = 0; i < sizeof(pointers) / sizeof(void *); i++) {
            if (pointers[i] == NULL) {
                free(pointers[i]);
            }
        }
        free(quantizer);
        free(decorrelator);
        RETURN_IF_FAIL(decompressor_status);
    }

    timer_stop("v2f_file_read_codec");

    return V2F_E_NONE;
}

v2f_error_t v2f_file_destroy_read_codec(
        v2f_compressor_t *const compressor,
        v2f_decompressor_t *const decompressor) {
    if (compressor == NULL || decompressor == NULL) {
        return V2F_E_INVALID_PARAMETER;
    }

    v2f_error_t destroy_forest_status = v2f_file_destroy_read_forest(
            compressor->entropy_coder, decompressor->entropy_decoder);
    free(compressor->entropy_coder);
    free(decompressor->entropy_decoder);
    free(compressor->quantizer);
    free(compressor->decorrelator);

    RETURN_IF_FAIL(destroy_forest_status);

    return V2F_E_NONE;
}

v2f_error_t v2f_file_write_forest(FILE *output,
                                  v2f_entropy_coder_t const *const coder,
                                  v2f_entropy_decoder_t const *const decoder,
                                  uint32_t different_roots) {
    if (output == NULL || coder == NULL) {
        return V2F_E_INVALID_PARAMETER;
    }

    if (different_roots == 0) {
        v2f_entropy_decoder_root_t *last_root = NULL;
        for (uint32_t i = 0; i < decoder->root_count; i++) {
            if (decoder->roots[i] == last_root) {
                break;
            }
            last_root = decoder->roots[i];
            different_roots++;
        }
        if (different_roots == 0) {
            log_error("different_roots = %u", different_roots);
            return V2F_E_INVALID_PARAMETER;
        }
    }

    v2f_sample_t value;

    // Entry count
    uint64_t total_entry_count = 0;
    uint32_t max_tree_size = 0;
    uint32_t min_tree_size = UINT32_MAX;
    uint32_t max_included_count = 0;
    v2f_entropy_coder_entry_t *last_root = NULL;
    for (uint32_t r = 0; r < decoder->root_count; r++) {
        if (coder->roots[r] == last_root) {
            break;
        }
        last_root = coder->roots[r];

        total_entry_count += decoder->roots[r]->root_entry_count;
        max_tree_size = max_tree_size >= decoder->roots[r]->root_entry_count ?
                        max_tree_size : decoder->roots[r]->root_entry_count;
        min_tree_size = min_tree_size <= decoder->roots[r]->root_entry_count ?
                        min_tree_size : decoder->roots[r]->root_entry_count;

        uint32_t included_count = 0;
        for (uint32_t i = 0; i < decoder->roots[r]->root_entry_count; i++) {
            if (decoder->roots[r]->entries_by_index[i].children_count !=
                (coder->max_expected_value + 1)) {
                included_count++;
            }
        }
        max_included_count = max_included_count >= included_count ?
                             max_included_count : included_count;
    }
    assert(min_tree_size >= V2F_C_MIN_ENTRY_COUNT);
    assert(max_tree_size <= V2F_C_MAX_ENTRY_COUNT);
    assert(total_entry_count >= V2F_C_MIN_ENTRY_COUNT);
    assert(total_entry_count <= V2F_C_MAX_ENTRY_COUNT);
    assert(max_included_count <=
           (UINT64_C(1) << (8 * decoder->bytes_per_sample)));
    value = (v2f_sample_t) (total_entry_count);
    RETURN_IF_FAIL(v2f_file_write_big_endian(output, &value, 1, 4));

    // Bytes per word
    assert(coder->bytes_per_word >= V2F_C_MIN_BYTES_PER_WORD);
    assert(coder->bytes_per_word <= V2F_C_MAX_BYTES_PER_WORD);
    value = (v2f_sample_t) (coder->bytes_per_word);
    RETURN_IF_FAIL(v2f_file_write_big_endian(output, &value, 1, 1));

    // Bytes per sample
    assert(decoder->bytes_per_sample >= V2F_C_MIN_BYTES_PER_SAMPLE);
    assert(decoder->bytes_per_sample <= V2F_C_MAX_BYTES_PER_SAMPLE);
    value = (v2f_sample_t) (decoder->bytes_per_sample);
    RETURN_IF_FAIL(v2f_file_write_big_endian(output, &value, 1, 1));

    // Max expected sample value
    assert(coder->max_expected_value <= V2F_C_MAX_SAMPLE_VALUE);
    value = (v2f_sample_t) (coder->max_expected_value);
    RETURN_IF_FAIL(v2f_file_write_big_endian(output, &value, 1, 2));

    // Root count
    assert(different_roots >= V2F_C_MIN_ROOT_COUNT);
    assert(different_roots <= V2F_C_MAX_ROOT_COUNT);
    value = (v2f_sample_t) different_roots - 1;
    RETURN_IF_FAIL(v2f_file_write_big_endian(output, &value, 1, 2));

    // Write roots
    for (uint32_t i = 0; i < different_roots; i++) {
        assert(coder->roots[i]->children_count > 0);
        assert(coder->roots[i]->children_count - 1 <= UINT16_MAX);

        // Write the total number of entries in the root
        value = decoder->roots[i]->root_entry_count;
        RETURN_IF_FAIL(v2f_file_write_big_endian(output, &value, 1, 4));

        // Write the total number of included entries in the root
        value = decoder->roots[i]->root_included_count;
        RETURN_IF_FAIL(v2f_file_write_big_endian(output, &value, 1, 4));

        // Write entries
        for (uint32_t j = 0; j < decoder->roots[i]->root_entry_count; j++) {
            // Index
            value = j;
            RETURN_IF_FAIL(v2f_file_write_big_endian(
                    output, &value, 1, V2F_C_BYTES_PER_INDEX));

            // Children count
            value = decoder->roots[i]->entries_by_index[j].coder_entry->children_count;
            RETURN_IF_FAIL(v2f_file_write_big_endian(
                    output, &value, 1, 4));

            // Children indices
            for (uint32_t c = 0;
                 c <
                 decoder->roots[i]->entries_by_index[j].coder_entry->children_count;
                 c++) {
                if (fwrite(
                        decoder->roots[i]->entries_by_index[j].coder_entry->children_entries[c]->word_bytes,
                        coder->bytes_per_word, 1, output) != 1) {
                    return V2F_E_IO;
                }
            }

            if (decoder->roots[i]->entries_by_index[j].coder_entry->children_count !=
                (coder->max_expected_value + 1)) {
                // Sample count
                value = (v2f_sample_t) decoder->roots[i]->entries_by_index[j].sample_count;
                RETURN_IF_FAIL(v2f_file_write_big_endian(
                        output, &value, 1, 2));

                // Samples
                for (uint32_t s = 0;
                     s < decoder->roots[i]->entries_by_index[j].sample_count;
                     s++) {
                    value = decoder->roots[i]->entries_by_index[j].samples[s];
                    RETURN_IF_FAIL(v2f_file_write_big_endian(
                            output, &value, 1, decoder->bytes_per_sample));
                }

                // word bytes
                if (fwrite(
                        decoder->roots[i]->entries_by_index[j].coder_entry->word_bytes,
                        coder->bytes_per_word, 1, output) != 1) {
                    return V2F_E_IO;
                }
            }
        }

        // Write the number of children of the root
        bool missing_i = (coder->roots[i]->children_count ==
                          (coder->max_expected_value + 1 - i));
        if (coder->roots[i]->children_count < coder->max_expected_value + 1) {
            // Non full root
            if (!missing_i) {
                log_debug(
                        "Root index %u has %u children, which is not full nor "
                        "is lacking exactly %u children",
                        i, coder->roots[i]->children_count, i);
                return V2F_E_INVALID_PARAMETER;
            }
        }
        value = (v2f_sample_t) coder->roots[i]->children_count;
        RETURN_IF_FAIL(v2f_file_write_big_endian(output, &value, 1, 4));

        // Write root children indices
        for (uint32_t j = 0; j < coder->roots[i]->children_count; j++) {
            // Write index
            bool found = false;
            for (uint32_t k = 0;
                 k < decoder->roots[i]->root_entry_count; k++) {
                if (coder->roots[i]->children_entries[j]
                    == decoder->roots[i]->entries_by_index[k].coder_entry) {
                    value = k;
                    RETURN_IF_FAIL(v2f_file_write_big_endian(
                            output, &value, 1, V2F_C_BYTES_PER_INDEX));
                    found = true;
                    break;
                }

            }
            if (!found) {
                log_error("decoder->roots[i]->entries_by_index[k] "
                          "cannot be found in coder->roots[i].");
                return V2F_E_INVALID_PARAMETER;
            }

            // Write associated symbol_value
            uint32_t symbol_value = j;
            if (missing_i) {
                symbol_value += i;
            }
            assert(symbol_value <= coder->max_expected_value);
            value = symbol_value;
            RETURN_IF_FAIL(v2f_file_write_big_endian(
                    output, &value, 1, decoder->bytes_per_sample));
        }
    }

    return V2F_E_NONE;
}

// TODO: cleanup memory allocation in a more elegant way

v2f_error_t v2f_file_read_forest(
        FILE *input,
        v2f_entropy_coder_t *const coder,
        v2f_entropy_decoder_t *const decoder) {
    log_debug("Reading coder/decoder pair from file");

    v2f_sample_t value;

    // Read the total number of entries
    RETURN_IF_FAIL(v2f_file_read_big_endian(input, &value, 1, 4, NULL));
    if (value < V2F_C_MIN_ENTRY_COUNT ||
        value > V2F_C_MAX_ENTRY_COUNT) {
        log_error("Invalid parameter: total_entry_count = %u", value);
        return V2F_E_CORRUPTED_DATA;
    }
    const uint32_t total_entry_count = value;
    uint32_t remaining_entry_count = total_entry_count;
    log_debug("total_entry_count = %u", total_entry_count);

    // Read the number of bytes per word
    RETURN_IF_FAIL(v2f_file_read_big_endian(input, &value, 1, 1, NULL));
    if (value < V2F_C_MIN_BYTES_PER_WORD ||
        value > V2F_C_MAX_BYTES_PER_WORD) {
        log_error("Invalid parameter: bytes_per_word = %u", value);
        return V2F_E_CORRUPTED_DATA;
    }
    const uint8_t bytes_per_word = (uint8_t) value;
    log_debug("bytes_per_word = %d", (int) bytes_per_word);

    // Read the number of bytes_per_sample
    RETURN_IF_FAIL(v2f_file_read_big_endian(input, &value, 1, 1, NULL));
    const uint8_t bytes_per_sample = (uint8_t) value;
    log_debug("bytes_per_sample = %d", (int) bytes_per_sample);
    if (bytes_per_sample < V2F_C_MIN_BYTES_PER_SAMPLE ||
        bytes_per_sample > V2F_C_MAX_BYTES_PER_SAMPLE) {
        log_error("Invalid parameter: bytes_per_sample = %u", value);
        return V2F_E_CORRUPTED_DATA;
    }

    // Read the maximum sample value
    RETURN_IF_FAIL(v2f_file_read_big_endian(input, &value, 1, 2, NULL));
    v2f_sample_t max_expected_value = value;
    if (value > V2F_C_MAX_SAMPLE_VALUE) {
        log_error("Invalid parameter: max_expected_sample = %u", value);
        return V2F_E_CORRUPTED_DATA;
    }
    log_debug("max_expected_value = %d", (int) max_expected_value);

    // Read the number of included roots
    RETURN_IF_FAIL(v2f_file_read_big_endian(input, &value, 1, 2, NULL));
    if (value > (v2f_sample_t) (V2F_C_MAX_ROOT_COUNT - 1)) {
        log_error("included_root_count = %u", value + 1);
        return V2F_E_CORRUPTED_DATA;
    }
    v2f_sample_t included_root_count = value + 1;
    if (included_root_count > (max_expected_value + 1)) {
        log_error("included_root_count = %u", included_root_count);
        return V2F_E_CORRUPTED_DATA;
    }
    log_debug("included_root_count = %u", included_root_count);

    // Allocate roots
    v2f_entropy_coder_entry_t **coder_root_pointers =
            malloc(sizeof(v2f_entropy_coder_entry_t *) *
                   (max_expected_value + 1));
    v2f_entropy_decoder_root_t **decoder_root_pointers =
            malloc(sizeof(v2f_entropy_decoder_root_t *) *
                   (max_expected_value + 1));

    void *root_pointers[] = {coder_root_pointers, decoder_root_pointers};
    if (coder_root_pointers == NULL || decoder_root_pointers == NULL) {
        log_error("coder_root_pointers = %p", (void *) coder_root_pointers);
        log_error("decoder_root_pointers = %p",
                  (void *) decoder_root_pointers);
        return V2F_E_OUT_OF_MEMORY;
    }

    v2f_entropy_coder_entry_t **null_children_entries = malloc(
            sizeof(v2f_entropy_coder_entry_t *));
    null_children_entries[0] = malloc(sizeof(v2f_entropy_coder_entry_t));
    null_children_entries[0]->children_count = 0;
    null_children_entries[0]->word_bytes = NULL;
    null_children_entries[0]->children_entries = malloc(
            sizeof(v2f_entropy_coder_entry_t *));
    null_children_entries[0]->children_entries[0] = NULL;
    log_debug("null_children_entries = %p", (void *) null_children_entries);
    log_debug("null_children_entries[0] = %p",
              (void *) (null_children_entries[0]));

    // Read the roots
    v2f_error_t status;
    for (uint32_t root_index = 0;
         root_index < included_root_count; root_index++) {
        log_debug("Reading root index %u (max index %u)", root_index,
                  (uint32_t) included_root_count - 1);

        // Allocate root entry
        coder_root_pointers[root_index] = malloc(
                sizeof(v2f_entropy_coder_entry_t));
        decoder_root_pointers[root_index] = malloc(
                sizeof(v2f_entropy_decoder_root_t));
        if (coder_root_pointers[root_index] == NULL ||
            decoder_root_pointers == NULL) {
            return V2F_E_OUT_OF_MEMORY;
        }

        // Total entry count (with or without assigned codeword)
        v2f_sample_t root_total_entry_count;
        status = v2f_file_read_big_endian(
                input, &root_total_entry_count, 1, 4, NULL);
        if (status != V2F_E_NONE
            || root_total_entry_count < V2F_C_MIN_ENTRY_COUNT
            || root_total_entry_count > V2F_C_MAX_ENTRY_COUNT
            || root_total_entry_count > remaining_entry_count) {
            return status == V2F_E_NONE ? V2F_E_CORRUPTED_DATA : status;
        }
        decoder_root_pointers[root_index]->root_entry_count = root_total_entry_count;
        log_debug("root_total_entry_count = %u", root_total_entry_count);

        // Total included entry count (with assigned codeword)
        v2f_sample_t root_included_count;
        status = v2f_file_read_big_endian(
                input, &root_included_count, 1, 4, NULL);
        if (status != V2F_E_NONE
            || root_included_count < V2F_C_MIN_ENTRY_COUNT
            || root_included_count > V2F_C_MAX_ENTRY_COUNT
            || root_included_count > remaining_entry_count
            || root_included_count > root_total_entry_count) {
            return status == V2F_E_NONE ? V2F_E_CORRUPTED_DATA : status;
        }
        decoder_root_pointers[root_index]->root_included_count = root_included_count;
        log_debug("root_included_count = %u", root_included_count);

        // Allocate decoder entries
        decoder_root_pointers[root_index]->entries_by_index =
                malloc(sizeof(v2f_entropy_decoder_entry_t) *
                       root_total_entry_count);
        if (decoder_root_pointers[root_index]->entries_by_index == NULL) {
            for (uint32_t i = 0;
                 i < sizeof(root_pointers) / sizeof(void *); i++) {
                if (root_pointers[i] != NULL) {
                    free(root_pointers[i]);
                }
            }
            return V2F_E_OUT_OF_MEMORY;
        }

        log_debug("root#%u coder_root_pointers[%u] = %p",
                  root_index, root_index,
                  (void *) (coder_root_pointers[root_index]));

        // Read and allocate root entries
        uint32_t next_index;
        for (next_index = 0;
             next_index < root_total_entry_count;
             next_index++) {
            // Allocate
            decoder_root_pointers[root_index]->entries_by_index[next_index].coder_entry =
                    malloc(sizeof(v2f_entropy_coder_entry_t));
            decoder_root_pointers[root_index]->entries_by_index[next_index].coder_entry->word_bytes =
                    malloc(sizeof(uint8_t) * bytes_per_word);
            if (decoder_root_pointers[root_index]->entries_by_index[
                        next_index].coder_entry == NULL
                || decoder_root_pointers[root_index]->entries_by_index[
                           next_index].coder_entry->word_bytes == NULL) {
                return V2F_E_OUT_OF_MEMORY;
            }

            // Index
            v2f_sample_t entry_index;
            status = v2f_file_read_big_endian(
                    input, &entry_index, 1, V2F_C_BYTES_PER_INDEX, NULL);
            if (status != V2F_E_NONE
                || entry_index >= V2F_C_MAX_ENTRY_COUNT
                || entry_index >= root_total_entry_count) {
                return status == V2F_E_NONE ? V2F_E_CORRUPTED_DATA : status;
            }
            if (entry_index != next_index) {
                log_error(
                        "This implementation expects entry_index == next_index, but they differ "
                        "(%u != %u)\n", entry_index, next_index);
                return V2F_E_CORRUPTED_DATA;
            }
            log_debug("Read index %u of root %u (%p)",
                      entry_index, root_index,
                      (void *) (decoder_root_pointers[root_index]->entries_by_index[
                              entry_index].coder_entry));

            // Number of children
            status = v2f_file_read_big_endian(input, &value, 1, 4, NULL);
            const v2f_sample_t entry_children_count = value;
            if (status != V2F_E_NONE ||
                entry_children_count > V2F_C_MAX_CHILD_COUNT) {
                log_error("entry_children_count = %u", entry_children_count);
                log_error("status = %d", (int) status);

                return status == V2F_E_NONE ? V2F_E_CORRUPTED_DATA : status;
            }
            decoder_root_pointers[root_index]->entries_by_index[
                    entry_index].coder_entry->children_count = entry_children_count;
            if (_LOG_LEVEL >= LOG_DEBUG_LEVEL + 1) {
                log_debug("entry_children_count = %u", entry_children_count);
            }

            // Allocate children pointers
            if (entry_children_count == 0) {
                decoder_root_pointers[root_index]->entries_by_index[
                        entry_index].children_count = 0;
                decoder_root_pointers[root_index]->entries_by_index[
                        entry_index].coder_entry->children_count = 0;
                decoder_root_pointers[root_index]->entries_by_index[
                        entry_index].coder_entry->children_entries = null_children_entries;
            } else {
                decoder_root_pointers[root_index]->entries_by_index[
                        entry_index].children_count = entry_children_count;
                decoder_root_pointers[root_index]->entries_by_index[
                        entry_index].coder_entry->children_count = entry_children_count;
                decoder_root_pointers[root_index]->entries_by_index[
                        entry_index].coder_entry->children_entries =
                        malloc(sizeof(v2f_entropy_coder_entry_t *) *
                               entry_children_count);
                if (decoder_root_pointers[root_index]->entries_by_index[
                            entry_index].coder_entry->children_entries ==
                    NULL) {
                    return status == V2F_E_NONE ? V2F_E_CORRUPTED_DATA
                                                : status;
                }
            }

            // Assign children indices
            for (uint32_t c = 0; c < entry_children_count; c++) {
                status = v2f_file_read_big_endian(input, &value, 1,
                                                  V2F_C_BYTES_PER_INDEX, NULL);
                const v2f_sample_t child_index = value;

                if (status != V2F_E_NONE ||
                    child_index >= V2F_C_MAX_ENTRY_COUNT) {
                    log_error("Error assigning children entries: status = %u "
                              "[root %u, child_index = %u]",
                              status, root_index, child_index);
                    return status == V2F_E_NONE ? V2F_E_CORRUPTED_DATA
                                                : status;
                }

                // Pointers might not have been initialized yet. In this first
                // pass, the root's global index is stored and then the pointers
                // are assigned once all memory has been allocated.
                decoder_root_pointers[root_index]->entries_by_index[
                        entry_index].coder_entry->children_entries[c] =
                        (v2f_entropy_coder_entry_t *) ((uint64_t) child_index);
                if (_LOG_LEVEL >= LOG_DEBUG_LEVEL + 1) {
                    log_debug("Child index %u for (root %u, index %u) %p, ",
                              child_index, root_index, entry_index,
                              (void *) (decoder_root_pointers[root_index]->entries_by_index[
                                      entry_index].coder_entry->children_entries[c]));
                }
            }


            if (entry_children_count < max_expected_value + 1) {
                // Read sample count
                status = v2f_file_read_big_endian(input, &value, 1, 2, NULL);
                const v2f_sample_t sample_count = value;
                if (status != V2F_E_NONE
                    || sample_count < V2F_C_MIN_SAMPLE_COUNT
                    || sample_count > V2F_C_MAX_SAMPLE_COUNT) {
                    log_error("sample_count = %u", sample_count);
                    return status == V2F_E_NONE ? V2F_E_CORRUPTED_DATA
                                                : status;
                }
                decoder_root_pointers[root_index]->entries_by_index[entry_index].sample_count = sample_count;
                decoder_root_pointers[root_index]->entries_by_index[entry_index].samples =
                        malloc(sizeof(v2f_sample_t) * sample_count);
                if (decoder_root_pointers[root_index]->entries_by_index[entry_index].samples ==
                    NULL) {
                    log_error(
                            "(void*) (decoder_root_pointers[root_index]->entries_by_index[entry_index].samples) = %p",
                            (void *) (decoder_root_pointers[root_index]->entries_by_index[entry_index].samples));

                    return status == V2F_E_NONE ? V2F_E_CORRUPTED_DATA
                                                : status;
                }
                if (_LOG_LEVEL >= LOG_DEBUG_LEVEL + 1) {
                    log_debug("%u samples for (root %u, index %u)",
                              decoder_root_pointers[root_index]->entries_by_index[entry_index].sample_count,
                              root_index, entry_index);
                }

                // Read samples
                for (uint32_t s = 0;
                     s <
                     decoder_root_pointers[root_index]->entries_by_index[entry_index].sample_count;
                     s++) {
                    status = v2f_file_read_big_endian(input, &value, 1,
                                                      bytes_per_sample, NULL);
                    const v2f_sample_t sample_value = value;
                    if (status != V2F_E_NONE ||
                        sample_value > max_expected_value ||
                        sample_value > V2F_C_MAX_SAMPLE_VALUE) {
                        log_error("sample_value = %u", sample_value);
                        return status == V2F_E_NONE ? V2F_E_CORRUPTED_DATA
                                                    : status;
                    }
                    decoder_root_pointers[root_index]->entries_by_index[entry_index].samples[s] = sample_value;
                    log_no_newline(LOG_DEBUG_LEVEL, "%s%u, ",
                                   (s == 0 ? "Samples: " : ""), sample_value);
                }
                log_no_newline(LOG_DEBUG_LEVEL,
                               (decoder_root_pointers[root_index]->entries_by_index[entry_index].sample_count >
                                0 ? "\n" : ""));

                // Read word bytes
                status = v2f_file_read_big_endian(
                        input, &value, 1, bytes_per_word, NULL);
                if (value >=
                    decoder_root_pointers[root_index]->root_included_count) {
                    log_error("Invalid word value %u", value);
                    return V2F_E_CORRUPTED_DATA;
                }
                v2f_entropy_coder_sample_to_buffer(
                        value,
                        decoder_root_pointers[root_index]->entries_by_index[entry_index].coder_entry->word_bytes,
                        bytes_per_word);
                log_no_newline(LOG_DEBUG_LEVEL, "Words: ");
                for (uint32_t w = 0; w < bytes_per_word; w++) {
                    log_no_newline(LOG_DEBUG_LEVEL, "%d",
                                   (int) decoder_root_pointers[root_index]->entries_by_index[
                                           entry_index].coder_entry->word_bytes[w]);
                }
                log_no_newline(LOG_DEBUG_LEVEL, "\n");
            }
        }

        assert(remaining_entry_count >= root_total_entry_count);
        remaining_entry_count -= root_total_entry_count;
        log_debug("root_total_entry_count = %u", root_total_entry_count);

        // Read root's children count
        status = v2f_file_read_big_endian(input, &value, 1, 4, NULL);
        const v2f_sample_t root_children_count = value;
        // Non full root nodes are accepted, as long as at most root_index entries are missing.
        const bool non_full_tree = (root_children_count <= max_expected_value);
        const bool missing_r = (non_full_tree && (root_children_count ==
                                                  (max_expected_value + 1 -
                                                   root_index)));
        if (status != V2F_E_NONE
            || root_children_count > root_included_count
            || root_children_count > V2F_C_MAX_CHILD_COUNT
            || (non_full_tree && (!missing_r))) {
            log_error("root_children_count = %u", root_children_count);
            return status == V2F_E_NONE ? V2F_E_CORRUPTED_DATA : status;
        }
        coder_root_pointers[root_index]->children_count = root_children_count;
        log_debug("root_children_count = %u", root_children_count);

        coder_root_pointers[root_index]->children_entries =
                malloc(sizeof(v2f_entropy_coder_entry_t *) *
                       (max_expected_value + 1));
        if (coder_root_pointers[root_index]->children_entries == NULL) {
            log_error(
                    "(void*) (coder_root_pointers[root_index]->children_entries) = %p",
                    (void *) (coder_root_pointers[root_index]->children_entries));

            return V2F_E_OUT_OF_MEMORY;
        }

        for (uint32_t c = 0; c < root_children_count; c++) {
            // Read root child index
            status = v2f_file_read_big_endian(
                    input, &value, 1, V2F_C_BYTES_PER_INDEX, NULL);
            const v2f_sample_t child_index = value;
            if (_LOG_LEVEL >= LOG_DEBUG_LEVEL + 1) {
                log_debug("child_index = %u", child_index);
            }

            v2f_sample_t symbol_value;
            bool valid_symbol_value = false;
            if (status == V2F_E_NONE) {
                // Read root child index input symbol value
                status = v2f_file_read_big_endian(
                        input, &value, 1, bytes_per_sample, NULL);
                symbol_value = value;
                valid_symbol_value = (
                        (!non_full_tree && symbol_value == c)
                        ||
                        (non_full_tree && missing_r &&
                         symbol_value == c + root_index));
            }

            if (status != V2F_E_NONE || child_index >= total_entry_count ||
                !valid_symbol_value) {
                log_error("child_index = %u", child_index);
                log_error("symbol_value = %u", symbol_value);
                log_error("valid_symbol_value = %d", (int) valid_symbol_value);

                return status == V2F_E_NONE ? V2F_E_CORRUPTED_DATA : status;
            }
            if (!non_full_tree) {
                coder_root_pointers[root_index]->children_entries[c] =
                        decoder_root_pointers[root_index]->entries_by_index[child_index].coder_entry;
            } else {
                assert(missing_r);
                coder_root_pointers[root_index]->children_entries[c +
                                                                  root_index] =
                        decoder_root_pointers[root_index]->entries_by_index[child_index].coder_entry;
            }
        }

        // Allocate word to index table
        // Create word to included node structure
        decoder_root_pointers[root_index]->entries_by_word =
                malloc(sizeof(v2f_entropy_decoder_entry_t *) *
                       decoder_root_pointers[root_index]->root_included_count);
        if (decoder_root_pointers[root_index]->entries_by_word == NULL) {
            log_error("error allocating entries by word for root_index = %u",
                      root_index);

            return V2F_E_OUT_OF_MEMORY;
        }
        memset(decoder_root_pointers[root_index]->entries_by_word,
               0, sizeof(v2f_entropy_decoder_entry_t *)
                  * decoder_root_pointers[root_index]->root_included_count);

        for (uint32_t i = 0; i < root_total_entry_count; i++) {
            // Assign pending child pointers
            for (uint32_t c = 0;
                 c <
                 decoder_root_pointers[root_index]->entries_by_index[i].coder_entry->children_count;
                 c++) {

                uint64_t pointer_index = (uint64_t) (decoder_root_pointers[
                        root_index]->entries_by_index[
                        i].coder_entry->children_entries[
                        c]);

                if (pointer_index >
                    decoder_root_pointers[root_index]->root_entry_count) {
                    log_error("pointer_index: %lu; entry count: %u",
                              pointer_index,
                              decoder_root_pointers[root_index]->root_entry_count);
                    return V2F_E_CORRUPTED_DATA;
                }

                decoder_root_pointers[root_index]->entries_by_index[i].coder_entry->children_entries[c] =
                        decoder_root_pointers[
                                root_index]->entries_by_index[pointer_index].coder_entry;
            }
        }

        // Assign word to entry table
        for (uint32_t index = 0;
             index <
             decoder_root_pointers[root_index]->root_entry_count; index++) {
            if (decoder_root_pointers[root_index]->entries_by_index[index].children_count <
                (max_expected_value + 1)) {
                v2f_sample_t word = v2f_entropy_coder_buffer_to_sample(
                        decoder_root_pointers[root_index]->entries_by_index[
                                index].coder_entry->word_bytes,
                        bytes_per_word);
                decoder_root_pointers[root_index]->entries_by_word[word] =
                        &(decoder_root_pointers[root_index]->entries_by_index[index]);
            }
        }

        for (uint32_t w = 0;
             w < decoder_root_pointers[root_index]->root_included_count; w++) {
            if (decoder_root_pointers[root_index]->entries_by_word[w] ==
                NULL) {
                log_debug("NULL pointer for w = %u", w);
                return V2F_E_CORRUPTED_DATA;
            }
        }

    }

    if (remaining_entry_count != 0) {
        log_error("remaining_entry_count = %u should be zero.",
                  remaining_entry_count);
        return V2F_E_CORRUPTED_DATA;
    }

    // Not all roots need to be defined
    for (uint32_t v = included_root_count; v < max_expected_value + 1; v++) {
        coder_root_pointers[v] = coder_root_pointers[included_root_count - 1];
        decoder_root_pointers[v] = decoder_root_pointers[included_root_count -
                                                         1];
    }

    v2f_error_t coder_status =
            v2f_entropy_coder_create(coder, max_expected_value,
                                     bytes_per_word, coder_root_pointers,
                                     max_expected_value + 1);
    v2f_error_t decoder_status =
            v2f_entropy_decoder_create(decoder, decoder_root_pointers,
                                       max_expected_value + 1,
                                       bytes_per_word, bytes_per_sample);
    if (coder_status != V2F_E_NONE || decoder_status != V2F_E_NONE) {
        for (uint32_t root_index = 0;
             root_index < included_root_count; root_index++) {
            for (uint32_t d = 0;
                 d <
                 decoder_root_pointers[root_index]->root_included_count; d++) {
                if (decoder_root_pointers[root_index]->entries_by_index[d].coder_entry->children_count >
                    0) {
                    free(decoder_root_pointers[root_index]->entries_by_index[d].coder_entry->children_entries);
                }
                if (decoder_root_pointers[root_index]->entries_by_index[d].coder_entry->children_count !=
                    (max_expected_value + 1)) {
                    free(decoder_root_pointers[root_index]->entries_by_index[d].coder_entry->word_bytes);
                }
                free(decoder_root_pointers[root_index]->entries_by_index[d].coder_entry);
            }
        }
        for (uint32_t i = 0;
             i < sizeof(root_pointers) / sizeof(void *); i++) {
            if (root_pointers[i] != NULL) {
                free(root_pointers[i]);
            }
        }
        free(null_children_entries[0]->children_entries);
        free(null_children_entries[0]);

        log_error("coder_status = %d", (int) coder_status);
        log_error("decoder_status = %d", (int) decoder_status);

        return status == V2F_E_NONE ? V2F_E_CORRUPTED_DATA : status;
    }


    return v2f_verify_forest(coder, decoder);
}

v2f_error_t v2f_file_destroy_read_forest(v2f_entropy_coder_t *coder,
                                         v2f_entropy_decoder_t *decoder) {
    if (coder == NULL || decoder == NULL ||
        coder->root_count != decoder->root_count) {
        return V2F_E_INVALID_PARAMETER;
    }

    v2f_entropy_decoder_root_t *last_root = NULL;
    bool null_children_deleted = false;
    for (uint32_t r = 0; r < decoder->root_count; r++) {
        if (decoder->roots[r] == last_root) {
            break;
        }
        for (uint32_t i = 0; i < decoder->roots[r]->root_entry_count; i++) {
            if (decoder->roots[r]->entries_by_index[i].coder_entry->children_count
                != (coder->max_expected_value + 1)) {
                free(decoder->roots[r]->entries_by_index[i].samples);
            }

            free(decoder->roots[r]->entries_by_index[i].coder_entry->word_bytes);
            if (decoder->roots[r]->entries_by_index[i].coder_entry->children_count >
                0) {
                free(decoder->roots[r]->entries_by_index[i].coder_entry->children_entries);
            } else if (!null_children_deleted) {
                free(decoder->roots[r]->entries_by_index[i].coder_entry->children_entries[0]->children_entries);
                free(decoder->roots[r]->entries_by_index[i].coder_entry->children_entries[0]);
                free(decoder->roots[r]->entries_by_index[i].coder_entry->children_entries);
                null_children_deleted = true;
            }
            free(decoder->roots[r]->entries_by_index[i].coder_entry);
        }
        assert(coder->roots[r]->children_count > 0);
        free(coder->roots[r]->children_entries);
        free(decoder->roots[r]->entries_by_index);
        free(decoder->roots[r]->entries_by_word);
        free(decoder->roots[r]);
        free(coder->roots[r]);
        last_root = decoder->roots[r];
    }
    free(coder->roots);
    free(decoder->roots);

    return V2F_E_NONE;
}

v2f_error_t v2f_verify_forest(
        v2f_entropy_coder_t *const coder,
        v2f_entropy_decoder_t *const decoder) {
    if (coder == NULL || decoder == NULL) {
        log_error("coder = %p", (void *) coder);
        log_error("decoder = %p", (void *) decoder);
        return V2F_E_INVALID_PARAMETER;
    }

    if (coder->root_count != decoder->root_count) {
        log_error("coder->root_count = %u", coder->root_count);
        log_error("decoder->root_count = %u", decoder->root_count);
        return V2F_E_INVALID_PARAMETER;
    }


    if (_LOG_LEVEL >= LOG_DEBUG_LEVEL + 1) {
        log_debug("coder->root_count = %u", coder->root_count);
        for (uint32_t r = 0; r < coder->root_count; r++) {
            log_debug("coder->roots[%u] = %p", r, (void *) (coder->roots[r]));
            log_debug("decoder->roots[%u] = %p", r,
                      (void *) (decoder->roots[r]));
            log_debug("coder->roots[%u]->children_count = %u",
                      r, coder->roots[r]->children_count);
            log_debug("decoder->roots[%u]->root_entry_count = %u",
                      r, decoder->roots[r]->root_entry_count);
        }
    }

    return V2F_E_NONE;
}

v2f_error_t v2f_file_read_big_endian(
        FILE *input_file,
        v2f_sample_t *sample_buffer,
        uint64_t max_sample_count,
        uint8_t bytes_per_sample,
        uint64_t *read_sample_count) {
    // Parameter sanitization
    if (sample_buffer == NULL
        || max_sample_count == 0
        || bytes_per_sample < 1
        || bytes_per_sample > 4
        || max_sample_count > V2F_C_MAX_BLOCK_SIZE) {
        return V2F_E_INVALID_PARAMETER;
    }

    // Keep track of the number of read samples, whether the user
    // requests its value back or not
    uint64_t _read_sample_count;
    if (read_sample_count == NULL) {
        read_sample_count = &_read_sample_count;
    }
    *read_sample_count = 0;

    // Read bytes onto the data buffer
    uint8_t *const data_buffer = (uint8_t *) sample_buffer;
    size_t read_bytes = fread(
            data_buffer,
            1,
            bytes_per_sample * max_sample_count,
            input_file);
    if (ferror(input_file) && !feof(input_file)) {
        return V2F_E_IO;
    }
    if (read_bytes % bytes_per_sample != 0) {
        return V2F_E_IO;
    }

    // Transform the read bytes into v2f_sample_t values assuming big endian.
    *read_sample_count = read_bytes / bytes_per_sample;
    for (uint64_t i = 0; i < *read_sample_count; i++) {


        v2f_sample_t *const next_sample =
                &(sample_buffer[*read_sample_count - i - 1]);
        uint8_t const *const next_data_buffer =
                &(data_buffer[(*read_sample_count - i - 1) *
                              bytes_per_sample]);
        *next_sample = v2f_entropy_coder_buffer_to_sample(
                next_data_buffer, bytes_per_sample);

        if (_LOG_LEVEL >= LOG_DEBUG_LEVEL + 1) {
            log_debug("*READ next_sample = %u", *next_sample);
        }
    }

    // Since max_sample_count must be greater than zero, an empty count
    // will never result in returning V2F_E_NONE (useful for looping!).
    return *read_sample_count == max_sample_count ?
           V2F_E_NONE : V2F_E_UNEXPECTED_END_OF_FILE;
}

v2f_error_t v2f_file_write_big_endian(
        FILE *output_file,
        v2f_sample_t *const sample_buffer,
        uint64_t sample_count,
        uint8_t bytes_per_sample) {

    for (uint64_t i = 0; i < sample_count; i++) {
        uint8_t output_buffer[bytes_per_sample];
        v2f_entropy_coder_sample_to_buffer(
                sample_buffer[i],
                output_buffer,
                bytes_per_sample);
        if (fwrite(output_buffer, bytes_per_sample, 1, output_file) != 1) {
            log_error(
                    "v2f_file_write_big_endian: [i=%lu] fwrite error %d (feof %d)",
                    i,
                    ferror(output_file),
                    feof(output_file));
            return V2F_E_IO;
        }
    }
    return V2F_E_NONE;
}


// Declared in v2f.h
int v2f_file_compress_from_path(
        char const *const raw_file_path,
        char const *const header_file_path,
        char const *const output_file_path,
        bool overwrite_quantizer_mode,
        v2f_quantizer_mode_t quantizer_mode,
        bool overwrite_qstep,
        v2f_sample_t step_size,
        bool overwrite_decorrelator_mode,
        v2f_decorrelator_mode_t decorrelator_mode) {

    // Basic parameter verification
    if (raw_file_path == NULL || header_file_path == NULL ||
        output_file_path == NULL
        || (overwrite_quantizer_mode &&
            quantizer_mode >= V2F_C_QUANTIZER_MODE_COUNT)
        || (overwrite_qstep && (step_size < 1 || step_size > 255))
        || (overwrite_decorrelator_mode &&
            decorrelator_mode > V2F_C_DECORRELATOR_MODE_COUNT)) {
        log_error("Invalid parameters");
        return 1;
    }

    FILE *raw_file = fopen(raw_file_path, "r");
    if (raw_file == NULL) {
        log_error("Cannot open input file %s for reading",
                  raw_file_path);
        return 1;
    }

    FILE *header_file = fopen(header_file_path, "r");
    if (header_file == NULL) {
        log_error("Cannot open V2F header file %s for reading.",
                  header_file_path);
        fclose(raw_file);
        return 1;
    }

    FILE *output_file = fopen(output_file_path, "w");
    if (output_file == NULL) {
        log_error("Cannot open output file %s for writing",
                  output_file_path);
        fclose(raw_file);
        fclose(header_file);
        return 1;
    }

    int status = v2f_file_compress_from_file(
            raw_file, header_file, output_file,
            overwrite_quantizer_mode, quantizer_mode,
            overwrite_qstep, step_size,
            overwrite_decorrelator_mode, decorrelator_mode);

    // Cleanup
    fclose(raw_file);
    fclose(header_file);
    fclose(output_file);

    return status;
}

// Declared in v2f.h
int v2f_file_compress_from_file(
        FILE *raw_file,
        FILE *header_file,
        FILE *output_file,
        bool overwrite_quantizer_mode,
        v2f_quantizer_mode_t quantizer_mode,
        bool overwrite_qstep,
        v2f_sample_t step_size,
        bool overwrite_decorrelator_mode,
        v2f_decorrelator_mode_t decorrelator_mode) {
    if (raw_file == NULL || header_file == NULL || output_file == NULL) {
        log_error("Invalid parameters");
        return 1;
    }

    // Read the entropy coder/decoder pair in the header file
    // (both are simultaneously defined)
    v2f_compressor_t compressor;
    v2f_decompressor_t decompressor;
    if (v2f_file_read_codec(header_file, &compressor, &decompressor)
        != V2F_E_NONE) {
        log_error("Error reading the V2F codec file");
        return 1;
    }

    // Apply overriding parameters if configured to do so
    if (overwrite_quantizer_mode) {
        compressor.quantizer->mode = quantizer_mode;
    }
    if (overwrite_qstep) {
        compressor.quantizer->step_size = step_size;
    }
    if (overwrite_decorrelator_mode) {
        compressor.decorrelator->mode = decorrelator_mode;
    }

    // Prepare buffers for the worst case
    // (full block with 1 word per input sample)
    v2f_sample_t *input_sample_buffer = (v2f_sample_t *) malloc(
            sizeof(v2f_sample_t) * V2F_C_MAX_BLOCK_SIZE);
    uint8_t *compressed_block_buffer = (uint8_t *) malloc(
            compressor.entropy_coder->bytes_per_word *
            (size_t) V2F_C_MAX_BLOCK_SIZE);
    if (input_sample_buffer == NULL || compressed_block_buffer == NULL) {
        log_error(
                "Error allocating input or output buffers with limits %ld and %d.\n",
                sizeof(v2f_sample_t) * V2F_C_MAX_BLOCK_SIZE,
                V2F_C_MAX_COMPRESSED_BLOCK_SIZE);
        if (input_sample_buffer != NULL) {
            free(input_sample_buffer);
        }
        if (compressed_block_buffer != NULL) {
            free(compressed_block_buffer);
        }
        v2f_file_destroy_read_codec(&compressor, &decompressor);
        return 1;
    }

    // Compress the blocks and output the block envelopes.
    // Samples are read in blocks of at most V2F_C_MAX_BLOCK_SIZE elements.
    // When an EOF is found, reading is stoped.
    v2f_error_t status = V2F_E_NONE;
    bool continue_reading = true;
    while (continue_reading && status == V2F_E_NONE) {
        // Read a raw block
        uint64_t read_sample_count;
        status = v2f_file_read_big_endian(
                raw_file, input_sample_buffer, V2F_C_MAX_BLOCK_SIZE,
                decompressor.entropy_decoder->bytes_per_sample,
                &read_sample_count);
        if (status != V2F_E_NONE && status != V2F_E_UNEXPECTED_END_OF_FILE) {
            log_error("Error reading input samples");
            break;
        }
        if (status != V2F_E_NONE) {
            // No more blocks will be read after this one.
            continue_reading = false;
        }

        if (read_sample_count == 0) {
            log_info("No more samples available");
            assert(status == V2F_E_UNEXPECTED_END_OF_FILE);
            status = V2F_E_NONE;
            break;
        }

        log_info("Enveloping block of %lu samples...", read_sample_count);

        // Compress the read block whenever a complete or partial block is read
        if (status == V2F_E_NONE || (status == V2F_E_UNEXPECTED_END_OF_FILE &&
                                     read_sample_count > 0)) {
            // Compress the block
            log_debug("\tcompressing block...");
            uint64_t written_byte_count;
            status = v2f_compressor_compress_block(
                    &compressor, input_sample_buffer, read_sample_count,
                    compressed_block_buffer, &written_byte_count);

            log_debug("\tsending envelope...");
            if (status == V2F_E_NONE) {
                // Generate the envelope only if compression is successful.

                // 1 - `compressed_bitstream_size`: 4 bytes, unsigned big-endian integer.
                {
                    assert(written_byte_count <= V2F_SAMPLE_T_MAX);
                    v2f_sample_t compressed_bitstream_size =
                            (v2f_sample_t) written_byte_count;
                    status = v2f_file_write_big_endian(output_file,
                                                       &compressed_bitstream_size,
                                                       1, 4);
                }
                if (status != V2F_E_NONE) {
                    break;
                }

                // 2 - `sample_count`: 4 bytes, unsigned big-endian integer.
                {
                    assert(read_sample_count <= V2F_SAMPLE_T_MAX);
                    v2f_sample_t casted_sample_count = (v2f_sample_t) read_sample_count;
                    status = v2f_file_write_big_endian(
                            output_file, &casted_sample_count, 1, 4);
                    read_sample_count = casted_sample_count;
                    if (status != V2F_E_NONE) {
                        break;
                    }
                }

                // 3 - `compressed_bitstream`: `compressed_bitstream_size` `bytes`.
                if (fwrite(compressed_block_buffer,
                           1, written_byte_count, output_file)
                    != written_byte_count) {
                    log_error("Error writing the compressed block");
                    status = V2F_E_IO;
                }

                log_info(
                        "... successfully enveloped %lu samples into a %lu byte bitstream.",
                        read_sample_count, written_byte_count);
            }
        }
    }

    // Cleanup and report status
    v2f_file_destroy_read_codec(&compressor, &decompressor);
    free(input_sample_buffer);
    free(compressed_block_buffer);

    // V2F_E_NONE is defined to be 0. It is compatible with this method's signature.
    return (int) status;
}

// Declared in v2f.h
int v2f_file_decompress_from_path(
        char const *const compressed_file_path,
        char const *const header_file_path,
        char const *const reconstructed_file_path,
        bool overwrite_quantizer_mode,
        v2f_quantizer_mode_t quantizer_mode,
        bool overwrite_qstep,
        v2f_sample_t step_size,
        bool overwrite_decorrelator_mode,
        v2f_decorrelator_mode_t decorrelator_mode) {

    // Basic parameter verification
    if (compressed_file_path == NULL || header_file_path == NULL ||
        reconstructed_file_path == NULL
        || (overwrite_quantizer_mode &&
            quantizer_mode >= V2F_C_QUANTIZER_MODE_COUNT)
        || (overwrite_qstep && (step_size < 1 || step_size > 255))
        || (overwrite_decorrelator_mode &&
            decorrelator_mode > V2F_C_DECORRELATOR_MODE_COUNT)) {
        log_error("Invalid parameters");
        return 1;
    }

    FILE *compressed_file = fopen(compressed_file_path, "r");
    if (compressed_file == NULL) {
        log_error("Cannot open input file %s for reading",
                  compressed_file_path);
        return 1;
    }

    FILE *header_file = fopen(header_file_path, "r");
    if (header_file == NULL) {
        log_error("Cannot open V2F header file %s for reading",
                  header_file_path);
        fclose(compressed_file);
        return 1;
    }

    FILE *reconstructed_file = fopen(reconstructed_file_path, "w");
    if (reconstructed_file == NULL) {
        log_error("Cannot open output file %s for writing",
                  reconstructed_file_path);
        fclose(compressed_file);
        fclose(header_file);
        return 1;
    }

    int status = v2f_file_decompress_from_file(
            compressed_file, header_file, reconstructed_file,
            overwrite_quantizer_mode, quantizer_mode,
            overwrite_qstep, step_size,
            overwrite_decorrelator_mode, decorrelator_mode);

    // Cleanup
    fclose(compressed_file);
    fclose(header_file);
    fclose(reconstructed_file);

    return status;
}

// Declared in v2f.h
int v2f_file_decompress_from_file(
        FILE *const compressed_file,
        FILE *const header_file,
        FILE *const reconstructed_file,
        bool overwrite_quantizer_mode,
        v2f_quantizer_mode_t quantizer_mode,
        bool overwrite_qstep,
        v2f_sample_t step_size,
        bool overwrite_decorrelator_mode,
        v2f_decorrelator_mode_t decorrelator_mode) {
    if (compressed_file == NULL
        || header_file == NULL
        || reconstructed_file == NULL) {
        log_error("Invalid parameters");
        return 1;
    }

    // Read the entropy coder/decoder pair in the header file
    v2f_compressor_t compressor;
    v2f_decompressor_t decompressor;
    if (v2f_file_read_codec(header_file, &compressor, &decompressor)
        != V2F_E_NONE) {
        log_error("Error reading the V2F codec file");
        return 1;
    }

    // Apply overriding parameters
    if (overwrite_quantizer_mode) {
        compressor.quantizer->mode = quantizer_mode;
    }
    if (overwrite_qstep) {
        compressor.quantizer->step_size = step_size;
    }
    if (overwrite_decorrelator_mode) {
        compressor.decorrelator->mode = decorrelator_mode;
    }

    // Prepare buffers for the worst case
    // (full block with 1 word per input sample)
    uint8_t *compressed_block_buffer = (uint8_t *) malloc(
            compressor.entropy_coder->bytes_per_word *
            (size_t) V2F_C_MAX_BLOCK_SIZE);
    v2f_sample_t *output_sample_buffer = (v2f_sample_t *) malloc(
            sizeof(v2f_sample_t) * V2F_C_MAX_BLOCK_SIZE);
    if (output_sample_buffer == NULL || compressed_block_buffer == NULL) {
        log_error(
                "Error allocating input or output buffers of %ld and %d bytes.\n",
                sizeof(v2f_sample_t) * V2F_C_MAX_BLOCK_SIZE,
                V2F_C_MAX_COMPRESSED_BLOCK_SIZE);
        if (output_sample_buffer != NULL) {
            free(output_sample_buffer);
        }
        if (compressed_block_buffer != NULL) {
            free(compressed_block_buffer);
        }
        v2f_file_destroy_read_codec(&compressor, &decompressor);
        return 1;
    }

    v2f_error_t status = V2F_E_NONE;
    bool found_eof_at_first_element = false;
    while (status == V2F_E_NONE) {
        // Read the compressed envelope
        // 1 - `compressed_bitstream_size`: 4 bytes, unsigned big-endian integer.
        v2f_sample_t compressed_bitstream_size;
        {
            uint64_t read_count;
            status = v2f_file_read_big_endian(
                    compressed_file, &compressed_bitstream_size,
                    1, 4, &read_count);
            if (status != V2F_E_NONE) {
                if (status == V2F_E_UNEXPECTED_END_OF_FILE &&
                    read_count == 0) {
                    // EOFs are expected to be aligned with envelopes.
                    found_eof_at_first_element = true;
                }
                break;
            }
        }
        if (compressed_bitstream_size == 0
            || compressed_bitstream_size > V2F_C_MAX_COMPRESSED_BLOCK_SIZE
            || (compressed_bitstream_size %
                compressor.entropy_coder->bytes_per_word
                != 0)) {
            status = V2F_E_CORRUPTED_DATA;
            log_error("Corrupted envelope?");
            break;
        }

        // 2 - `sample_count`: 4 bytes, unsigned big-endian integer.
        v2f_sample_t sample_count;
        status = v2f_file_read_big_endian(
                compressed_file, &sample_count, 1, 4, NULL);
        if (status != V2F_E_NONE) {
            break;
        }
        if (sample_count < V2F_C_MIN_BLOCK_SIZE
            || sample_count > V2F_C_MAX_BLOCK_SIZE) {
            status = V2F_E_CORRUPTED_DATA;
            log_error("Corrupted envelope?");
            break;
        }

        // 3 - `compressed_bitstream`: `compressed_bitstream_size` `bytes`.
        if (fread(compressed_block_buffer, 1,
                  compressed_bitstream_size, compressed_file)
            != compressed_bitstream_size) {
            status = V2F_E_CORRUPTED_DATA;
            log_error("Corrupted envelope?");
            break;
        }

        // At this point, data have been successfully read.
        // Now decode the envelope.
        {
            uint64_t decoded_sample_count;
            status = v2f_decompressor_decompress_block(
                    &decompressor, compressed_block_buffer,
                    compressed_bitstream_size, sample_count,
                    output_sample_buffer, &decoded_sample_count);
            if (decoded_sample_count != sample_count) {
                // The field and the actual number of samples shall match
                status = V2F_E_CORRUPTED_DATA;
            }
        }
        if (status != V2F_E_NONE) {
            log_error("Error decoding the envelope.");
            break;
        }

        log_info("Decoded an envelop with %u samples.", sample_count);

        // Finally output the samples to the file
        // Write Output File
        status = v2f_file_write_big_endian(
                reconstructed_file,
                output_sample_buffer,
                sample_count,
                decompressor.entropy_decoder->bytes_per_sample);
        if (status != V2F_E_NONE) {
            log_error("Error writing samples to output buffer.");
            break;
        }
    }

    // The way it is signaled when no more block envelopes are present
    // is by finding an and of file while reading the first element of the
    // envelope (and having read exactly 0 bytes in that read)
    if (status == V2F_E_UNEXPECTED_END_OF_FILE && found_eof_at_first_element) {
        status = V2F_E_NONE;
    }

    v2f_file_destroy_read_codec(&compressor, &decompressor);

    free(output_sample_buffer);
    free(compressed_block_buffer);

    // V2F_E_NONE is defined to be 0. It is compatible with this method's signature.
    return (int) status;
}
