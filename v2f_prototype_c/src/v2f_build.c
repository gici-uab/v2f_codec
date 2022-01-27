/**
 * @file v2f_build.c
 * @author Miguel Hernández Cabronero <miguel.hernandez@uab.cat>
 * @date 17/02/2021
 *
 * Tools to build coders and decoders.
 */

#include "v2f_build.h"

#include <stdlib.h>
#include <assert.h>

#include "log.h"
#include "errors.h"

v2f_error_t v2f_build_minimal_codec(
        uint8_t bytes_per_word,
        v2f_compressor_t *compressor,
        v2f_decompressor_t *decompressor) {
    if (compressor == NULL || decompressor == NULL) {
        return V2F_E_INVALID_PARAMETER;
    }

    // Build the quantizer and decorrelator
    v2f_quantizer_t *quantizer = malloc(sizeof(v2f_quantizer_t));
    v2f_decorrelator_t *decorrelator = malloc(sizeof(v2f_decorrelator_t));
    if (quantizer == NULL || decorrelator == NULL) {
        // LCOV_EXCL_START
        if (quantizer != NULL) { free(quantizer); }
        if (decorrelator != NULL) { free(decorrelator); }
        return V2F_E_OUT_OF_MEMORY;
        // LCOV_EXCL_STOP
    }
    RETURN_IF_FAIL(
            v2f_quantizer_create(quantizer, V2F_C_QUANTIZER_MODE_NONE, 1,
                                 ((v2f_sample_t) (1 << (8 * bytes_per_word)) -
                                  1)));
    RETURN_IF_FAIL(v2f_decorrelator_create(
            decorrelator,
            V2F_C_DECORRELATOR_MODE_NONE,
            (v2f_sample_t) ((1 << (8 * bytes_per_word)) - 1)));

    // Build the entropy entropy_coder and entropy_decoder
    v2f_entropy_coder_t *entropy_coder = malloc(sizeof(v2f_entropy_coder_t));
    v2f_entropy_decoder_t *entropy_decoder = malloc(
            sizeof(v2f_entropy_decoder_t));
    if (entropy_coder == NULL || entropy_decoder == NULL) {
        // LCOV_EXCL_START
        free(quantizer);
        free(decorrelator);
        if (entropy_coder != NULL) { free(entropy_coder); }
        if (entropy_decoder != NULL) { free(entropy_decoder); }
        return V2F_E_OUT_OF_MEMORY;
        // LCOV_EXCL_STOP
    }
    v2f_error_t status = v2f_build_minimal_forest(
            bytes_per_word, entropy_coder, entropy_decoder);
    if (status != V2F_E_NONE) {
        // LCOV_EXCL_START
        free(quantizer);
        free(decorrelator);
        free(entropy_coder);
        free(entropy_decoder);
        RETURN_IF_FAIL(status);
        // LCOV_EXCL_STOP
    }

    v2f_error_t status_compressor = v2f_compressor_create(
            compressor, quantizer, decorrelator, entropy_coder);
    v2f_error_t status_decompressor =
            status_compressor != V2F_E_NONE ?
            status_compressor : v2f_decompressor_create(
                    decompressor, quantizer, decorrelator, entropy_decoder);
    if (status_decompressor != V2F_E_NONE) {
        // LCOV_EXCL_START
        free(quantizer);
        free(decorrelator);
        free(entropy_coder);
        free(entropy_decoder);
        RETURN_IF_FAIL(status);
        // LCOV_EXCL_STOP
    }

    return V2F_E_NONE;
}

v2f_error_t v2f_build_destroy_minimal_codec(
        v2f_compressor_t *compressor,
        v2f_decompressor_t *decompressor) {
    if (compressor == NULL || decompressor == NULL) {
        return V2F_E_INVALID_PARAMETER;
    }

    free(compressor->quantizer);
    free(compressor->decorrelator);
    v2f_error_t status = v2f_build_destroy_minimal_forest(
            compressor->entropy_coder, decompressor->entropy_decoder);
    free(compressor->entropy_coder);
    free(decompressor->entropy_decoder);
    RETURN_IF_FAIL(status);

    return V2F_E_NONE;
}

