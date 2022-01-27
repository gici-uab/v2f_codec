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

void show_banner(void);

#endif
