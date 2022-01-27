/**
 * @file
 *
 * @brief  Interface definition of functionality shared by several tests suites.
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

#include "../src/errors.h"
#include "../src/v2f.h"
#include "../src/v2f_entropy_coder.h"
#include "../src/v2f_entropy_decoder.h"

/**
 * Compares two vectors. Padding is not compared.
 *
 * @param vector1 first vector to compare.
 * @param vector2 second vector to compare.
 * @param large_f vector size in bits.
 *
 * @return whether the vectors are equal.
 */
bool test_vectors_are_equal(uint8_t const *const vector1, uint8_t const *const vector2, const uint32_t large_f);

/**
 * Given two FILEs open for reading, return true if and only if both have the same (remaining)
 * length_bits and contain the same data.
 *
 * File pointers are advanced to the end of the file.
 *
 * @param file1 first file open for reading.
 * @param file2 second file open for reading.
 *
 * @return true if the files have the same length in bits and contain the same data, false otherwise.
 */
bool test_assert_files_are_equal(FILE *const file1, FILE *const file2);

/**
 * Given an open file for writing, truncate the file to 0 bytes and move the
 * file pointer to the first byte.
 *
 * @param file to be truncated.
 */
void test_reset_file(FILE *const file);

/**
 * Return the size of file in bytes. The file pointer after calling this function is not modified.
 *
 * @param file open file to be queried.
 *
 * @return the size of the file in bytes, or -1 if there is an error.
 */
off_t get_file_size(FILE *const file);

/**
 * Copy the remaining data of @a input into @a output and move the file pointer of @a output to the first byte
 * of the file.
 *
 * @param input file to be copied.
 * @param output file where the copy is to be stored.
 */
void copy_file(FILE * input, FILE * output);

#endif /* TEST_COMMON_H */
