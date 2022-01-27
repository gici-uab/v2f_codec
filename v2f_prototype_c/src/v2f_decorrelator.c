/**
 * @file
 * @author Miguel Hernández Cabronero <miguel.hernandez@uab.cat>
 * @date 08/03/2021
 *
 * Implementation of the decorrelation interfaces.
 */

#include "v2f_decorrelator.h"

#include <assert.h>
#include <stdlib.h>

#include "log.h"
#include "timer.h"
#include "log.h"
#include "common.h"

v2f_error_t v2f_decorrelator_create(
        v2f_decorrelator_t *decorrelator,
        v2f_decorrelator_mode_t mode,
        v2f_sample_t max_sample_value) {

    if (decorrelator == NULL
        || mode > V2F_C_DECORRELATOR_MODE_JPEG_LS
        || (decorrelator != V2F_C_DECORRELATOR_MODE_NONE &&
            max_sample_value < 1)) {
        log_error("decorrelator = %p", (void *) decorrelator);
        log_error("mode = %u", mode);
        log_error("max_sample_value = %u", max_sample_value);
        return V2F_E_INVALID_PARAMETER;
    }

    decorrelator->mode = mode;
    decorrelator->max_sample_value = max_sample_value;
    decorrelator->samples_per_row = 0;

    return V2F_E_NONE;
}

v2f_sample_t v2f_decorrelator_map_predicted_sample(
        v2f_sample_t sample,
        v2f_sample_t prediction,
        v2f_sample_t max_sample_value) {
    assert(sample <= max_sample_value);
    assert(prediction <= max_sample_value);

    v2f_signed_sample_t prediction_difference =
            (v2f_signed_sample_t) ((int64_t) sample - (int64_t) prediction);

    const v2f_sample_t diff_to_max = max_sample_value - prediction;
    const v2f_sample_t closer_to_max = diff_to_max < prediction;

    const v2f_sample_t theta =
            (closer_to_max * diff_to_max)
            + ((!closer_to_max) * prediction);

    const v2f_sample_t abs_value = (v2f_sample_t) labs(prediction_difference);
    const v2f_sample_t within_pm_theta = (abs_value <= theta);
    const v2f_sample_t negative = (prediction_difference < 0);

    const v2f_sample_t coded_value = (v2f_sample_t) (
            (within_pm_theta * ((abs_value << 1) - negative))
            + ((!within_pm_theta) * (theta + abs_value)));

    return coded_value;
}

v2f_sample_t v2f_decorrelator_unmap_sample(
        v2f_sample_t coded_value,
        v2f_sample_t prediction,
        v2f_sample_t max_sample_value) {

    assert(prediction <= max_sample_value);

    v2f_sample_t theta =
            (prediction <= max_sample_value - prediction) ?
            prediction : max_sample_value - prediction;

    v2f_signed_sample_t prediction_error;

    if (coded_value <= (theta << 1)) {
        if (coded_value % 2 == 0) {
            prediction_error = (v2f_signed_sample_t) (coded_value >> 1);
        } else {
            prediction_error = (v2f_signed_sample_t) (-(
                    ((int64_t) (coded_value + 1)) >> 1));
        }
    } else {
        if (theta == prediction) {
            prediction_error = (v2f_signed_sample_t) ((int64_t) coded_value -
                                                      (int64_t) theta);
        } else {
            assert(theta == max_sample_value - prediction);
            prediction_error = (v2f_signed_sample_t) ((int64_t) theta -
                                                      (int64_t) coded_value);
        }
    }

    return (v2f_sample_t) ((int64_t) prediction + (int64_t) prediction_error);
}

