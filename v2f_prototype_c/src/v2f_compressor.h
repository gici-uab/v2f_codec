/**
 * @file
 *
 * @brief Module that implements an interface for applying a complete
 *   compression pipeline.
 */

#ifndef V2F_COMPRESSOR_H
#define V2F_COMPRESSOR_H

#include "v2f.h"
#include "v2f_quantizer.h"
#include "v2f_decorrelator.h"
#include "v2f_entropy_coder.h"

// Many v2f_* enums and structs are defined in v2f.h

/**
 * Initialize a compressor.
 *
 * @param compressor compressor to be initialized
 * @param quantizer initialized quantizer
 * @param decorrelator initialized decorrelator
 * @param entropy_coder initialized entropy coder
 * @return
 *  - @ref V2F_E_NONE : Creation successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_compressor_create(
        v2f_compressor_t *compressor,
        v2f_quantizer_t *quantizer,
        v2f_decorrelator_t *decorrelator,
        v2f_entropy_coder_t *entropy_coder);


/**
 * Compress the samples in `input_samples` and write the result to output_buffer
 * using the full pipeline of `compressor`.
 *
 * @param compressor intitialized compressor to be used for compression
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
v2f_error_t v2f_compressor_compress_block(
        v2f_compressor_t *const compressor,
        v2f_sample_t *const input_samples,
        uint64_t sample_count,
        uint8_t *const output_buffer,
        uint64_t *const written_byte_count);


#endif /* V2F_COMPRESSOR_H */
