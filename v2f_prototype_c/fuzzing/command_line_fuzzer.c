/**
 * @file
 *
 * A fuzzing harness that tests command-line programs.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fenv.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#include "fuzzing_common.h"

#include "command_line_fuzzer_common.h"

// LCOV_EXCL_START "Coveraged" main is in command_line_fiu_fuzzer.

/**
 * This is just a pass-through function so that command_line_fiu_fuzzer can overwrite it.
 *
 * @param command command to be tested
 * @param argc main's argc for the command
 * @param argv main's argv for the command
 * @param setstdin  file to use as @a stdin
 * @param closestdout  should @a stdout be closed?
 * @param closestderr  should @a stderr be closed?
 *
 * @return the return value of the call
 */
int
call_command(int (*command)(int argc, char *argv[]), int argc, char *argv[],
             char const *setstdin, bool closestdout, bool closestderr);

int
call_command(int (*command)(int argc, char *argv[]), int argc, char *argv[],
             char const *setstdin, bool closestdout, bool closestderr) {

    int old_stdin_fd = dup(STDIN_FILENO);
    int old_stdout_fd = dup(STDOUT_FILENO);
    int old_stderr_fd = dup(STDERR_FILENO);

    (void) fflush(stdin);
    (void) clearerr(stdin);

    if (setstdin != NULL) {
        if (strlen(P_tmpdir) > 250) {
            abort(); // LCOV_EXCL_LINE
        }

        int new_stdin_fd = fake_open_read(setstdin, getpid());

        if (new_stdin_fd == -1) {
            close(STDIN_FILENO);
        } else {
            dup2(new_stdin_fd, STDIN_FILENO);
        }
    } else {
        // We control stdin through this file (we don't want any command reading the fuzzed file directly).
        close(STDIN_FILENO);
    }

    if (closestdout) {
        close(STDOUT_FILENO);
    }

    if (closestderr) {
        close(STDERR_FILENO);
    }

    int r = command(argc, argv);

    dup2(old_stdin_fd, STDIN_FILENO);
    dup2(old_stdout_fd, STDOUT_FILENO);
    dup2(old_stderr_fd, STDERR_FILENO);

    close(old_stdin_fd);
    close(old_stdout_fd);
    close(old_stderr_fd);

    return r;
}

/**
 * A fuzzing harness that tests command-line programs.
 *
 * @param argc number of command-line arguments
 * @param argv list of command-line arguments
 *
 * @return the return value of the handled call
 */
int main(int argc, char *argv[]) {

    // NOTE: this fuzzing harness requires an extra argument to specify the command line tool to fuzz.
    // Not providing this value will cause the fuzzing to fail, given that the deferred forkserver will not start
    // before the program terminates.

    // There is no point in sample crossover for fuzzing samples for different tools.
    if (argc != 2) {
        fprintf(stderr,
                "This fuzzer requires a command-line tool as argument (non-negative integer, or -1).\n");
        return 1;
    }

    int command = atoi(argv[1]);

    char *scratch_buffer = malloc(SCRATCH_BUFFER_SIZE);
    assert(scratch_buffer != NULL);


    // There may be open files and allocated memory, so persistent mode is not feasible.
    // Instead we employ the forkserver mode.
#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
#else
    (void) (count);
#endif

    // Handle command.
    handle_command_file(stdin, command, scratch_buffer, call_command);

    free(scratch_buffer);

    return 0;
}

// LCOV_EXCL_STOP