v2f_error_t v2f_decorrelator_decorrelate_block(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count) {
    timer_start("v2f_decorrelator_decorrelate_block");
    if (decorrelator == NULL || input_samples == NULL || sample_count == 0) {
        return V2F_E_INVALID_PARAMETER;
    }
    if (decorrelator->mode >= V2F_C_DECORRELATOR_MODE_COUNT) {
        return V2F_E_NONE;
    }

    v2f_error_t status;
    switch (decorrelator->mode) {
        case V2F_C_DECORRELATOR_MODE_NONE:
            status = V2F_E_NONE;
            break;
        case V2F_C_DECORRELATOR_MODE_LEFT:
            status = v2f_decorrelator_apply_left_prediction(
                    decorrelator, input_samples, sample_count);
            break;
        case V2F_C_DECORRELATOR_MODE_2_LEFT:
            status = v2f_decorrelator_apply_2_left_prediction(
                    decorrelator, input_samples, sample_count);
            break;
        case V2F_C_DECORRELATOR_MODE_JPEG_LS:
            status = v2f_decorrelator_apply_jpeg_ls_prediction(
                    decorrelator, input_samples, sample_count);
            break;
            // LCOV_EXCL_START
        default:
            abort();
            // LCOV_EXCL_STOP
    }
    timer_stop("v2f_decorrelator_decorrelate_block");
    return status;
}

v2f_error_t v2f_decorrelator_invert_block(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count) {
    if (decorrelator == NULL || input_samples == NULL || sample_count == 0) {
        return V2F_E_INVALID_PARAMETER;
    }
    if (decorrelator->mode > V2F_C_DECORRELATOR_MODE_JPEG_LS) {
        return V2F_E_NONE;
    }

    v2f_error_t status;
    switch (decorrelator->mode) {
        case V2F_C_DECORRELATOR_MODE_NONE:
            status = V2F_E_NONE;
            break;
        case V2F_C_DECORRELATOR_MODE_LEFT:
            status = v2f_decorrelator_inverse_left_prediction(
                    decorrelator, input_samples, sample_count);
            break;
        case V2F_C_DECORRELATOR_MODE_2_LEFT:
            status = v2f_decorrelator_inverse_2_left_prediction(
                    decorrelator, input_samples, sample_count);
            break;
        case V2F_C_DECORRELATOR_MODE_JPEG_LS:
            status = v2f_decorrelator_inverse_jpeg_ls_prediction(
                    decorrelator, input_samples, sample_count);
            break;
        default:
            status = V2F_E_INVALID_PARAMETER;
    }

    return status;
}

v2f_error_t v2f_decorrelator_apply_left_prediction(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count) {
    // Verify parameters and define needed constants.
    if (decorrelator == NULL
        || input_samples == NULL
        || sample_count == 0
        || decorrelator->mode != V2F_C_DECORRELATOR_MODE_LEFT
        || ((decorrelator->samples_per_row > 0
             && (sample_count % decorrelator->samples_per_row != 0)))
            ) {
        return V2F_E_INVALID_PARAMETER;
    }
    const v2f_sample_t max_sample_value = decorrelator->max_sample_value;

    // The first sample is predicted to be exactly zero.
    v2f_sample_t prediction = 0;
    for (uint64_t sample_index = 0;
         sample_index < sample_count; sample_index++) {
        const v2f_sample_t tmp = input_samples[sample_index];

        if (input_samples[sample_index] > max_sample_value) {
            log_error("Encountered input sample input_samples[%lu]=%u "
                      "> max_sample_value=%u",
                      sample_index,
                      input_samples[sample_index],
                      max_sample_value);
            return V2F_E_CORRUPTED_DATA;
        }

        input_samples[sample_index] = v2f_decorrelator_map_predicted_sample(
                input_samples[sample_index],
                prediction,
                max_sample_value);

        prediction = tmp;
    }

    return V2F_E_NONE;
}

v2f_error_t v2f_decorrelator_inverse_left_prediction(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count) {
    // Verify parameters and define needed constants.
    if (decorrelator == NULL
        || input_samples == NULL
        || sample_count == 0
        || decorrelator->mode != V2F_C_DECORRELATOR_MODE_LEFT
        || ((decorrelator->samples_per_row > 0
             && (sample_count % decorrelator->samples_per_row != 0)))
            ) {
        return V2F_E_INVALID_PARAMETER;
    }

    const v2f_sample_t max_sample_value = decorrelator->max_sample_value;

    // The first sample of each sample_index is predicted to be exactly zero.
    v2f_sample_t prediction = 0;
    for (uint64_t sample_index = 0;
         sample_index < sample_count; sample_index++) {
        input_samples[sample_index] =
                v2f_decorrelator_unmap_sample(
                        input_samples[sample_index],
                        prediction,
                        decorrelator->max_sample_value);

        if (input_samples[sample_index] > max_sample_value) {
            log_error("Decoded invalid sample input_samples[%lu]=%u > "
                      "max_sample_value=%u",
                      sample_index,
                      input_samples[sample_index],
                      max_sample_value);
            return V2F_E_CORRUPTED_DATA;
        }

        prediction = input_samples[sample_index];
    }
    return V2F_E_NONE;
}

