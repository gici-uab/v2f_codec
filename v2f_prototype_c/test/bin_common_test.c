/**
 * @file
 *
 * Suite of tests for the @ref bin_common.h module.
 *
 */

#include <stdio.h>
#include <string.h>

#include "../bin/bin_common.h"
#include "CUExtension.h"
#include "suite_registration.h"


#include "test_common.h"
#include "test_samples.h"

/**
 * Test the integer list string tokenizer function.
 */
void test_string_tokenizer(void);

void test_string_tokenizer(void) {
    uint32_t* parsed_integers = NULL;
    uint32_t integer_count = 0;

    CU_ASSERT_FATAL(parse_positive_integer_list("", &parsed_integers, &integer_count) != 0);
    CU_ASSERT_FATAL(parse_positive_integer_list(",5", &parsed_integers, &integer_count) != 0);
    CU_ASSERT_FATAL(parse_positive_integer_list("5,", &parsed_integers, &integer_count) != 0);
    CU_ASSERT_FATAL(parse_positive_integer_list("5,,6", &parsed_integers, &integer_count) != 0);
    CU_ASSERT_FATAL(parse_positive_integer_list("a", &parsed_integers, &integer_count) != 0);
    CU_ASSERT_FATAL(parse_positive_integer_list("5,6,a,7", &parsed_integers, &integer_count) != 0);

    CU_ASSERT_FATAL(parse_positive_integer_list("5,6,7", &parsed_integers, &integer_count) == 0);
    CU_ASSERT_FATAL(parsed_integers != NULL);
    CU_ASSERT_FATAL(integer_count == 3);
    CU_ASSERT_FATAL(parsed_integers[0] == 5);
    CU_ASSERT_FATAL(parsed_integers[1] == 6);
    CU_ASSERT_FATAL(parsed_integers[2] == 7);
    free(parsed_integers);

    CU_ASSERT_FATAL(parse_positive_integer_list("10,100,1000,10000,100000,1000000",
                                                &parsed_integers, &integer_count) == 0);
    CU_ASSERT_FATAL(parsed_integers != NULL);
    CU_ASSERT_FATAL(integer_count == 6);
    CU_ASSERT_FATAL(parsed_integers[0] == 10);
    CU_ASSERT_FATAL(parsed_integers[1] == 100);
    CU_ASSERT_FATAL(parsed_integers[2] == 1000);
    CU_ASSERT_FATAL(parsed_integers[3] == 10000);
    CU_ASSERT_FATAL(parsed_integers[4] == 100000);
    CU_ASSERT_FATAL(parsed_integers[5] == 1000000);
    free(parsed_integers);
}

CU_START_REGISTRATION(bin_common)
    CU_QADD_TEST(test_string_tokenizer)
CU_END_REGISTRATION()