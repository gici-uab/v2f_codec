/**
 * @file
 *
 * @brief Module that provides quantization tools.
 */

#ifndef V2F_QUANTIZER_H
#define V2F_QUANTIZER_H

#include "v2f.h"

// Many v2f_* enums and structs are defined in v2f.h

/**
 * Initialize a quantizer.
 *
 * @param quantizer pointer to the quantizer to initialize.
 * @param mode unique mode id for this quantizer.
 * @param step_size quantization step size. Must be at least 1.
 *   If 1, no quantization is performed.
 * @param max_sample_value maximum allowed input sample value. Needed to
 *   perform a valid dequantization within the expected dynamic range.
 * @return
 *  - @ref V2F_E_NONE : Creation successful
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_quantizer_create(
        v2f_quantizer_t *quantizer,
        v2f_quantizer_mode_t mode,
        v2f_sample_t step_size,
        v2f_sample_t max_sample_value);

/**
 * Quantize all samples in the block.
 *
 * @param quantizer pointer to the quantizer to be used
 * @param input_samples samples to be quantized (they are modified)
 * @param sample_count number of samples to be quantized
 * @return
 *  - @ref V2F_E_NONE : Quantization successful
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_quantizer_quantize(
        v2f_quantizer_t *const quantizer,
        v2f_sample_t *const input_samples,
        uint64_t sample_count);

/**
 * Dequantize all samples in the block.
 *
 * @param quantizer pointer to the quantizer to be used.
 * @param input_samples quantization indices to be dequantized.
 * @param sample_count number of samples to dequantize.
 * @return
 *  - @ref V2F_E_NONE : Dequantization successful
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_quantizer_dequantize(
        v2f_quantizer_t *const quantizer,
        v2f_sample_t *const input_samples,
        uint64_t sample_count);

/**
 * Apply uniform quantization by dividing each sample
 * @param step_size number to be used when dividing
 * @param input_samples samples to be quantized
 * @param sample_count number of samples to be quantized
 * @return
 *  - @ref V2F_E_NONE : Creation successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_quantizer_apply_uniform_division(
        v2f_sample_t step_size,
        v2f_sample_t *const input_samples,
        uint64_t sample_count);


/**
 * Apply uniform quantization by right-shifting each sample.
 *
 * @param shift number of bitplanes to erase.
 * @param input_samples samples to be quantized.
 * @param sample_count number of samples to be quantized.
 *
 * @return
 *  - @ref V2F_E_NONE : Creation successfull.
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter.
 */
v2f_error_t v2f_quantize_apply_uniform_shift(
        v2f_sample_t shift,
        v2f_sample_t *const input_samples,
        uint64_t sample_count);

/**
 * Apply inverse uniform quantization for a given step size.
 * The interval midpoint is used.
 *
 * @param step_size original step size.
 * @param input_samples buffer of quantization indices to be dequantized.
 * @param sample_count number of indices to dequantizer.
 * @param max_sample_value maximum sample value allowed before quantization.
 *   It is needed to reconstruct samples in the last quantization interval when
 *   it is not complete.
 * @return
 *  - @ref V2F_E_NONE : Creation successfull.
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter.
 */
v2f_error_t v2f_quantizer_inverse_uniform(
        v2f_sample_t step_size,
        v2f_sample_t *const input_samples,
        uint64_t sample_count,
        v2f_sample_t max_sample_value);

#endif /* V2F_QUANTIZER_H */
