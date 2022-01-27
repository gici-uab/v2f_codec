/**
 * @file v2f_compressor.c
 * @author Miguel Hernández Cabronero <miguel.hernandez@uab.cat>
 * @date 08/03/2021
 *
 *
 */

#include "v2f_compressor.h"
#include "timer.h"

v2f_error_t v2f_compressor_create(
        v2f_compressor_t *compressor,
        v2f_quantizer_t *quantizer,
        v2f_decorrelator_t *decorrelator,
        v2f_entropy_coder_t *entropy_coder) {
    if (compressor == NULL ||
        decorrelator == NULL || quantizer == NULL || entropy_coder == NULL) {
        return V2F_E_INVALID_PARAMETER;
    }

    compressor->quantizer = quantizer;
    compressor->decorrelator = decorrelator;
    compressor->entropy_coder = entropy_coder;

    return V2F_E_NONE;
}

v2f_error_t v2f_compressor_compress_block(
        v2f_compressor_t *const compressor,
        v2f_sample_t *const input_samples,
        uint64_t sample_count,
        uint8_t *const output_buffer,
        uint64_t *const written_byte_count) {

    timer_start("v2f_compressor_compress_block");

    RETURN_IF_FAIL(v2f_quantizer_quantize(
            compressor->quantizer, input_samples, sample_count));


    RETURN_IF_FAIL(v2f_decorrelator_decorrelate_block(
            compressor->decorrelator, input_samples, sample_count));


    RETURN_IF_FAIL(v2f_entropy_coder_compress_block(
            compressor->entropy_coder, input_samples, sample_count,
            output_buffer, written_byte_count));

    timer_stop("v2f_compressor_compress_block");

    return V2F_E_NONE;
}
