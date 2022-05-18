/**
 * @file
 * @author Miguel Hernández Cabronero <miguel.hernandez@uab.cat>
 * @date 16/02/2021
 *
 * Implementation of the V2F decoder.
 */

#include "v2f_entropy_decoder.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "errors.h"

v2f_error_t v2f_entropy_decoder_create(
        v2f_entropy_decoder_t *const decoder,
        v2f_entropy_decoder_root_t **roots,
        uint32_t root_count,
        uint8_t bytes_per_word,
        uint8_t bytes_per_sample) {

    if (decoder == NULL || roots == NULL
        || root_count < V2F_C_MIN_ROOT_COUNT
        || root_count > V2F_C_MAX_ROOT_COUNT
        || bytes_per_word < V2F_C_MIN_BYTES_PER_WORD
        || bytes_per_word > V2F_C_MAX_BYTES_PER_WORD
        || bytes_per_sample < V2F_C_MIN_BYTES_PER_SAMPLE
        || bytes_per_sample > V2F_C_MAX_BYTES_PER_SAMPLE) {
        log_error("decoder = %p", (void*) decoder);
        log_error("roots = %p", (void*) roots);
        log_error("decoder: root_count = %u", root_count);
        log_error("decoder: bytes_per_word = %d", (int) bytes_per_word);
        log_error("decoder: bytes_per_sample = %d", (int) bytes_per_sample);
        return V2F_E_INVALID_PARAMETER;
    }

    uint32_t total_node_count = 0;
    for (uint32_t r = 0; r < root_count; r++) {
        total_node_count += roots[r]->root_entry_count;
    }

    for (uint64_t i = 0; i < root_count; i++) {
        if (roots[i]->entries_by_index == NULL
            || roots[i]->entries_by_word == NULL
            || roots[i]->root_entry_count<V2F_C_MIN_ENTRY_COUNT
                                          ||
                                          roots[i]->root_entry_count>V2F_C_MAX_ENTRY_COUNT
            || roots[i]->root_included_count<V2F_C_MIN_ENTRY_COUNT
                                             ||
                                             roots[i]->root_included_count>V2F_C_MAX_ENTRY_COUNT
            || roots[i]->root_included_count > roots[i]->root_entry_count) {

            log_error("roots[i]->entries_by_index = %p",
                      (void *) (roots[i]->entries_by_index));
            log_error("roots[i]->entries_by_word  = %p",
                      (void *) (roots[i]->entries_by_word));
            log_error("roots[i]->root_entry_count = %u",
                      roots[i]->root_entry_count);
            log_error("roots[i]->root_included_count = %u",
                      roots[i]->root_included_count);


            return V2F_E_INVALID_PARAMETER;
        }
    }

    for (uint32_t i = 0; i < root_count; i++) {
        if (roots[i]->root_included_count >
            (uint64_t) (UINT64_C(1) << (8 * bytes_per_word))) {
            log_error("roots[%u]->root_entry_count = %u", i, roots[i]->root_entry_count);
            return V2F_E_INVALID_PARAMETER;
        }
    }

    decoder->bytes_per_word = bytes_per_word;
    decoder->bytes_per_sample = bytes_per_sample;
    decoder->roots = roots;
    decoder->root_count = root_count;
    decoder->current_root = roots[0];
    decoder->null_entry = NULL;

    return V2F_E_NONE;
}

v2f_error_t v2f_entropy_decoder_destroy(v2f_entropy_decoder_t *const decoder) {
    if (decoder == NULL
        || decoder->bytes_per_word == 0 || decoder->roots == NULL) {
        return V2F_E_INVALID_PARAMETER;
    }
    if (decoder->null_entry != NULL) {
        free(decoder->null_entry);
        decoder->null_entry = NULL;
    }
    for (uint64_t i = 0; i < decoder->root_count; i++) {
        if (decoder->roots[i]->entries_by_index == NULL) {
            return V2F_E_INVALID_PARAMETER;
        }
    }

    return V2F_E_NONE;
}