v2f_error_t v2f_build_minimal_forest(
        uint8_t bytes_per_word,
        v2f_entropy_coder_t *coder,
        v2f_entropy_decoder_t *decoder) {
    if (coder == NULL || decoder == NULL
        || bytes_per_word < V2F_C_MINIMAL_MIN_BYTES_PER_WORD
        || bytes_per_word > V2F_C_MINIMAL_MAX_BYTES_PER_WORD) {
        return V2F_E_INVALID_PARAMETER;
    }

    const v2f_sample_t max_expected_value =
            (v2f_sample_t) ((UINT64_C(1) << (8 * bytes_per_word)) - 1);

    v2f_entropy_coder_entry_t **null_entry = malloc(
            sizeof(v2f_entropy_coder_entry_t *));
    if (null_entry == NULL) {
        return V2F_E_OUT_OF_MEMORY; // LCOV_EXCL_LINE
    }
    null_entry[0] = NULL;

    // Allocate memory
    const uint8_t bytes_per_sample = bytes_per_word;
    const uint32_t entry_count = (uint32_t) (1 << (8 * bytes_per_word));
    const uint32_t included_count = entry_count;
    const uint32_t symbol_count = entry_count;
    uint32_t root_count = symbol_count;
    v2f_entropy_coder_entry_t *coder_entries = malloc(
            sizeof(v2f_entropy_coder_entry_t) * entry_count);
    v2f_entropy_decoder_entry_t *decoder_entries = malloc(
            sizeof(v2f_entropy_decoder_entry_t) * entry_count);
    v2f_entropy_decoder_root_t **decoder_roots =
            malloc(sizeof(v2f_entropy_decoder_root_t *) * root_count);
    v2f_entropy_coder_entry_t **coder_roots = malloc(
            sizeof(v2f_entropy_coder_entry_t *) * root_count);
    coder_roots[0] = malloc(sizeof(v2f_entropy_coder_entry_t));
    decoder_roots[0] = malloc(sizeof(v2f_entropy_decoder_root_t));
    coder_roots[0]->children_entries = malloc(
            sizeof(v2f_entropy_coder_entry_t *) * symbol_count);
    void *pointers[] = {
            (void *) coder_entries, (void *) decoder_entries,
            (void *) coder_roots, (void *) decoder_roots,
            (void *) coder_roots[0], (void *) decoder_roots[0],
            (void *) coder_roots[0]->children_entries};

    if (coder_entries == NULL || decoder_entries == NULL
        || coder_roots == NULL || decoder_roots == NULL
        || coder_roots[0] == NULL || decoder_roots[0] == NULL
        || coder_roots[0]->children_entries == NULL) {
        // LCOV_EXCL_START
        for (uint32_t i = 0; i < sizeof(pointers) / sizeof(void *); i++) {
            if (pointers[i] != NULL) {
                free(pointers[i]);
            }
        }
        return V2F_E_OUT_OF_MEMORY;
        // LCOV_EXCL_STOP
    }

    // Build the coder and decoder entries for the minimal V2F tree
    bool free_mallocs = false;
    uint32_t next_index;
    for (next_index = 0; next_index < entry_count; next_index++) {
        coder_entries[next_index].word_bytes = malloc(
                sizeof(uint8_t) * bytes_per_word);
        if (coder_entries[next_index].word_bytes == NULL) {
            // LCOV_EXCL_START
            free_mallocs = true;
            break;
            // LCOV_EXCL_STOP
        }
        v2f_error_t status = v2f_entropy_coder_fill_entry(
                bytes_per_word, next_index, &(coder_entries[next_index]));
        if (status != V2F_E_NONE) {
            // LCOV_EXCL_START
            free(coder_entries[next_index].word_bytes);
            free_mallocs = true;
            break;
            // LCOV_EXCL_STOP
        }
        coder_entries[next_index].children_count = 0;
        coder_entries[next_index].children_entries = null_entry;
        decoder_entries[next_index].samples =
                malloc(sizeof(v2f_sample_t) * bytes_per_sample);
        if (decoder_entries[next_index].samples == NULL) {
            // LCOV_EXCL_START
            free_mallocs = true;
            free(coder_entries[next_index].word_bytes);
            break;
            // LCOV_EXCL_STOP
        }
        decoder_entries[next_index].sample_count = 1;
        decoder_entries[next_index].samples[0] = (v2f_sample_t) next_index;
        decoder_entries[next_index].children_count = 0;
        decoder_entries[next_index].coder_entry = &(coder_entries[next_index]);
    }
    if (free_mallocs) {
        // LCOV_EXCL_START
        for (uint32_t d = 0; d < next_index; d++) {
            free(coder_entries[d].word_bytes);
            free(decoder_entries[d].samples);
        }
        for (uint32_t i = 0; i < sizeof(pointers) / sizeof(void *); i++) {
            if (pointers[i] != NULL) {
                free(pointers[i]);
            }
        }
        return V2F_E_OUT_OF_MEMORY;
        // LCOV_EXCL_STOP
    }

    // Make the forest structure
    for (uint32_t i = 0; i < root_count; i++) {
        if (i == 0) {
            assert(entry_count == symbol_count);
            coder_roots[0]->word_bytes = NULL;
            coder_roots[0]->children_count = symbol_count;
            for (uint32_t j = 0; j < entry_count; j++) {
                coder_roots[0]->children_entries[j] = &(coder_entries[j]);
            }
        } else {
            coder_roots[i] = coder_roots[0];
        }
    }
    v2f_error_t status = v2f_entropy_coder_create(
            coder, max_expected_value,
            bytes_per_word, coder_roots, root_count);
    if (status != V2F_E_NONE) {
        // LCOV_EXCL_START
        for (uint32_t d = 0; d < entry_count; d++) {
            free(coder_entries[d].word_bytes);
            free(decoder_entries[d].samples);
        }
        for (uint32_t i = 0; i < sizeof(pointers) / sizeof(void *); i++) {
            if (pointers[i] != NULL) {
                free(pointers[i]);
            }
        }
        return status;
        // LCOV_EXCL_STOP
    }

    // Build decoder
    for (uint64_t r = 0; r < root_count; r++) {
        if (r == 0) {
            // Initialize coder forest
            for (uint32_t s = 0; s < symbol_count; s++) {
                coder_roots[0]->children_entries[s] = &(coder_entries[s]);
            }
            coder_roots[0]->children_count = entry_count;

            // Initialize decoder forest
            decoder_roots[0]->root_entry_count = entry_count;
            decoder_roots[0]->root_included_count = included_count;
            decoder_roots[0]->entries_by_index = decoder_entries;
            decoder_roots[0]->entries_by_word = malloc(
                    sizeof(v2f_entropy_decoder_entry_t *) * included_count);
            if (decoder_roots[0]->entries_by_word == NULL) {
                // LCOV_EXCL_START
                for (uint32_t d = 0; d < entry_count; d++) {
                    free(coder_entries[d].word_bytes);
                    free(decoder_entries[d].samples);
                }
                for (uint32_t i = 0;
                     i < sizeof(pointers) / sizeof(void *); i++) {
                    if (pointers[i] != NULL) {
                        free(pointers[i]);
                    }
                }
                return V2F_E_OUT_OF_MEMORY;
                // LCOV_EXCL_STOP
            }
            for (uint32_t i = 0; i < decoder_roots[0]->root_entry_count; i++) {
                if (decoder_roots[0]->entries_by_index[i].children_count <
                    (max_expected_value + 1)) {
                    v2f_sample_t word = v2f_entropy_coder_buffer_to_sample(
                            decoder_roots[0]->entries_by_index[i].coder_entry->word_bytes,
                            bytes_per_sample);
                    log_debug("Adding word word = %u", word);
                    decoder_roots[0]->entries_by_word[word] = &(decoder_roots[0]->entries_by_index[i]);
                }
            }
        } else {
            coder_roots[r] = coder_roots[0];
            decoder_roots[r] = decoder_roots[0];
        }
    }

    status = v2f_entropy_decoder_create(
            decoder, decoder_roots, root_count, bytes_per_word,
            bytes_per_word);
    if (status != V2F_E_NONE) {
        // LCOV_EXCL_START
        for (uint32_t d = 0; d < entry_count; d++) {
            free(coder_entries[d].word_bytes);
            free(decoder_entries[d].samples);
        }
        for (uint32_t i = 0; i < sizeof(pointers) / sizeof(void *); i++) {
            if (pointers[i] != NULL) {
                free(pointers[i]);
            }
        }
        return status;
        // LCOV_EXCL_STOP
    }

    return V2F_E_NONE;
}

