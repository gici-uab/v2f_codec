/**
 * @file
 *
 * @brief Declaration of utility functions used by @ref command_line_fuzzer.c and @ref command_line_fiu_fuzzer.c.
 */

#ifndef COMMAND_LINE_FUZZER_COMMON_H
#define COMMAND_LINE_FUZZER_COMMON_H

#include <stdio.h>
#include <stdbool.h>

/// Size of the buffer
#define SCRATCH_BUFFER_SIZE 1024

/**
 * Execute and verify a binary/tool on a file.
 *
 * @param file file onto which the command is to be tested
 * @param tool command to be evaluated
 * @param scratch_buffer buffer to be used to parse the call
 * @param call_wrapper function that wraps the actual call to the tool
 */
void handle_command_file(
        FILE *const file,
        int tool,
        char *const scratch_buffer,
        int (*call_wrapper)(int (*command)(int argc, char *argv[]), int argc,
                            char *argv[],
                            char const *setstdin, bool closestdout,
                            bool closestderr));

/**
 * Given a (valid) file path to be open, open instead a PID-dependent file so that concurrent
 * fuzzer executions do not clash.
 *
 * @param pathname path of the file to be used
 * @param parent_pid PID of the parent process
 *
 * @return the fid of the multiprocess-safe file opened in this call
 */
int fake_open_read(const char *pathname, const pid_t parent_pid);

#endif