v2f_error_t v2f_decorrelator_apply_2_left_prediction(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count) {
    // Verify parameters and define needed constants.
    if (decorrelator == NULL
        || input_samples == NULL
        || sample_count == 0
        || (decorrelator->samples_per_row > 0
            && decorrelator->samples_per_row < 3)
        || decorrelator->mode != V2F_C_DECORRELATOR_MODE_2_LEFT
        || ((decorrelator->samples_per_row > 0
             && (sample_count % decorrelator->samples_per_row != 0)))) {
        return V2F_E_INVALID_PARAMETER;
    }
    const v2f_sample_t max_sample_value = decorrelator->max_sample_value;

    // Prediction starts with zero
    v2f_sample_t left_left_neighbor = 0;
    v2f_sample_t left_neighbor = 0;
    for (uint64_t sample_index = 0;
         sample_index < sample_count; sample_index++) {
        const v2f_sample_t tmp = input_samples[sample_index];

        // Prediction is the rounded average of the two left neighbors
        const v2f_sample_t prediction = (
                (left_neighbor + left_left_neighbor + 1) >> 1);

        // Decode and verify sample
        input_samples[sample_index] = v2f_decorrelator_map_predicted_sample(
                tmp, prediction, max_sample_value);


        if (input_samples[sample_index] > max_sample_value) {
            log_error("Encountered input sample input_samples[%lu]=%u >"
                      "max_sample_value=%u",
                      sample_index,
                      input_samples[sample_index],
                      max_sample_value);
            return V2F_E_CORRUPTED_DATA;
        }

        // Update stored references to previous values
        left_left_neighbor = left_neighbor;
        left_neighbor = tmp;
    }

    return V2F_E_NONE;
}

v2f_error_t v2f_decorrelator_inverse_2_left_prediction(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count) {
    // Verify parameters and define needed constants.
    if (decorrelator == NULL
        || input_samples == NULL
        || sample_count == 0
        || (decorrelator->samples_per_row > 0
            && decorrelator->samples_per_row < 3)
        || decorrelator->mode != V2F_C_DECORRELATOR_MODE_2_LEFT
        || ((decorrelator->samples_per_row > 0
             && (sample_count % decorrelator->samples_per_row != 0)))) {
        return V2F_E_INVALID_PARAMETER;
    }

    const v2f_sample_t max_sample_value = decorrelator->max_sample_value;

    // Prediction starts with zero
    v2f_sample_t left_left_neighbor = 0;
    v2f_sample_t left_neighbor = 0;

    for (uint64_t sample_index = 0;
         sample_index < sample_count; sample_index++) {
        const v2f_sample_t prediction = (
                (left_neighbor + left_left_neighbor + 1) >> 1);

        input_samples[sample_index] =
                v2f_decorrelator_unmap_sample(
                        input_samples[sample_index],
                        prediction,
                        max_sample_value);
        if (input_samples[sample_index] > max_sample_value) {
            log_error("Decoded invalid sample");
        }

        left_left_neighbor = left_neighbor;
        left_neighbor = input_samples[sample_index];
    }

    return V2F_E_NONE;
}

