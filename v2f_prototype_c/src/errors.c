/**
 * @file
 *
 * Implementation of the interface for consistent error management.
 *
 * @see errors.h for further details and usage information.
 *
 */

#include "errors.h"

#include <assert.h>

#include "v2f.h"

/**
 * Table to convert from an v2f_error_t constant value to its literal representation.
 */
static
char const *const v2f_error_strings[] = {
        "V2F_E_NONE",
        "V2F_E_UNEXPECTED_END_OF_FILE",
        "V2F_E_IO",
        "V2F_E_CORRUPTED_DATA",
        "V2F_E_INVALID_PARAMETER",
        "V2F_E_NON_ZERO_RESERVED_OR_PADDING",
        "V2F_E_UNABLE_TO_CREATE_TEMPORARY_FILE",
        "V2F_E_OUT_OF_MEMORY",
        "V2F_E_FEATURE_NOT_IMPLEMENTED",
};

// LCOV_EXCL_START

char const *v2f_strerror(v2f_error_t error) {
    assert(sizeof(v2f_error_strings) ==
           (V2F_E_FEATURE_NOT_IMPLEMENTED + 1) * sizeof(char *));
    assert(error >= 0 && error <= V2F_E_FEATURE_NOT_IMPLEMENTED);

    return v2f_error_strings[error];
}

// LCOV_EXCL_STOP
