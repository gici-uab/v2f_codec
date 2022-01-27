/**
 * @file
 * @author Miguel Hernández Cabronero <miguel.hernandez@uab.cat>
 * @date 16/02/2021
 * @version 1.0
 *
 * @brief Implementation of the entropy coding routines.
 */

#include "v2f_entropy_coder.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "v2f.h"
#include "log.h"
#include "timer.h"

v2f_error_t v2f_entropy_coder_create(
        v2f_entropy_coder_t *const coder,
        v2f_sample_t max_expected_value,
        uint8_t bytes_per_word,
        v2f_entropy_coder_entry_t **roots,
        uint32_t root_count) {
    if (coder == NULL || roots == NULL
        || bytes_per_word < V2F_C_MIN_BYTES_PER_WORD
        || bytes_per_word > V2F_C_MAX_BYTES_PER_WORD
        || root_count < V2F_C_MIN_ROOT_COUNT
        || root_count > V2F_C_MAX_ROOT_COUNT
        || max_expected_value < 1
        || max_expected_value > V2F_C_MAX_SAMPLE_VALUE) {

        return V2F_E_INVALID_PARAMETER;
    }
    coder->roots = roots;
    coder->root_count = root_count;
    coder->current_entry = roots[0];
    coder->bytes_per_word = bytes_per_word;
    coder->max_expected_value = max_expected_value;

    return V2F_E_NONE;
}

v2f_error_t v2f_entropy_coder_destroy(v2f_entropy_coder_t *const coder) {
    if (coder == NULL
        || coder->bytes_per_word < V2F_C_MIN_BYTES_PER_WORD
        || coder->bytes_per_word > V2F_C_MAX_BYTES_PER_WORD) {
        return V2F_E_INVALID_PARAMETER;
    }

    return V2F_E_NONE;
}

v2f_error_t v2f_entropy_coder_compress_block(
        v2f_entropy_coder_t *const coder,
        v2f_sample_t const *const input_samples,
        uint64_t sample_count,
        uint8_t *const output_buffer,
        uint64_t *const written_byte_count) {
    timer_start("v2f_entropy_coder_compress_block");

    if (coder == NULL || input_samples == NULL || sample_count == UINT64_MAX) {
        return V2F_E_INVALID_PARAMETER;
    }

    // Blocks are independently coded, hence the first root is always the starting point
    coder->current_entry = coder->roots[0];

    uint8_t *buffer = output_buffer;

    uint64_t write_count = 0;
    write_count = 0;
    for (uint64_t sample_index = 0;
         sample_index < sample_count;
         sample_index++) {
        // Note: this loop is heavily optimized and uses some potentially unsafe operations
        // to avoid conditional branching. Therefore, static and/or dynamic
        // code checkers will

        const v2f_sample_t sample = input_samples[sample_index];
        const uint32_t current_children_count = coder->current_entry->children_count;

        uint64_t emit = (uint64_t) (current_children_count <= sample);
        const uint64_t bytes_emit = emit * (uint64_t) coder->bytes_per_word;

        log_debug("sample_index = %lu", sample_index);
        log_debug("sample_count = %lu", sample_count);
        log_debug("coder->current_entry = %p",
                  (void *) (coder->current_entry));
        log_debug("current_children_count = %u", current_children_count);
        log_debug("sample = %u", sample);
        log_debug("emit = %d", (int) emit);
        if (_LOG_LEVEL >= LOG_DEBUG_LEVEL) {
            if (emit > 0) {
                uint64_t word_value = 0;
                log_debug("Emitted word:");
                for (uint32_t i = 0; i < coder->bytes_per_word; i++) {
                    log_no_newline(LOG_DEBUG_LEVEL, " %x", (int) coder->current_entry->word_bytes[i]);
                    word_value += (uint64_t) (
                            coder->current_entry->word_bytes[i]
                                    << ((coder->bytes_per_word - i - 1) * 8));
                }
                log_no_newline(LOG_DEBUG_LEVEL, " :: %lu \n", word_value);
            }
            log_no_newline(LOG_DEBUG_LEVEL, "\n\n");
        }


        // NOTE: this triggers "Conditional jump or move depends on uninitialised value(s)"
        // errors in valgrind. This is ok, because bytes_emit will be zero
        // whenever an invalid/uninitialzied address is passed.
        // This potentially unsafe operation is included to allow branchless
        // compression.
        memcpy(buffer,
               coder->current_entry->word_bytes + (1 - emit),
               bytes_emit);
        buffer += bytes_emit;
        write_count += bytes_emit;
        uint64_t new_address = ((uint64_t) coder->current_entry);


        // If word is emitted, the next root is selected based on the number
        // of children of the emitted word.
        new_address += emit *
                       (((uint64_t) (coder->roots[current_children_count *
                                                  emit]->children_entries[sample]))
                        - ((uint64_t) (coder->current_entry)));

        // If it is not emitted, a child of the current entry is used instead
        new_address += (1 - emit) *
                       (((uint64_t) (coder->current_entry->children_entries[
                               sample * (1 - emit)]))
                        - ((uint64_t) (coder->current_entry)));

        coder->current_entry = (v2f_entropy_coder_entry_t *) new_address;
    }

    // Emit the last element if included. If not included, another codeword is emited instead.
    // The first child is recursively explored until an included node is found.
    // The decoder knows the total number of samples in the block, so this is not problematic.
    while (coder->current_entry->children_count ==
           coder->max_expected_value + 1) {
        coder->current_entry = coder->current_entry->children_entries[0];
    }
    memcpy(buffer, coder->current_entry->word_bytes, coder->bytes_per_word);
    write_count += coder->bytes_per_word;

    if (written_byte_count != NULL) {
        *written_byte_count = write_count;
    }

    timer_stop("v2f_entropy_coder_compress_block");

    return V2F_E_NONE;
}

v2f_error_t v2f_entropy_coder_fill_entry(
        uint8_t bytes_per_index,
        uint32_t index,
        v2f_entropy_coder_entry_t *const entry) {
    if (entry == NULL || index >= V2F_C_MAX_ENTRY_COUNT) {
        return V2F_E_INVALID_PARAMETER;
    }

    if (index >= (UINT64_C(1) << (8 * bytes_per_index))) {
        return V2F_E_INVALID_PARAMETER;
    }
    for (uint8_t i = 0; i < bytes_per_index; i++) {
        const uint8_t shift = (uint8_t) (8 * (bytes_per_index - i - 1));
        entry->word_bytes[i] = (uint8_t) (
                (index & ((uint32_t) (0xff << shift))) >> shift);
    }

    return V2F_E_NONE;
}

v2f_sample_t v2f_entropy_coder_buffer_to_sample(
        uint8_t const *const data_buffer,
        uint8_t bytes_per_sample) {
    v2f_sample_t sample = 0;

    for (uint8_t i = 0; i < bytes_per_sample; i++) {
        sample <<= 8;
        sample |= data_buffer[i];
    }

    return sample;
}

void v2f_entropy_coder_sample_to_buffer(
        const v2f_sample_t sample,
        uint8_t *const data_buffer,
        uint8_t bytes_per_sample) {
    for (uint8_t i = 0; i < bytes_per_sample; i++) {
        const uint32_t shift = (uint32_t) (8 * (bytes_per_sample - 1 - i));
        const v2f_sample_t value = (v2f_sample_t) (
                (sample & (v2f_sample_t) ((v2f_sample_t) 0xff << shift))
                        >> shift);
        assert (value <= UINT8_MAX);
        data_buffer[i] = (uint8_t) value;
    }
}