v2f_error_t v2f_decorrelator_apply_jpeg_ls_prediction(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count) {
    // Verify parameters and define needed constants.
    if (decorrelator == NULL
        || input_samples == NULL
        || sample_count == 0
        || (decorrelator->samples_per_row > 0
            && decorrelator->samples_per_row < 3)
        || decorrelator->mode != V2F_C_DECORRELATOR_MODE_JPEG_LS
        || ((decorrelator->samples_per_row > 0
             && (sample_count % decorrelator->samples_per_row != 0)))) {
        return V2F_E_INVALID_PARAMETER;
    }

    if (decorrelator->samples_per_row == 0) {
        decorrelator->samples_per_row = sample_count;
    };
    const v2f_sample_t max_sample_value = decorrelator->max_sample_value;

    // Prediction starts with zero
    v2f_sample_t left_neighbor = 0;
    v2f_sample_t north_neighbor = 0;
    v2f_sample_t left_north_neighbor = 0;
    v2f_sample_t prediction;

    v2f_sample_t * previous_row = (v2f_sample_t *) malloc(
            sizeof(v2f_sample_t) * decorrelator->samples_per_row);
    if (previous_row == NULL) {
        log_error(
                "Error allocating buffers with limits %ld.\n",
                sizeof(v2f_sample_t) * decorrelator->samples_per_row);
        return V2F_E_OUT_OF_MEMORY;
    }
    memset(previous_row, 0, sizeof(v2f_sample_t) * decorrelator->samples_per_row);

    for (uint64_t sample_index = 0;
         sample_index < sample_count; sample_index++) {
        const v2f_sample_t tmp = input_samples[sample_index];
        const uint64_t current_row = sample_index/(decorrelator->samples_per_row);

        previous_row[sample_index - decorrelator->samples_per_row * current_row] = tmp;

        if (left_north_neighbor >= MAX(left_neighbor, north_neighbor)) {
            prediction = MIN(left_neighbor, north_neighbor);
        } else if (left_north_neighbor <= MIN(left_neighbor, north_neighbor)) {
            prediction = MAX(left_neighbor, north_neighbor);
        } else {
            prediction = left_neighbor + north_neighbor - left_north_neighbor;
        }

        // Update stored references to previous values
        left_neighbor = tmp;
        left_north_neighbor = north_neighbor;
        north_neighbor = previous_row[sample_index + 1 - decorrelator->samples_per_row *
                                     ((sample_index + 1)/(decorrelator->samples_per_row))];

        // Decode and verify sample
        input_samples[sample_index] = v2f_decorrelator_map_predicted_sample(
                tmp, prediction, max_sample_value);

        if (input_samples[sample_index] > max_sample_value) {
            log_error("Encountered input sample input_samples[%lu]=%u >"
                      "max_sample_value=%u",
                      sample_index,
                      input_samples[sample_index],
                      max_sample_value);
            return V2F_E_CORRUPTED_DATA;
        }
    }

    return V2F_E_NONE;
}

v2f_error_t v2f_decorrelator_inverse_jpeg_ls_prediction(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count) {
    // Verify parameters and define needed constants.
    if (decorrelator == NULL
        || input_samples == NULL
        || sample_count == 0
        || (decorrelator->samples_per_row > 0
            && decorrelator->samples_per_row < 3)
        || decorrelator->mode != V2F_C_DECORRELATOR_MODE_JPEG_LS
        || ((decorrelator->samples_per_row > 0
             && (sample_count % decorrelator->samples_per_row != 0)))) {
        return V2F_E_INVALID_PARAMETER;
    }

    if (decorrelator->samples_per_row == 0) {
        decorrelator->samples_per_row = sample_count;
    }
    const v2f_sample_t max_sample_value = decorrelator->max_sample_value;

    // Prediction starts with zero
    v2f_sample_t left_neighbor = 0;
    v2f_sample_t north_neighbor = 0;
    v2f_sample_t left_north_neighbor = 0;
    v2f_sample_t prediction;
    for (uint64_t sample_index = 0;
         sample_index < sample_count; sample_index++) {

        if (left_north_neighbor >= MAX(left_neighbor, north_neighbor)) {
            prediction = MIN(left_neighbor, north_neighbor);
        } else if (left_north_neighbor <= MIN(left_neighbor, north_neighbor)) {
            prediction = MAX(left_neighbor, north_neighbor);
        }
        else {
            prediction = left_neighbor + north_neighbor - left_north_neighbor;
        }

        input_samples[sample_index] =
                v2f_decorrelator_unmap_sample(
                        input_samples[sample_index],
                        prediction,
                        max_sample_value);
        if (input_samples[sample_index] > max_sample_value) {
            log_error("Decoded invalid sample");
        }

        left_neighbor = input_samples[sample_index];
        left_north_neighbor = north_neighbor;
        if (sample_index + 1 >= decorrelator->samples_per_row) {
            north_neighbor = input_samples[sample_index + 1 -
                                           decorrelator->samples_per_row];
        }
    }

    return V2F_E_NONE;
}
