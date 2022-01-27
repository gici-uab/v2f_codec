/**
 * @file
 *
 * Not an actual fuzzer.
 *
 * This file takes all fuzzing samples from a directory and processes them. Useful to take measure
 * code coverage for a huge number of samples WHILE INJECTING I/O FAILURES with libfiu.
 */

#ifndef _GNU_SOURCE
/// Symbol that allows access to low-level symbols
#define _GNU_SOURCE
#endif

#include <fenv.h>

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <assert.h>
#include <execinfo.h>

/// Symbol to enable the injection of I/O failure points
#define FIU_ENABLE

#include <fiu.h>
#include <fiu-control.h>

#include "command_line_fuzzer_common.h"

/**
 * Take all fuzzing samples from a directory and processes them. Useful to take measure
 * code coverage for a huge number of samples WHILE INJECTING I/O FAILURES with libfiu.
 *
 * @param argc command-line argument count
 * @param argv command-line argument list
 *
 * @return 0 is always returned by this method (the program can abort, though)
 */
int main(int argc, char *argv[]);

/**
 * Callback for libfiu fiu_enable.
 *
 * @param name parameters of the fiu callback function (see libfiu documentation).
 * @param failnum parameters of the fiu callback function (see libfiu documentation).
 * @param failinfo parameters of the fiu callback function (see libfiu documentation).
 * @param flags parameters of the fiu callback function (see libfiu documentation).
 *
 * @return
 *   - 1 : the fault was triggered
 *   - 0 : the fault was not triggered
 */
int fiu_callback(const char *name, int *failnum, void **failinfo,
                 unsigned int *flags);

/**
 * Function that wraps the actual call to the tool.
 *
 * @param command command to be called
 * @param argc argc for the command
 * @param argv argv for the command
 * @param setstdin path to the file to be used as @a stdin
 * @param closestdout should stdout be closed?
 * @param closestderr should stderr be closed?
 *
 * @return the return value of the called command
 */
int
call_command(int (*command)(int argc, char *argv[]), int argc, char *argv[],
             char const *setstdin, bool closestdout, bool closestderr);

int fiu_callback(const char *name, int *failnum, void **failinfo,
                 unsigned int *flags) {
    (void) name;
    (void) failinfo;

    static bool called = false;
    static int count = -1;

    if (*flags == 2) {
        count = *failnum;
        called = false;
        return 0;
    } else if (*flags == 3) {
        return called ? 1 : 0;
    }

    // Figure out if we still are in this fuzzing harness or we are in libc.
    const int max_trace_size = 16;

    void *buffer[max_trace_size];

    const int trace_size = backtrace(buffer, max_trace_size);

    if (trace_size < 4) {
        // something went very wrong
        abort(); // LCOV_EXCL_LINE
    }

    char **const strings = backtrace_symbols(buffer, trace_size);

    if (strings == NULL) {
        abort(); // LCOV_EXCL_LINE
    }

    char const *const str1 = "command_line_fiu_fuzzer_coverage";
    char const *const str2 = "libfiu.so";
    char const *const str3 = "fiu_posix_preload.so";

    const bool still_in_fuzzing_harness = strstr(strings[0], str1) != NULL
                                          && strstr(strings[1], str2) != NULL
                                          && strstr(strings[2], str3) != NULL
                                          && strstr(strings[3], str1) != NULL;

    if (!still_in_fuzzing_harness) {
        return 0; // LCOV_EXCL_LINE
    }

    // We are where we need to be, so decrease counter and act accordingly.

    count--;

    if (count == -1) {
        backtrace_symbols_fd(buffer, trace_size > 5 ? 5 : trace_size,
                             STDERR_FILENO);
        printf("fault injected now!\n");

        called = true;
        return 1;
    } else {
        return 0;
    }
}

