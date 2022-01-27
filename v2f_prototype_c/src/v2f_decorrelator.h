/**
 * @file
 *
 * @brief Tools to apply decorrelation to some input data, e.g., prediction.
 *
 * These methods transform unsigned samples that may represent unsigned
 * values.
 */

#ifndef V2F_DECORRELATOR_H
#define V2F_DECORRELATOR_H

#include "v2f.h"

// Many v2f_* enums and structs are defined in v2f.h


/**
 * Initialize a decorrelator instance.
 *
 * @param decorrelator pointer to decorrelator to initialize
 * @param mode mode index that identifies this decorrelator
 * @param max_sample_value max original sample value
 * @return
 *  - @ref V2F_E_NONE : Creation successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_decorrelator_create(
        v2f_decorrelator_t *decorrelator,
        v2f_decorrelator_mode_t mode,
        v2f_sample_t max_sample_value);

/**
 * Code the prediction error of sample_value
 * given its prediction and maximum possible sample_value.
 * A similar coding mechanism is used as in CCSDS 123.0-B-2
 * (see http://dx.doi.org/10.1109/MGRS.2020.3048443 for more information).
 *
 * @param sample sample to be coded
 * @param prediction predicted value for this sample
 * @param max_sample_value maximum possible sample value
 *
 * @return the v2f_sample_t that represents the prediction error.
 */
v2f_sample_t v2f_decorrelator_map_predicted_sample(
        v2f_sample_t sample,
        v2f_sample_t prediction,
        v2f_sample_t max_sample_value);

/**
 * Decode the mapping applied by @ref v2f_decorrelator_map_predicted_sample
 * given a coded value and the same prediction used by that function.
 *
 * @param coded_value value coded by
 *   @ref v2f_decorrelator_map_predicted_sample.
 * @param prediction the predicted value for this sample.
 * @param max_sample_value maximum sample value used when coding the value.
 *
 * @return the sample_value passed to
 *   @ref v2f_decorrelator_map_predicted_sample.
 */
v2f_sample_t v2f_decorrelator_unmap_sample(
        v2f_sample_t coded_value,
        v2f_sample_t prediction,
        v2f_sample_t max_sample_value);

/**
 * Apply decorrelation to a block of samples.
 *
 * @param decorrelator initialized decorrelator
 * @param input_samples buffer of samples to decorrelate
 * @param sample_count number of samples in the buffer
 * @return
 *  - @ref V2F_E_NONE : Decorrelation successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_decorrelator_decorrelate_block(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count);

/**
 * Apply inverse decorrelation to a block of samples.
 *
 * @param decorrelator initialized decorrelator
 * @param input_samples buffer of samples to decorrelate
 * @param sample_count number of samples in the buffer
 * @return
 *  - @ref V2F_E_NONE : Creation successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_decorrelator_invert_block(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count);

/**
 * Apply DPCM decorrelation using the immediately previous sample
 * (prediction is 0 for the first sample of the block),
 * and store the prediction errors.
 *
 * @param decorrelator initialized decorrelator
 * @param input_samples data to be decorrelated
 * @param sample_count number of samples to be decorrelated
 * @return
 *  - @ref V2F_E_NONE : Creation successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_decorrelator_apply_left_prediction(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count);

/**
 * Apply inverse decorrelation to DPCM using the immediately previous sample
 * (prediction is assumed to have been 0 for the first sample of the block).
 *
 * @param decorrelator initialized decorrelator
 * @param input_samples data to be decorrelated
 * @param sample_count number of samples
 * @return
 *  - @ref V2F_E_NONE : Creation successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_decorrelator_inverse_left_prediction(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count);

/**
 * Apply DPCM decorrelation using the mean of the two previous samples,
 * with two exceptions:
 *
 * - The first sample of each row uses prediction 0
 * - The second sample of each row uses prediction equal to the first sample.
 *
 * @param decorrelator initialized decorrelator
 * @param input_samples data to be decorrelated
 * @param sample_count number of samples to be decorrelated
 * @return
 *  - @ref V2F_E_NONE : Creation successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_decorrelator_apply_2_left_prediction(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count);

/**
 * Apply inverse decorrelation to DPCM using the average of the two previous
 * samples (prediction is assumed to have been 0 for the first two
 * samples of the block).
 *
 * @param decorrelator initialized decorrelator
 * @param input_samples data to be decorrelated
 * @param sample_count number of samples
 * @return
 *  - @ref V2F_E_NONE : Creation successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_decorrelator_inverse_2_left_prediction(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count);

/**
 * Apply JPEG-LS decorrelation using:
 * - min(West, North) if North_West >= max(West, North)
 * - max(West, North) if North_West <= min(West, North)
 * - West + North - North_West otherwise
 *
 * - The first sample of each row and the entire first row uses prediction 0
 *
 * @param decorrelator initialized decorrelator
 * @param input_samples data to be decorrelated
 * @param sample_count number of samples to be decorrelated
 * @return
 *  - @ref V2F_E_NONE : Creation successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_decorrelator_apply_jpeg_ls_prediction(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count);

/**
 * Apply inverse decorrelation to JPEG-LS using:
 * - min(West, North) if North_West >= max(West, North)
 * - max(West, North) if North_West <= min(West, North)
 * - West + North - North_West otherwise
 *
 * @param decorrelator initialized decorrelator
 * @param input_samples data to be decorrelated
 * @param sample_count number of samples
 * @return
 *  - @ref V2F_E_NONE : Creation successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_decorrelator_inverse_jpeg_ls_prediction(
        v2f_decorrelator_t *decorrelator,
        v2f_sample_t *input_samples,
        uint64_t sample_count);

#endif /* V2F_DECORRELATOR_H */