v2f_error_t v2f_build_destroy_minimal_forest(v2f_entropy_coder_t *coder,
                                             v2f_entropy_decoder_t *decoder) {
    if (coder == NULL || decoder == NULL) {
        return V2F_E_INVALID_PARAMETER;
    }

    // Free decoder resources
    RETURN_IF_FAIL(v2f_entropy_decoder_destroy(decoder));
    v2f_entropy_decoder_root_t *last_root = NULL;
    for (uint32_t i = 0; i < decoder->root_count; i++) {
        if (decoder->roots[i] == last_root) {
            break;
        }

        for (uint32_t j = 0; j < decoder->roots[i]->root_entry_count; j++) {
            free(decoder->roots[i]->entries_by_index[j].samples);
            free(decoder->roots[i]->entries_by_index[j].coder_entry->word_bytes);
        }
        free(decoder->roots[i]->entries_by_index);
        free(decoder->roots[0]->entries_by_word);
        last_root = decoder->roots[i];
        free(decoder->roots[i]);
    }
    free(decoder->roots);

    // Free coder resources
    RETURN_IF_FAIL(v2f_entropy_coder_destroy(coder));
    free(coder->roots[0]->children_entries[0]);
    free(coder->roots[0]->children_entries);
    free(coder->roots[0]);
    free(coder->roots);

    return V2F_E_NONE;
}
