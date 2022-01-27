/**
 * @file
 * @author Miguel Hernández Cabronero <miguel.hernandez@uab.cat>
 * @date 08/03/2021
 *
 *
 */

#include "v2f_decompressor.h"
#include "timer.h"
#include "log.h"

v2f_error_t v2f_decompressor_create(
        v2f_decompressor_t *decompressor,
        v2f_quantizer_t *quantizer,
        v2f_decorrelator_t *decorrelator,
        v2f_entropy_decoder_t *entropy_decoder) {
    if (decompressor == NULL ||
        decorrelator == NULL || quantizer == NULL || entropy_decoder == NULL) {
        return V2F_E_INVALID_PARAMETER;
    }

    decompressor->quantizer = quantizer;
    decompressor->decorrelator = decorrelator;
    decompressor->entropy_decoder = entropy_decoder;

    return V2F_E_NONE;
}

v2f_error_t v2f_decompressor_decompress_block(
        v2f_decompressor_t *const decompressor,
        uint8_t *const compressed_data,
        uint64_t buffer_size_bytes,
        uint64_t max_output_sample_count,
        v2f_sample_t *const reconstructed_samples,
        uint64_t *const written_sample_count) {

    timer_start("v2f_decompressor_decompress_block");

    {
        timer_start("v2f_entropy_decoder_decompress_block");
        RETURN_IF_FAIL(v2f_entropy_decoder_decompress_block(
                decompressor->entropy_decoder, compressed_data,
                buffer_size_bytes,
                reconstructed_samples, max_output_sample_count, written_sample_count));
        timer_stop("v2f_entropy_decoder_decompress_block");

        timer_start("v2f_decorrelator_invert_block");
        RETURN_IF_FAIL(v2f_decorrelator_invert_block(
                decompressor->decorrelator, reconstructed_samples,
                *written_sample_count));
        timer_stop("v2f_decorrelator_invert_block");

        timer_start("v2f_quantizer_dequantize");
        RETURN_IF_FAIL(v2f_quantizer_dequantize(
                decompressor->quantizer, reconstructed_samples,
                *written_sample_count));
        timer_stop("v2f_quantizer_dequantize");
    }

    timer_stop("v2f_decompressor_decompress_block");

    return V2F_E_NONE;
}
