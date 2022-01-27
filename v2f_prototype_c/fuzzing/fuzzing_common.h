/**
 * @file
 *
 * @brief Declaration of utility functions common to several fuzzers.
 */

#ifndef FUZZING_COMMON_H
#define FUZZING_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include "../src/errors.h"

// Macros

/**
 * Define a loop-once __AFL_LOOP if not using afl-clang-fast
 */
#ifndef __AFL_HAVE_MANUAL_CONTROL
#define __AFL_LOOP(x) !(count++)
static int count = 0; ///< Definition of the AFL loop counter, used by afl-fuzz - warning is OK
#endif

/**
 * The same as @ref RETURN_IF_FAIL, but aborts instead of returning.
 */
#define ABORT_IF_FAIL(x) do { v2f_error_t return_value = (x); if(return_value != V2F_E_NONE) \
    { printf("Error %s at %s:%d\n", v2f_strerror(return_value), __FILE__, (int) __LINE__); abort(); } } while(0)

// Common fd-related functions.

/**
 * Truncate a file to zero bytes and set the file pointer to the first byte.
 *
 * @param file file to be truncated.
 */
void fuzzing_reset_file(FILE *const file);

/**
 * Verify whether two files open for reading have equal (remaining) size and contents.
 *
 * @param f1 first file to be compared.
 * @param f2 second file to be compared.
 *
 * @return true if and only if file are bitwise identical.
 */
bool fuzzing_check_files_are_equal(FILE *f1, FILE *f2);


/**
 * Copy the remaining data of @a input into @a output and move the file pointer of @a output to the first byte
 * of the file.
 *
 * @param input file to be copied.
 * @param output file where the copy is to be stored.
 */
void copy_file(FILE *input, FILE *output);

/**
 * Generate a temporary file asserting that it is properly open.
 * @param temporary_file pointer to the FILE* pointer that should
 *  point to the newly created file.
 * @return always V2F_E_NONE, otherwise it aborts.
 */
v2f_error_t
v2f_fuzzing_assert_temp_file_created(FILE **const temporary_file);

/**
 * Read the sample count and bytes per index from the input file.
 * - samples: 4 byte unsigned big endian
 * - bytes per sample/index: 1 byte
 *
 * @param file file open for reading
 * @param sample_count pointer to the variable where the number of samples is
 *   to be stored
 * @param bytes_per_sample pointer to the variable where the number of bytes
 *   per sample is to be stored.
 * @return
 *   - @ref V2F_E_NONE : values successfully read
 *   - @ref V2F_E_CORRUPTED_DATA : invalid values read
 *   - @ref V2F_E_IO : input/output error
 */
v2f_error_t fuzzing_get_samples_and_bytes_per_sample(
        FILE *file,
        uint32_t *sample_count,
        uint8_t *bytes_per_sample);

#endif /* FUZZING_COMMON_H */
