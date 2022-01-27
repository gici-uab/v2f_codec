/**
 * @file
 *
 * @brief This file exports a few miscellaneous functions and macros.
 *
 * Among the exported functions there are redefinitions of the abs function in stdlib.h to make sure it has the proper
 * data width, or definitions of minimum and maximum functions as, even if they are usually provided, they are not in
 * C99.
 *
 */
#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>

/// Shorthand for the max operation
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
/// Shorthand for the min operation
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/**
 * Read the value of a single bit in @a buffer, at a given @a index position.
 *
 * Note that @a index 0 indicates the MSB of the first byte.
 *
 * @param buffer buffer to be read.
 * @param index position within the buffer.
 *
 * @return 0 or 1, depending on the value of the selected bit.
 */
uint32_t v2f_get_bit(uint8_t const *const buffer, const uint32_t index);

/**
 * Set a single bit value in the selected position of buffer.
 *
 * Note that indexing semantics are identical to those of @ref v2f_get_bit,
 * i.e., @a index 0 indicates the MSB of the first byte.
 *
 * @param buffer buffer where the value is to be set.
 * @param index position within buffer where the value is to be set.
 * @param value value to be assigned to the selected position (must be 0 or 1).
 */
void v2f_set_bit(uint8_t *const buffer, const uint32_t index,
                    const uint32_t value);

/**
 * Silences an unused variable warning. This macro shall be employed when a function for some
 * reason must have an argument that must not use.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * void function(int unused) {
 *     V2F_SILENCE_UNUSED(unused);
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param x variable to silence
 */
#define V2F_SILENCE_UNUSED(x) ((void) (x))

/**
 * Silences an unused variable warning when variable is only used by an assert. This macro shall be employed when code
 * is complied with NDEBUG defined to disable warnings resulting from variables that are only used an assertion.
 * As the assertion is disabled, an unused variable appears.
 *
 * This macro does nothing when NDEBUG is defined.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * void function(int min, int max) {
 *     int difference = max - min;
 *
 *     assert(difference > 0);
 *
 *     V2F_SILENCE_ONLY_USED_BY_ASSERT(difference);
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param x variable to silence
 */
#if NDEBUG
// If NDEBUG is set, this function disables unused variable warnings due to disabled assertions.
#define V2F_SILENCE_ONLY_USED_BY_ASSERT(x) ((void) (x))
#else
// In debug mode, this function does nothing.
#define V2F_SILENCE_ONLY_USED_BY_ASSERT(x)
#endif


/**
 * Check whether the first @a length_bits bits of @a vector are all zero.
 *
 * @param vector vector to be checked
 * @param length_bits number of bits to be checked from vector. Must be at least 1.
 *
 * @return true if and only if the vector is identically zero
 */
bool
v2f_is_all_zero(uint8_t const *const vector, const uint32_t length_bits);


/**
 * Show the contents of a vector
 *
 * @param name a name identifying the vector contents
 * @param vector vector to be analyzed
 * @param vector_length_bits lenght of the vector in bits
 */
void debug_show_vector_contents(char *name, uint8_t *vector,
                                uint32_t vector_length_bits);

#endif
