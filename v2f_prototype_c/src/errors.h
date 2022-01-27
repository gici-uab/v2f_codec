/**
 * @file
 *
 * @brief Interface for consistent error management.
 *
 * Internal-facing functions may assert() or abort() for causes of incorrect function usage.
 * However, functions of the public API (i.e., those declared as @a V2F_EXPORTED_SYMBOL in @ref v2f.h)
 * shall never crash. Instead, they should return an error that can be handled externally.
 *
 * The protocol used in this implementation to achieve this goal is the definition of @ref v2f_error_t
 * as return type for most internal and external functions, plus the use of the @ref RETURN_IF_FAIL macro
 * to provide an exception-like behavior in case an error is detected. On success, functions are expected
 * to return V2F_E_NONE.
 *
 * Usage:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * v2f_error_t example_function(...) {
 *
 *    // Both function1 and function2 conform to the aforementioned protocol
 *    // and have v2f_error_t return type
 *    RETURN_IF_FAIL(function1(...));
 *    RETURN_IF_FAIL(function2(...));
 *
 *    return V2F_E_NONE;
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef ERRORS_H
#define ERRORS_H

#include <stdint.h>

#include "v2f.h"

/* A few conventions/rules follow:
 *
 * 1-- On errors
 *
 * Functions are supposed to always work properly. If there is an issue with an internal facing function (not on the
 * public API) we fail (assert() or abort()) for causes of incorrect function usage, but return an error code for any
 * other cause.
 * External facing functions never assert or abort for any invalid parameter combination.
 *
 * Out of memory, disc failures, etc. are all errors we report to the api user. We never crash the program, and we let
 * the API user decide what to do with our error code.
 *
 * No resource leakage shall occur when an error is returned (and open files or allocated memory must be released).
 *
 * 2-- GOTO usage
 *
 * 	We do Resource-Allocation-Is-Initialization (RAII) for this we employ the common design pattern of a
 * 	goto chain to release partially allocated resources. Other than this GOTO is not allowed.
 *
 * 3-- Function names, namespaces
 *
 * Function names should be imperatives (do_this), except short common names (sgn).
 * Functions and datatypes should be prefixed by v2f_.
 *
 * 4-- Charset
 *
 * Source files shall contain only 7bit ascii, except for comment sections where utf-8 is allowed.
 *
 * 5-- Parameter order
 *
 * inputs, outputs, class_state_variable, configuration.
 *
 * Only functions that never fail can have an output as return value, and only if there is only one return value.
 * Otherwise, return values are by reference (caller must ensure it fits).
 *
 * 6-- Book references.
 *
 * Book sections are referenced by a § followed by the section number following by the section name (if any)
 * in the same case as in the book. There must be a space after § and after the section number. No dot between
 * section number and section name.
 *
 * 7-- Type qualifiers
 *
 * "const" should be used all around (including pointers, e.g., v2f_entropy_coder_t *const).
 * "restrict" when needed too.
 *
 * 8-- Memory
 *
 * We try not to do dynamic memory allocation (find some reasons in MISRA).
 * No destruction is needed most of the time. Hard-coded upper memory bound.
 *
 * mallocs may be used in some operations where having a maximum value for all reasonable
 * variables leads to very large worst case buffer sizes.
 *
 * 9-- Data types
 *
 * We use signed datatypes unless there is a good reason to use another type
 * (cast from non-negative signed to unsigned is always lossless). Buffers of chars are measured in size_t.
 *
 * 10-- Documentation
 *
 * In doxygen documentation should go what something does and how to use it. Implementation details should go in
 * regular comments.
 *
 * 11-- Shifts
 *
 * Read C99 § 6.5.7.
 *
 * No negative shifts (e.g., << -3).
 * No shifts by width or more bits.
 * No right shifts of negative values (!= by negative values).
 * No left shift of signed values that affect sign bit (result shall be representable).
 *
 * 12-- Source formating
 *
 * Lines are 120 columns wide (technically 119, as last column should always be empty).
 *
 */


// C99 has no exception system and some pieces of code become very very very messy otherwise.
/**
 * Return from the current function if a given v2f_error_t value is not V2F_E_NONE.
 *
 * Given a function v2f_error_t f(...) usage is as follows:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * RETURN_IF_FAIL(f(...));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This is equivalent to writing:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * {
 *     v2f_error_t return_value = f(..);
 *
 *     if (return_value != V2F_E_NONE) {
 *         return return_value; 
 *     }
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param x v2f_error_t value to be evaluated.
 */
#define RETURN_IF_FAIL(x) do { const v2f_error_t local_return_value = (x); \
    if(local_return_value != V2F_E_NONE) return local_return_value; } while(0)

/**
 * Return a string representing the v2f_error_t passed as argument.
 * It must be a valid error value.
 * @param error error to be represented
 * @return a string
 */
char const *v2f_strerror(v2f_error_t error);

#endif