int
call_command(int (*command)(int argc, char *argv[]), int argc, char *argv[],
             char const *setstdin, bool closestdout, bool closestderr) {

    int wstatus_ok;

    printf("First run\n");

    (void) fflush(stdin);
    (void) clearerr(stdin);

    const pid_t parent_pid = getpid();

    // Call once without faults... (and make sure everything works)
    {
        pid_t child = fork();

        if (child == 0) {
            // Call command
            int old_stdin_fd = dup(STDIN_FILENO);
            int old_stdout_fd = dup(STDOUT_FILENO);
            int old_stderr_fd = dup(STDERR_FILENO);

            if (setstdin != NULL) {
                int new_stdin_fd = fake_open_read(setstdin, parent_pid);

                if (new_stdin_fd == -1) {
                    close(STDIN_FILENO);
                } else {
                    dup2(new_stdin_fd, STDIN_FILENO);
                }
            } else {
                // Make sure stdin is closed if we are not putting anything ourselves.
                close(STDIN_FILENO);
            }

            if (closestdout) {
                close(STDOUT_FILENO);
            } else {
                // Suppress any output.
                dup2(open("/dev/null", O_WRONLY), STDOUT_FILENO);
            }

            if (closestderr) {
                close(STDERR_FILENO);
            } else {
                // Suppress any output.
                dup2(open("/dev/null", O_WRONLY), STDERR_FILENO);
            }

            int r = command(argc, argv);

            dup2(old_stdin_fd, STDIN_FILENO);
            dup2(old_stdout_fd, STDOUT_FILENO);
            dup2(old_stderr_fd, STDERR_FILENO);

            close(old_stdin_fd);
            close(old_stdout_fd);
            close(old_stderr_fd);

            exit(r);
        }

        waitpid(child, &wstatus_ok, 0);

        if (!WIFEXITED(wstatus_ok)) {
            abort(); // LCOV_EXCL_LINE
        }
    }

    // Call again, but this time crash I/O operations
    for (int faultno = 0; faultno < 128; faultno++) {
        printf("Fault %d\n", faultno);

        pid_t child = fork();

        if (child == 0) {
            // Fix std* streams.
            int old_stdin_fd = dup(STDIN_FILENO);
            int old_stdout_fd = dup(STDOUT_FILENO);
            int old_stderr_fd = dup(STDERR_FILENO);

            if (setstdin != NULL) {
                int new_stdin_fd = fake_open_read(setstdin, parent_pid);

                if (new_stdin_fd == -1) {
                    close(STDIN_FILENO);
                } else {
                    dup2(new_stdin_fd, STDIN_FILENO);
                }
            } else {
                // Make sure stdin is closed if we are not putting anything ourselves.
                close(STDIN_FILENO);
            }

            if (closestdout) {
                close(STDOUT_FILENO);
            } else {
                // Suppress any output.
                dup2(open("/dev/null", O_WRONLY), STDOUT_FILENO);
            }

            if (closestderr) {
                close(STDERR_FILENO);
            } else {
                // Suppress any output.
                dup2(open("/dev/null", O_WRONLY), STDERR_FILENO);
            }

            // Here goes the I/O fault injector
            char const *const name[3] = {"posix/stdio/*", "libc/mm/*",
                                         "posix/io/oc/open"};
            const size_t name_count = sizeof(name) / sizeof(char *);

            for (size_t i = 0; i < name_count; i++) {
                if (fiu_enable_external(name[i], 1, NULL, 0, fiu_callback)) {
                    abort(); // LCOV_EXCL_LINE
                }
            }

            // Set counter
            unsigned int two = 2;
            fiu_callback("", &faultno, NULL, &two);

            // Call command

            int r1 = command(argc, argv);

            // Disable fault injector
            for (size_t i = 0; i < name_count; i++) {
                if (fiu_disable(name[i])) {
                    abort(); // LCOV_EXCL_LINE
                }
            }

            fflush(stdin);
            fflush(stdout);
            fflush(stderr);

            // Reset std* streams.
            dup2(old_stdin_fd, STDIN_FILENO);
            dup2(old_stdout_fd, STDOUT_FILENO);
            dup2(old_stderr_fd, STDERR_FILENO);

            close(old_stdin_fd);
            close(old_stdout_fd);
            close(old_stderr_fd);

            // Exit with whether the fault was triggered (1) or not (0).
            unsigned int three = 3;
            int r2 = fiu_callback("", NULL, NULL, &three);

            if (r2 == 1 && r1 == 0) {
                // There was a fault, but command says everything is ok!!
                exit(2); // LCOV_EXCL_LINE
            } else if (r2 == 1) {
                exit(1); // There was a fault and program complained.
            } else {
                exit(0); // No fault.
            }
        }

        int wstatus;

        waitpid(child, &wstatus, 0);

        if (!WIFEXITED(wstatus)) {
            assert(WIFSIGNALED(wstatus)); // LCOV_EXCL_LINE
            printf("Child killed by %d\n",
                   WTERMSIG(wstatus)); // LCOV_EXCL_LINE
            abort(); // LCOV_EXCL_LINE
        }

        // Program says OK after fault!
        if (WEXITSTATUS(wstatus) == 2) {
            printf("Child says OK after fault!\n"); // LCOV_EXCL_LINE
            abort(); // LCOV_EXCL_LINE
        }

        // If no fault is no longer triggered, then we tested all I/O operations for faults.
        if (WEXITSTATUS(wstatus) == 0)
            break;
    }

    return WEXITSTATUS(wstatus_ok);
}


int main(int argc, char *argv[]) {

    if (fiu_init(0)) {
        abort(); // LCOV_EXCL_LINE
    }

    // There is no point in sample crossover for fuzzing samples for different tools.
    if (argc != 3) {
        //LCOV_EXCL_START
        fprintf(stderr,
                "This fuzzer requires a command-line tool as argument (a non-negative integer, or -1) "
                "and a directory full of samples.\n");
        return 1;
        //LCOV_EXCL_STOP
    }

    const int command = atoi(argv[1]);

    const int dir_fd = open(argv[2], O_RDONLY);

    if (dir_fd < 0) {
        perror("Cannot open input directory"); // LCOV_EXCL_LINE
        abort(); // LCOV_EXCL_LINE
    }

    DIR *const dir = fdopendir(dir_fd);

    if (dir == NULL) {
        abort(); // LCOV_EXCL_LINE
    }

    struct dirent *file;

    while ((file = readdir(dir)) != NULL) {
        if (strcmp(file->d_name, ".") == 0 ||
            strcmp(file->d_name, "..") == 0) {
            continue;
        }

        const int file_fd = openat(dir_fd, file->d_name, O_RDONLY);

        //const int file_fd = openat(dir_fd, "sample-005053.flex", O_RDONLY);

        if (file_fd < 0) {
            abort(); // LCOV_EXCL_LINE
        }

        struct stat stat;
        if (fstat(file_fd, &stat) < 0) {
            abort(); // LCOV_EXCL_LINE
        }

        if (!S_ISREG(stat.st_mode)) {
            close(file_fd); // LCOV_EXCL_LINE
            continue; // LCOV_EXCL_LINE
        }

        FILE *input_file = fdopen(file_fd, "r");

        if (file == NULL) {
            abort(); // LCOV_EXCL_LINE
        }

        char *scratch_buffer = malloc(SCRATCH_BUFFER_SIZE);

        printf("%s/%s\n", argv[2], file->d_name);

        // Handle command.
        handle_command_file(input_file, command, scratch_buffer, call_command);

        free(scratch_buffer);

        fclose(input_file);
    }

    closedir(dir);

    close(dir_fd);

    return 0;
}

