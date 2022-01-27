/**
 * @file
 *
 * Interface implementation of functionality shared by several tests suites.
 *
 * LCOV_EXCL_LINE is used for I/O error points in this module that are not meant to be triggered.
 *
 * @see test_common.h for further details.
 *
 */

#include "../src/common.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "test_common.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "CUExtension.h"

// Test functions need not be part covered by the report
// LCOV_EXCL_START

bool test_vectors_are_equal(uint8_t const *const vector1,
                            uint8_t const *const vector2,
                            const uint32_t large_f) {
    if (memcmp(vector1, vector2, large_f / 8) != 0) {
        return false;
    }

    if (large_f % 8) {
        return ((vector1[large_f / 8] ^ vector2[large_f / 8]) &
                (UINT8_C(0xff) << (8 - (large_f % 8)))) == 0;
    } else {
        return true;
    }
}

bool test_assert_files_are_equal(FILE *const file1, FILE *const file2) {
    
    if (fseeko(file1, 0, SEEK_SET) != 0) {
        // IO error
        abort(); // LCOV_EXCL_LINE
    }

    if (fseeko(file2, 0, SEEK_SET) != 0) {
        // IO error.
        abort(); // LCOV_EXCL_LINE
    }

    size_t pos = 0;

    const size_t buffer_size = 1024;

    uint8_t buffer1[buffer_size];
    uint8_t buffer2[buffer_size];

    while (true) {
        size_t r1 = fread(&buffer1, 1, buffer_size, file1);
        size_t r2 = fread(&buffer2, 1, buffer_size, file2);

        if (ferror(file1) || ferror(file2)) {
            abort(); // LCOV_EXCL_LINE
        }

        if (r1 != r2) {
            abort(); // LCOV_EXCL_LINE
        }

        if (feof(file1)) {
            // This is the line that returns true upon identical files.
            return feof(file2);
        }

        for (size_t i = 0; i < (size_t) r1; i++) {
            if (buffer1[i] != buffer2[i]) {
                abort(); // LCOV_EXCL_LINE
            }
            pos++;
        }
    }

    // The loop is only left with return or abort, it is never broken
    return true; // LCOV_EXCL_LINE
}


void test_reset_file(FILE *const file) {
    clearerr(file);

    CU_ASSERT_EQUAL_FATAL(fseeko(file, 0, SEEK_SET), 0);

    // As per POSIX § 2.5.1 Interaction of File Descriptors and Standard I/O Streams
    CU_ASSERT_EQUAL_FATAL(fflush(file), 0);

    CU_ASSERT_EQUAL_FATAL(ftruncate(fileno(file), 0), 0);
}

off_t get_file_size(FILE *const file) {
    const off_t offset_before = ftello(file);

    if (offset_before < 0) {
        abort(); // LCOV_EXCL_LINE
    }

    if (fseeko(file, 0, SEEK_END) < 0) {
        abort(); // LCOV_EXCL_LINE
    }

    off_t file_size = ftello(file);

    if (file_size < 0) {
        abort(); // LCOV_EXCL_LINE
    }

    if (fseeko(file, offset_before, SEEK_SET) < 0) {
        abort(); // LCOV_EXCL_LINE
    }

    return file_size;
}

void copy_file(FILE *input, FILE *output) {

    while (true) {
        char buffer[1024];

        size_t r1 = fread(&buffer, 1, 1024, input);

        if (ferror(input)) {
            abort(); // LCOV_EXCL_LINE
        }

        if (r1 == 0) {
            assert(feof(input));
            break;
        }

        (void) fwrite(&buffer, (size_t) r1, 1, output);

        if (ferror(output)) {
            abort(); // LCOV_EXCL_LINE
        }
    }

    if (fseeko(output, 0, SEEK_SET) != 0) {
        abort(); // LCOV_EXCL_LINE
    }
}

// LCOV_EXCL_STOP