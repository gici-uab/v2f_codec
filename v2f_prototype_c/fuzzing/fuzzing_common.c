/**
 * @file
 *
 * Implementation of utility functions common to several fuzzers.
 *
 * I/O errors in this module are tagged with LCOV_EXCL_LINE, since they are not meant to be triggered.
 *
 */

// silence warning in fuzzing.h regarding static int count = 0;
#ifndef __AFL_HAVE_MANUAL_CONTROL
#define __AFL_HAVE_MANUAL_CONTROL
#endif

#include <stdbool.h>
#include <unistd.h>
#include <assert.h>

#include "fuzzing_common.h"
#include "../src/v2f.h"
#include "../src/common.h"
#include "../src/errors.h"
#include "fuzzing_common.h"

void fuzzing_reset_file(FILE *const file) {
    clearerr(file);

    if (fseeko(file, 0, SEEK_SET)) {
        abort(); // LCOV_EXCL_LINE
    }

    // As per POSIX § 2.5.1 Interaction of File Descriptors and Standard I/O Streams
    if (fflush(file)) {
        abort(); // LCOV_EXCL_LINE
    }

    if (ftruncate(fileno(file), 0) != 0) {
        abort(); // LCOV_EXCL_LINE
    }
}

bool fuzzing_check_files_are_equal(FILE *f1, FILE *f2) {
    size_t pos = 0;

    const size_t buffer_size = 1024;

    unsigned char buffer1[buffer_size];
    unsigned char buffer2[buffer_size];

    while (true) {
        size_t r1 = fread(&buffer1, 1, buffer_size, f1);
        size_t r2 = fread(&buffer2, 1, buffer_size, f2);

        if (ferror(f1) || ferror(f2)) {
            return false; // LCOV_EXCL_LINE
        }

        for (size_t i = 0; i < (r1 < r2 ? r1 : r2); i++) {
            if (buffer1[i] != buffer2[i]) {
                // LCOV_EXCL_START
                printf("Difference at byte %d (%d vs %d)\n", (int) pos,
                       buffer1[i], buffer2[i]);
                return false;
                // LCOV_EXCL_STOP
            }
            pos++;
        }

        if (r1 != r2) {
            if (r1 < r2) { // LCOV_EXCL_LINE
                printf("Premature EOF in fd1 at pos %d\n",
                       (int) (pos + (size_t) r1)); // LCOV_EXCL_LINE
            } else {
                printf("Premature EOF in fd2 at pos %d\n",
                       (int) (pos + (size_t) r2)); // LCOV_EXCL_LINE
            }
            return false; // LCOV_EXCL_LINE
        }

        if (r1 == 0) {
            return true;
        }
    }

    return false;
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

v2f_error_t
v2f_fuzzing_assert_temp_file_created(FILE **const temporary_file) {

#ifndef O_TMPFILE
    *temporary_file = tmpfile();
#else
    /*
     * When fuzzing, there are too many files in TMPDIR, thus tmpfile takes a long time to find a unique filename. This
     * is not a problem, except that the fuzzer kills tasks that take too much time right between the creation of the
     * temporary file and its unlinking (both operations are performed within tmpfile). As more tasks are killed, more
     * files are in TMPDIR, further exacerbating the problem.
     *
     * We use the following solution to create temporary files atomically.
     */

    *temporary_file = NULL;

    const int fd = open(P_tmpdir, O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);

    if (fd < 0) {
        return V2F_E_UNABLE_TO_CREATE_TEMPORARY_FILE;
    }

    *temporary_file = fdopen(fd, "w+");
#endif

    assert(*temporary_file != NULL);

    return V2F_E_NONE;
}


v2f_error_t fuzzing_get_samples_and_bytes_per_sample(
        FILE *file,
        uint32_t *sample_count,
        uint8_t *bytes_per_sample) {
    const uint32_t max_sample_count = 2048;

    *sample_count = 0;
    for (uint8_t i = 0; i < 4; i++) {
        *sample_count <<= 8;
        uint8_t byte;
        if (fread(&byte, 1, 1, file) != 1) {
            return V2F_E_IO;
        }
        *sample_count += byte;
    }

    if (*sample_count == 0 || *sample_count > max_sample_count) {
        return V2F_E_CORRUPTED_DATA;
    }
    if (fread(bytes_per_sample, 1, 1, file) != 1) {
        return V2F_E_IO;
    }

    return V2F_E_NONE;
}
