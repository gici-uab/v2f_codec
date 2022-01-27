/**
 * @file
 * 
 * @brief Module that implements an interface for applying a complete
 *   decompression pipeline.
 */

#ifndef V2F_DECOMPRESSOR_H
#define V2F_DECOMPRESSOR_H

#include "v2f.h"
#include "v2f_quantizer.h"
#include "v2f_decorrelator.h"
#include "v2f_entropy_decoder.h"

// Many v2f_* enums and structs are defined in v2f.h

/**
 * Initialize a decompressor.
 *
 * @param decompressor decompressor to be initialized
 * @param quantizer initialized quantizer
 * @param decorrelator initialized decorrelator
 * @param entropy_decoder initialized entropy decoder
 * @return
 *  - @ref V2F_E_NONE : Creation successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_decompressor_create(
        v2f_decompressor_t *decompressor,
        v2f_quantizer_t *quantizer,
        v2f_decorrelator_t *decorrelator,
        v2f_entropy_decoder_t *entropy_decoder);


/**
 * Decompress the codewords in `compressed_data` and write the
 * result to `reconstructed_samples` using the full decompression pipeline.
 *
 * @param decompressor intitialized decompressor to be used for compression.
 * @param compressed_data buffer with the codewords to be decompressed.
 * @param buffer_size_bytes number of bytes in `compressed_data`.
 * @param max_output_sample_count maximum number of samples that will
 *  be written to `reconstructed_samples` (if available).
 * @param reconstructed_samples buffer where the reconstructed samples
 *  are to be writen. The caller is responsible to pass a large enough
 *  buffer.
 * @param written_sample_count pointer where the number of reconstructed
 *   samples is to be written. If the pointer is NULL, it is ignored.
 *
 * @return
  *  - @ref V2F_E_NONE : Decompression successfull
  *  - @ref V2F_E_IO : Input/output error
  *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_decompressor_decompress_block(
        v2f_decompressor_t *const decompressor,
        uint8_t *const compressed_data,
        uint64_t buffer_size_bytes,
        uint64_t max_output_sample_count,
        v2f_sample_t *const reconstructed_samples,
        uint64_t *const written_sample_count);


#endif /* V2F_DECOMPRESSOR_H */
