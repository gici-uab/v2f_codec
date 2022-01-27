/**
 * @file
 * 
 * @brief Implementation of the entropy decoding routines.
 */

#ifndef V2F_ENTROPY_DECODER_H
#define V2F_ENTROPY_DECODER_H

#include "v2f.h"
#include "v2f_entropy_coder.h"

// Many v2f_* enums and structs are defined in v2f.h

/**
 * Initialize a decoder with the given table of decoder entries by index.
 *
 * @param decoder decoder to be initialized.
 * @param roots list of root nodes.
 * @param root_count number of roots in `roots`.
 * @param bytes_per_word number of bytes used to store each codeword
 *   in the compressed data.
 * @param bytes_per_sample number of bytes used to store each decoded sample.
 * @return
 *   - @ref V2F_E_NONE : Creation was successful
 *   - @ref V2F_E_INVALID_PARAMETER : At least one parameter was invalid
 */
v2f_error_t v2f_entropy_decoder_create(
        v2f_entropy_decoder_t *const decoder,
        v2f_entropy_decoder_root_t ** roots,
        uint32_t root_count,
        uint8_t bytes_per_word,
        uint8_t bytes_per_sample);

/**
 * Destroy a decoder, releasing any resources allocated during initialization.
 *
 * @param decoder pointer to the decoder to be destroyed.
 *
 * @return
 *   - @ref V2F_E_NONE : Destruction was successful
 *   - @ref V2F_E_INVALID_PARAMETER : At least one parameter was invalid
 */
v2f_error_t v2f_entropy_decoder_destroy(v2f_entropy_decoder_t *const decoder);

/**
 * Decompress a block compressed with v2f_entropy_coder_compress_block using
 * the encoder corresponding to `decoder`.
 *
 * Note that the last emitted word might code more samples than there were in
 * the original block before compression. To guarantee a perfectly identical
 * reconstructed block and to avoid buffer overflows,
 * see the `max_output_sample_count` argument.
 *
 * @param decoder decoder to be used for decompression
 * @param compressed_block buffer of compressed data
 * @param compressed_size number of bytes to be decompressed from
 *   `compressed_block`.
 * @param reconstructed_samples pointer to the output sample buffer. It must be
 *   large enough to decode all samples in the first `compressed_size` bytes
 *   of `compressed_block`.
 * @param max_output_sample_count maximum number of samples that should be
 *   output by this method.
 * @param written_sample_count pointer to a counter where the number of
 *   output samples is stored. If NULL, it is ignored.
 * @return
 *  - @ref V2F_E_NONE : The block was successfully decompressed
 *  - @ref V2F_E_INVALID_PARAMETER : invalid parameter provided
 */
v2f_error_t v2f_entropy_decoder_decompress_block(
        v2f_entropy_decoder_t *const decoder,
        uint8_t const*const compressed_block,
        uint64_t compressed_size,
        v2f_sample_t *const reconstructed_samples,
        uint64_t max_output_sample_count,
        uint64_t *const written_sample_count);

/**
 * Decode the samples corresponding to the first encoded word in
 * compressed_block.
 *
 * It is assumed that indices were produced by concatenating the bytes
 * produced by @ref v2f_entropy_coder_fill_entry, i.e., using big-endian ordering
 * when applicable.
 *
 * @param decoder initialized entropy decoder to be used for decompression
 * @param compressed_block pointer to the next index position
 * @param output_samples buffer of samples with enough capacity to accommodate
 *   the largest sequence of samples encoded by any word of the encoder
 * @param samples_written pointer to a variable where the number
 *   of decoded samples represented by the index is to be stored.
 *   Ignored if NULL.
 *
 * @return
 *  - @ref V2F_E_NONE : The index was successfully decoded
 *  - @ref V2F_E_INVALID_PARAMETER : invalid parameter provided
 *  - @ref V2F_E_CORRUPTED_DATA : compressed data contained an invalid index,
 *   i.e., an index >= decoder->total_entry_count.
 */
v2f_error_t v2f_entropy_decoder_decode_next_index(
        v2f_entropy_decoder_t *const decoder,
        uint8_t const* const compressed_block,
        v2f_sample_t *const output_samples,
        uint32_t *const samples_written);

#endif /* V2F_ENTROPY_DECODER_H */
