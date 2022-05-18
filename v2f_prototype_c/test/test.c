/**
 * @file
 *
 * 
 * Entry point for all unittest suites:
 *
 *   - General functionality tests:
 *
 * All tests have been run and verified not to fail.
 *
 * Usage (from the project root):
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * $ make
 * $ ./build/unittest
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <CUnit/Basic.h>

#include "suite_registration.h"

/**
 * Entropy point for all unittests suites.
 * It executes all test suites and reports the test results, stopping on any failure.
 *
 * @return 0 is always returned by this function, but test may abort the program if an error is detected.
 *
 * @req V2F-3.1
 */
int main(void);


int main(void) {

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return (int) CU_get_error(); // LCOV_EXCL_LINE
    }

    register_timer();
    register_build();
    register_entropy_codec();
    register_file();
    register_quantizer();
    register_decorrelator();
    register_compressor_decompressor();
    register_bin_common();

    //CU_basic_set_mode(CU_BRM_NORMAL);
    CU_basic_set_mode(CU_BRM_VERBOSE);

    CU_basic_run_tests();

    return 0;
}
