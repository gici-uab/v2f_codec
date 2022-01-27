/**
 * @file
 * @author Miguel Hernández Cabronero <miguel.hernandez@uab.cat>
 * @date 16/02/2021
 *
 * @brief Definition of the v2f encoder structures and methods
 */

#ifndef V2F_ENTROPY_CODER_H
#define V2F_ENTROPY_CODER_H

#include "v2f.h"

// Many v2f_* enums and structs are defined in v2f.h

/**
 * Initialize an coder.
 *
 * @param coder pointer to the coder to be initialized.
 * @param max_expected_value maximum expected value of any input sample
 * @param bytes_per_word number of bytes used to store each codeword
 * @param roots pointers to root entries.
 * @param root_count number of root entries
 *
 * @return
 *  - @ref V2F_E_NONE : Initialization was successful
 *  - @ref V2F_E_INVALID_PARAMETER : At least one parameter was invalid
 */
v2f_error_t v2f_entropy_coder_create(
        v2f_entropy_coder_t *const coder,
        v2f_sample_t max_expected_value,
        uint8_t bytes_per_word,
        v2f_entropy_coder_entry_t **roots,
        uint32_t root_count);

/**
 * This function does NOT free any variables passed to the initialization.
 *
 * @param coder coder to be destroyed
 *
 * @return
 *  - @ref V2F_E_NONE : Destroy was successful
 *  - @ref V2F_E_INVALID_PARAMETER : invalid parameter provided (NULL pointer
 *    or coder does not seem initialized)
 */
v2f_error_t v2f_entropy_coder_destroy(v2f_entropy_coder_t *const coder);

/**
 * Compress the samples in `input_samples` and write the result to output_buffer
 * using `coder`.
 *
 * @param coder intitialized entropy coder to be used for compression
 * @param input_samples buffer with at least `input_samples`
 *   v2f_sample_t values.
 * @param sample_count number of samples to be coded from the buffer.
 *   Must be < UINT64_MAX.
 * @param output_buffer buffer where the output is produced. It must be large
 *  enough to accommodate the worst case scenario, i.e., one index is emitted
 *  per input symbol (input_samples*coder->bytes_per_word bytes).
 * @param written_byte_count pointer to a variable where the number of bytes
 *   written to output_buffer is stored. If the pointer is NULL, it is ignored.
 *
 * @return
 *  - @ref V2F_E_NONE : The block was successfully compressed
 *  - @ref V2F_E_INVALID_PARAMETER : invalid parameter provided
 */
v2f_error_t v2f_entropy_coder_compress_block(
        v2f_entropy_coder_t *const coder,
        v2f_sample_t const *const input_samples,
        uint64_t sample_count,
        uint8_t *const output_buffer,
        uint64_t *const written_byte_count);

/**
 * Fill the index bytes of `entry` given its index.
 *
 * Values are stored in big-endian order.
 *
 * @param bytes_per_index coder to which the entry belong
 * @param entry entry to be filled. Its word_bytes member must be
 *   a buffer with enough bytes given coder->bytes_per_word.
 * @param index index to be assigned to the entry
 *
 * @return
 *  - @ref V2F_E_NONE : The entry was successfully filled
 *  - @ref V2F_E_INVALID_PARAMETER : invalid parameter provided
 */
v2f_error_t v2f_entropy_coder_fill_entry(
        uint8_t bytes_per_index,
        uint32_t index,
        v2f_entropy_coder_entry_t *const entry);

/**
 * Read a sample value from a buffer of uint8_t values.
 *
 * The sample may have more than one byte per sample. If so,
 * the value is treated as big endian.
 *
 * @param data_buffer buffer of uint8_t values
 * @param bytes_per_sample number of bytes per sample
 *
 * @return the value of the first sample stored in the buffer,
 * as v2f_sample_t.
 */
v2f_sample_t v2f_entropy_coder_buffer_to_sample(
        uint8_t const *const data_buffer,
        uint8_t bytes_per_sample);

/**
 * Write a single sample to a uint8_t buffer.
 * Big endian is used when necessary.
 *
 * @param sample sample to be written
 * @param data_buffer buffer with enough space to write `bytes_per_sample` bytes
 * @param bytes_per_sample number of bytes to be used to represent the
 *   sample.
 */
void v2f_entropy_coder_sample_to_buffer(
        const v2f_sample_t sample,
        uint8_t *const data_buffer,
        uint8_t bytes_per_sample);

#endif /* V2F_ENTROPY_CODER_H */
