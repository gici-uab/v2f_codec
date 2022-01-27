/**
 * @file
 * @author Miguel Hernández Cabronero <miguel.hernandez@uab.cat>
 * @date 08/03/2021
 *
 * Tools to apply quantization
 */

#include "v2f_quantizer.h"

#include <assert.h>

#include "log.h"
#include "timer.h"
#include "common.h"

v2f_error_t v2f_quantizer_create(
        v2f_quantizer_t *quantizer,
        v2f_quantizer_mode_t mode,
        v2f_sample_t step_size,
        v2f_sample_t max_sample_value) {
    if (step_size < 1) {
        log_error("step_size = %u", step_size);
        return V2F_E_INVALID_PARAMETER;
    }
    if (quantizer == NULL
        || (mode != V2F_C_QUANTIZER_MODE_NONE && step_size < 1)) {
        log_error("quantizer = %p", (void*) quantizer);
        log_error("step_size = %u", step_size);
        return V2F_E_INVALID_PARAMETER;
    }
    if (mode == V2F_C_QUANTIZER_MODE_NONE && step_size > 1) {
        log_error("mode = %u", mode);
        log_error("step_size = %u", step_size);
        return V2F_E_INVALID_PARAMETER;
    }
    if (mode > V2F_C_QUANTIZER_MODE_UNIFORM) {
        log_error("mode = %u", mode);
        return V2F_E_INVALID_PARAMETER;
    }
    if (step_size > V2F_C_QUANTIZER_MODE_MAX_STEP_SIZE) {
        log_error("step_size = %u", step_size);
        return V2F_E_INVALID_PARAMETER;
    }
    if (max_sample_value > V2F_C_MAX_SAMPLE_VALUE) {
        log_error("max_sample_value = %u", max_sample_value);
        return V2F_E_INVALID_PARAMETER;
    }

    quantizer->mode = mode;
    quantizer->step_size = step_size;
    quantizer->max_sample_value = max_sample_value;

    return V2F_E_NONE;
}

v2f_error_t v2f_quantizer_quantize(
        v2f_quantizer_t *const quantizer,
        v2f_sample_t *const input_samples,
        uint64_t sample_count) {
    if (quantizer == NULL || input_samples == NULL || sample_count < 1) {
        return V2F_E_INVALID_PARAMETER;
    }

    timer_start("v2f_quantizer_quantize");

    if (quantizer->mode == V2F_C_QUANTIZER_MODE_NONE || quantizer->step_size == 1) {
        timer_stop("v2f_quantizer_quantize");
        return V2F_E_NONE;
    }

    v2f_error_t status;
    switch (quantizer->mode) {
        case V2F_C_QUANTIZER_MODE_NONE:
            status = V2F_E_NONE;
            break;
        case V2F_C_QUANTIZER_MODE_UNIFORM:
            switch(quantizer->step_size) {
                case 2:
                    status = v2f_quantize_apply_uniform_shift(
                            1, input_samples, sample_count);
                    break;
                case 4:
                    status = v2f_quantize_apply_uniform_shift(
                            2, input_samples, sample_count);
                    break;
                case 8:
                    status = v2f_quantize_apply_uniform_shift(
                            3, input_samples, sample_count);
                    break;
                default:
                    status = v2f_quantizer_apply_uniform_division(
                            quantizer->step_size,
                            input_samples,
                            sample_count);
                    break;
            }
            break;
        default:
            status = V2F_E_INVALID_PARAMETER; // LCOV_EXCL_LINE
            break; // LCOV_EXCL_LINE
    }

    timer_stop("v2f_quantizer_quantize");

    return status;
}

v2f_error_t v2f_quantizer_dequantize(
        v2f_quantizer_t *const quantizer,
        v2f_sample_t *const input_samples,
        uint64_t sample_count) {
    if (quantizer == NULL || input_samples == NULL || sample_count < 1) {
        return V2F_E_INVALID_PARAMETER;
    }

    if (quantizer->mode == V2F_C_QUANTIZER_MODE_NONE || quantizer->step_size == 1) {
        return V2F_E_NONE;
    }

    v2f_sample_t status;
    switch (quantizer->mode) {
        case V2F_C_QUANTIZER_MODE_NONE:
        case V2F_C_QUANTIZER_MODE_UNIFORM:
            status = v2f_quantizer_inverse_uniform(
                    quantizer->step_size, input_samples, sample_count, quantizer->max_sample_value);
            break;
        default:
            status = V2F_E_INVALID_PARAMETER; // LCOV_EXCL_LINE
            break; // LCOV_EXCL_LINE
    }

    return status;
}

v2f_error_t v2f_quantizer_apply_uniform_division(
        v2f_sample_t step_size,
        v2f_sample_t *const input_samples,
        uint64_t sample_count) {
    log_debug("applying uniform division step = %u", step_size);

    if (sample_count == UINT64_MAX || step_size <= 1) {
        return V2F_E_INVALID_PARAMETER;
    }

    for (uint64_t i = 0; i < sample_count; i++) {
        input_samples[i] /= step_size;
    }

    return V2F_E_NONE;
}

v2f_error_t v2f_quantize_apply_uniform_shift(
        v2f_sample_t shift,
        v2f_sample_t *const input_samples,
        uint64_t sample_count) {
    log_debug("appying uniform shift = %u", shift);

    if (sample_count == UINT64_MAX || shift < 1) {
        return V2F_E_INVALID_PARAMETER;
    }

    for (uint64_t i = 0; i < sample_count; i++) {
        input_samples[i] >>= shift;
    }

    return V2F_E_NONE;
}

v2f_error_t v2f_quantizer_inverse_uniform(
        v2f_sample_t step_size,
        v2f_sample_t *const input_samples,
        uint64_t sample_count,
        v2f_sample_t max_sample_value) {
    log_debug("apply inverse quantization step_size = %u", step_size);

    if (step_size < 1 || input_samples == NULL || sample_count == 0) {
        return V2F_E_INVALID_PARAMETER;
    }
    if (step_size == 1) {
        return V2F_E_NONE;
    }

    assert(sample_count < UINT64_MAX);
    for (uint64_t i = 0; i < sample_count; i++) {
        input_samples[i] = step_size * input_samples[i] + (step_size >> 1);
        if (input_samples[i] > max_sample_value) {
            // Avoid exceeding the dynamic range due to the last quantization
            // bin being incomplete
            assert(step_size > 1);
//            assert((input_samples[i] - max_sample_value) <= (step_size >> 1));
            input_samples[i] = max_sample_value;
        }
    }

    return V2F_E_NONE;
}
