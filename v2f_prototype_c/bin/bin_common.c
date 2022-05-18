/**
 * @file
 *
 * Implementation of common functions for the command-line interfaces.
 *
 */

#include "bin_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>

int parse_integer(char const *const str, int32_t *const output,
                  char const *const key) {
    errno = 0;

    char *endptr;

    const intmax_t value = strtoimax(str, &endptr, 10);

    if (endptr == str) {
        fprintf(stderr, "Invalid value format in option %s (%s)\n", key, str);
        return 1;
    }

    if (errno != 0) {
        fprintf(stderr, "Invalid value format in option %s: %s (%s)\n", key,
                strerror(errno), str);
        return 1;
    }

    if (value < INT32_MIN || value > INT32_MAX) {
        fprintf(stderr, "Out-of-range value in option %s (%s)\n", key, str);
        return 1;
    }

    *output = (int32_t) value;

    return 0;
}

int parse_positive_integer(char const *const str, uint32_t *const output,
                           char const *const key) {
    int32_t signed_integer;
    const int parse_status = parse_integer(str, &signed_integer, key);
    if (parse_status != 0) {
        return parse_status;
    }

    if (signed_integer < 0) {
        fprintf(stderr,
                "Input value was negative, but only positive values are allowed for %s\n",
                key);
        return 1;
    }

    *output = (uint32_t) signed_integer;

    return 0;
}

int parse_positive_integer_list(char const *const str, uint32_t **output, uint32_t *output_length) {
    if (output == NULL || output_length == NULL) {
        return 1;
    }

    size_t original_size = strlen(str);
    if (original_size == 0) {
        return 1;
    }

    // Validate input string
    bool after_comma = true;
    uint32_t comma_count = 0;
    for (size_t i = 0; i < original_size; i++) {
        // Only commas and numbers are allowed
        if (str[i] != ',' && (str[i] < '0' || str[i] > '9')) {
            return 1;
        }

        // Consecutive commas are not allowed. They are not allowed at the beginning or the
        // end of the string.
        if (i == original_size - 1) {
            after_comma = true;
        }
        if (str[i] == ',') {
            if (after_comma) {
                return 1;
            }
            after_comma = true;
            comma_count++;
        } else {
            after_comma = false;
        }
    }

    *output = malloc(sizeof(uint32_t) * (comma_count + 1));
    if (*output == NULL) {
        return 1;
    } else {
        *output_length = comma_count + 1;
    }

    char *restrict str2 = malloc(sizeof(char) * (original_size + 1));
    strcpy(str2, str);
    uint32_t processed_tokens = 0;
    char *token;
    do {
        token = strtok(processed_tokens == 0 ? str2 : NULL, ",");
        if (token != NULL) {
            assert(processed_tokens < *output_length);
            (*output)[processed_tokens] = (uint32_t) atoi(token);
            processed_tokens++;
        }
    } while (token != NULL);

    free(str2);

    return 0;
}

void show_banner(void) {
    printf("------------------------------------------------------------------\n"
           "V2F Codec Software version %s\n\n"
           "Software development:\n"
           "    Miguel Hernández-Cabronero <miguel.hernandez@uab.cat>, et al.\n"
           "Project management:\n"
           "    Miguel Hernández-Cabronero <miguel.hernandez@uab.cat>\n"
           "    Joan Serra-Sagristà <joan.serra@uab.cat>\n\n"
           "Technical supervision:\n"
           "    Javier Marin <javier.marin@satellogic.com>\n"
           "    David Vilaseca <vila@satellogic.com>\n\n"
           "Produced by Universitat Autònoma de Barcelona (UAB) for Satellogic.\n"
           "------------------------------------------------------------------\n",
           PROJECT_VERSION);
}

