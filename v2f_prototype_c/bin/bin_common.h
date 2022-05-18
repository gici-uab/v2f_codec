/**
 * @file
 *
 * @brief Implementation of common functions for the CLI interfaces.
 *
 */

#ifndef COMMON_H
#define COMMON_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "../src/v2f.h"

/**
 * Parse one signed integer, verifying it can be stored in an int32_t variable.
 *
 * @param str string containing the integer.
 * @param output variable where the value is to be stored.
 * @param key name of the parameter.
 *
 * @return 0 if and only if the parameter was successfully read.
 *   Otherwise a message is shown and execution may terminate directly.
 */
int parse_integer(char const *const str, int32_t *const output,
                  char const *const key);

/**
 * Parse one signed integer, verifying it is not negative.
 *
 * @param str string containing the integer.
 * @param output variable where the value is to be stored.
 * @param key name of the parameter.
 *
 * @return 0 if and only if the parameter was successfully read.
 *   Otherwise a message is shown and execution may terminate directly.
 */
int parse_positive_integer(char const *const str, uint32_t *const output,
                           char const *const key);

/**
 * Parse a list of positive integers separated by commas
 * and store it in a newly allocated array.
 *
 * @param str string to be parsed.
 * @param output address of a pointer that will be assigned to a newly
 *   allocated array of uint32_t, with the parsed integers. The user
 *   is responsible of freeing the allocated array.
 * @param output_length address of a variable where the number of elements in output is to be
 *   stored.
 *
 * @return 0 if and only if the string could be correctly parsed and did not contain any
 *   weird syntax (e.g., anything but numbers and commas, two or more consecutive commas,
 *   or a comma at the beginning or the end of the string). If any other value is returned,
 *   output and output_length are not modified.
 */
int parse_positive_integer_list(char const *const str, uint32_t ** output, uint32_t* output_length);

void show_banner(void);

#endif