v2f_error_t v2f_entropy_decoder_decompress_block(
        v2f_entropy_decoder_t *const decoder,
        uint8_t const *const compressed_block,
        uint64_t compressed_size,
        v2f_sample_t *const reconstructed_samples,
        uint64_t max_output_sample_count,
        uint64_t *const written_sample_count) {
    if (decoder == NULL || compressed_block == NULL
        || compressed_size == 0 || reconstructed_samples == NULL) {
        return V2F_E_INVALID_PARAMETER;
    }
    if (compressed_size % decoder->bytes_per_word != 0) {
        return V2F_E_INVALID_PARAMETER;
    }

    // Blocks are independently coded, hence the first root is always the starting point
    decoder->current_root = decoder->roots[0];

    uint64_t write_count = 0;

    uint8_t const *input_buffer = compressed_block;
    log_debug("compressed_block = %p", compressed_block);
    log_debug("compressed_block + compressed_size = %p",
              compressed_block + compressed_size);
    log_debug("compressed_size = %lu", compressed_size);
    uint64_t consumed_bytes = 0;
    for (uint64_t word_index = 0;
         word_index < compressed_size / decoder->bytes_per_word;
         word_index++) {
        log_debug("word_index = %lu", word_index);

        uint32_t samples_written;
        v2f_sample_t decoded_samples[V2F_C_MAX_SAMPLE_COUNT];
        RETURN_IF_FAIL(v2f_entropy_decoder_decode_next_index(
                decoder, input_buffer, decoded_samples, &samples_written));
        log_debug("samples_written = %u", samples_written);
        
        for (uint32_t i = 0;
             i < samples_written && write_count < max_output_sample_count;
             i++) {
            log_debug("i = %u", i);
            log_debug("decoded_samples[i] = %u", decoded_samples[i]);
            reconstructed_samples[write_count] = decoded_samples[i];
            write_count++;
        }
        input_buffer += decoder->bytes_per_word;
        consumed_bytes += decoder->bytes_per_word;

        log_debug("input_buffer = %p", input_buffer);
        assert(input_buffer < input_buffer + compressed_size);

        log_debug("consumed_bytes = %lu / compressed_size = %lu",
                  consumed_bytes, compressed_size);
    }

    if (written_sample_count != NULL) {
        *written_sample_count = write_count;
    }

    return V2F_E_NONE;
}

v2f_error_t v2f_entropy_decoder_decode_next_index(
        v2f_entropy_decoder_t *const decoder,
        uint8_t const *const compressed_block,
        v2f_sample_t *const output_samples,
        uint32_t *const samples_written) {

    log_debug("DECODING NEXT SAMPLE\n\n\n");

    v2f_sample_t word = v2f_entropy_coder_buffer_to_sample(compressed_block,
                                                           decoder->bytes_per_word);
    log_debug("word = %u", word);
    log_debug("decoder->current_root = %p", (void *) (decoder->current_root));
    log_debug("decoder->bytes_per_word = %u", decoder->bytes_per_word);
    log_debug("decoder->current_root->root_entry_count = %u",
              decoder->current_root->root_entry_count);
    log_debug(
            "decoder->current_root->entries_by_index[word].children_count = %u",
            decoder->current_root->entries_by_index[word].children_count);

    if (word >= decoder->current_root->root_included_count) {
        return V2F_E_CORRUPTED_DATA;
    }

    if (decoder->current_root->entries_by_word[word]->children_count
        >= decoder->root_count) {
        // The decoder does not have enough roots to go after this sample.
        return V2F_E_CORRUPTED_DATA;
    }

    uint32_t sample_write_count = 0;
    log_debug("decoder->current_root->entries_by_word[word]->sample_count = %u",
              decoder->current_root->entries_by_word[word]->sample_count);
    for (uint32_t s = 0;
         s < decoder->current_root->entries_by_word[word]->sample_count; s++) {
        output_samples[s] = decoder->current_root->entries_by_word[word]->samples[s];
        sample_write_count++;
        log_debug(
                "decoder->current_root->entries_by_index[%u].samples[%u] = %u",
                word, s,
                decoder->current_root->entries_by_word[word]->samples[s]);
    }

    decoder->current_root = decoder->roots[
            decoder->current_root->entries_by_word[word]->children_count];

    if (samples_written != NULL) {
        assert(sample_write_count <= V2F_C_MAX_SAMPLE_COUNT);
        *samples_written = sample_write_count;
    }

    return V2F_E_NONE;
}
