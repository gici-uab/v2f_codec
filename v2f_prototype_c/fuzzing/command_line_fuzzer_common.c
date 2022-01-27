/**
 * @file
 *
 * Implementation of utility functions used by the @ref command_line_fuzzer.c and @ref command_line_fiu_fuzzer.c
 * fuzzers.
 */


#include <unistd.h>
#include <sched.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>

#include "command_line_fuzzer_common.h"

/**
 * Given a (valid) file path to be open and an open mode,
 * open instead a PID-dependent file so that concurrent
 * fuzzer executions do not clash.
 *
 * @param pathname path of the file to be used
 * @param mode mode in which the file is to be open
 *
 * @return the fid of the multiprocess-safe file opened in this call
 */
FILE *fake_fopen(const char *pathname, const char *mode);

/** Redefinition of fopen as @ref fake_fopen for safe fuzzer execution
 */
#define fopen fake_fopen

/** Name of the tool */
#define TOOL_NAME verify_codec_bin
/** Name of the file implementing the tool */
#define TOOL_FILE "../bin/v2f_verify_codec.c"

#include "command_line_fuzzer_include_helper.h"

#undef TOOL_NAME
#undef TOOL_FILE
#undef fopen

/** Name of the tool */
#define TOOL_NAME compress
/** Name of the file implementing the tool */
#define TOOL_FILE "../bin/v2f_compress.c"

#include "command_line_fuzzer_include_helper.h"

#undef TOOL_NAME
#undef TOOL_FILE
#undef fopen

/**
 * Array of command/tool functions available for command-line testing
 */
static
int (*command_list[2])(int argc, char *argv[]) = {
        main_verify_codec_bin, // 0
        main_compress, // 1
        // other_example, // 1
};



/**
 * Total number of available temporary file names
 */
#define TOTAL_TEMP_FILES 256


/**
 * Valid temporary file names
 */
static char const *const temporary_file_names[TOTAL_TEMP_FILES] = {
        "f000", "f001", "f002", "f003", "f004", "f005", "f006", "f007",
        "f008", "f009", "f010", "f011", "f012", "f013", "f014", "f015",
        "f016", "f017", "f018", "f019", "f020", "f021", "f022", "f023",
        "f024", "f025", "f026", "f027", "f028", "f029", "f030", "f031",
        "f032", "f033", "f034", "f035", "f036", "f037", "f038", "f039",
        "f040", "f041", "f042", "f043", "f044", "f045", "f046", "f047",
        "f048", "f049", "f050", "f051", "f052", "f053", "f054", "f055",
        "f056", "f057", "f058", "f059", "f060", "f061", "f062", "f063",
        "f064", "f065", "f066", "f067", "f068", "f069", "f070", "f071",
        "f072", "f073", "f074", "f075", "f076", "f077", "f078", "f079",
        "f080", "f081", "f082", "f083", "f084", "f085", "f086", "f087",
        "f088", "f089", "f090", "f091", "f092", "f093", "f094", "f095",
        "f096", "f097", "f098", "f099", "f100", "f101", "f102", "f103",
        "f104", "f105", "f106", "f107", "f108", "f109", "f110", "f111",
        "f112", "f113", "f114", "f115", "f116", "f117", "f118", "f119",
        "f120", "f121", "f122", "f123", "f124", "f125", "f126", "f127",
        "f128", "f129", "f130", "f131", "f132", "f133", "f134", "f135",
        "f136", "f137", "f138", "f139", "f140", "f141", "f142", "f143",
        "f144", "f145", "f146", "f147", "f148", "f149", "f150", "f151",
        "f152", "f153", "f154", "f155", "f156", "f157", "f158", "f159",
        "f160", "f161", "f162", "f163", "f164", "f165", "f166", "f167",
        "f168", "f169", "f170", "f171", "f172", "f173", "f174", "f175",
        "f176", "f177", "f178", "f179", "f180", "f181", "f182", "f183",
        "f184", "f185", "f186", "f187", "f188", "f189", "f190", "f191",
        "f192", "f193", "f194", "f195", "f196", "f197", "f198", "f199",
        "f200", "f201", "f202", "f203", "f204", "f205", "f206", "f207",
        "f208", "f209", "f210", "f211", "f212", "f213", "f214", "f215",
        "f216", "f217", "f218", "f219", "f220", "f221", "f222", "f223",
        "f224", "f225", "f226", "f227", "f228", "f229", "f230", "f231",
        "f232", "f233", "f234", "f235", "f236", "f237", "f238", "f239",
        "f240", "f241", "f242", "f243", "f244", "f245", "f246", "f247",
        "f248", "f249", "f250", "f251", "f252", "f253", "f254", "f255",
};


FILE *fake_fopen(const char *pathname, const char *mode) {
    bool found = false;

    for (int32_t i = 0; i < TOTAL_TEMP_FILES; i++) {
        if (strcmp(pathname, temporary_file_names[i]) == 0) {
            found = true;
        }
    }

    if (!found) {
        return NULL;
    }

    if (strlen(P_tmpdir) > 250) {
        abort(); // LCOV_EXCL_LINE
    }

    char fixed_pathname[1024];

    static pid_t pid;

    if (pid == 0) {
        pid = getpid();
    }


    sprintf(fixed_pathname, "%s/%s-%d", P_tmpdir, pathname, pid);

    return fopen(fixed_pathname, mode);
}

int fake_open_read(const char *pathname, const pid_t parent_pid) {
    bool found = false;

    for (int32_t i = 0; i < TOTAL_TEMP_FILES; i++) {
        if (strcmp(pathname, temporary_file_names[i]) == 0) {
            found = true;
            break;
        }
    }

    if (!found) {
        return -1; // LCOV_EXCL_LINE
    }

    if (strlen(P_tmpdir) > 250) {
        abort(); // LCOV_EXCL_LINE
    }

    char fixed_pathname[1024];

    sprintf(fixed_pathname, "%s/%s-%d", P_tmpdir, pathname, parent_pid);

    return open(fixed_pathname, O_RDONLY);
}

/**
 * Remove PID-dependent temporary files
 *
 * @param how_many number of temporary files to remove
 *
 * @return (void)
 */
static void get_rid_of_temp_files(int how_many) {
    char fixed_pathname[1024];

    static pid_t pid;

    if (pid == 0) {
        pid = getpid();
    }

    for (int32_t i = 0; i < how_many; i++) {
        sprintf(fixed_pathname, "%s/%s-%d", P_tmpdir, temporary_file_names[i],
                pid);

        (void) remove(fixed_pathname);
    }
}

/**
 *
 * @enum command_line_element_t
 * Enumeration of the types of command-line elements available in an invocation
 */
typedef enum {
    CMD_INPLACE_PARAMETER = 0,
    CMD_FILE = 1,
    CMD_REUSE_FILE = 2,
    CMD_SETSTDIN = 3,
    CMD_CLOSESTDOUT = 5,
    CMD_CLOSESTDERR = 6,
    CMD_DICT_PARAMETER = 7,
    CMD_DICT_OPTION = 8,
    CMD_END_LIST = 9,
} command_line_element_t;

/// Maximum number of commands in an invocation
#define MAX_COMMANDS 16
/// Maximum number of temporary files in an invocation
#define MAX_TEMP_FILES 16


/*
 * Command files are as described below in pseudo-struct syntax.
 *MAX_COMMANDS
 * struct syntax {
 *   uint8_t file_count;
 *
 *   uint8_t commands[MAX_COMMANDS];
 *
 *   struct {
 *     uint16_t block_size; // block_size == 0xFFFF => DO NOT CREATE
 *     uint8_t block_contents[(block_size + 255) / 256 * 256];
 *   } file_contents[file_count];
 * }
 *
 * A command is one of the following:
 *
 * CMD_INPLACE_PARAMETER + null terminated string
 * CMD_DICT_PARAMETER + dictionary_index.
 * CMD_DICT_OPTION + dictionary_index.
 * CMD_FILE + file_index
 */

/**
 * Read @a size bytes from @a stream into @a ptr, or abort.
 *
 * @param ptr buffer where read data is to be stored
 * @param size number of bytes to read
 * @param stream FILE* open for reading from which data are to be read
 */
#define FULL_READ(ptr, size, stream) \
        if (fread((ptr), (size), 1, (stream)) != 1) { \
            if (ferror(stream)) { abort(); } else { return; } \
        }

/**
 * Print information about a command-line invocation
 *
 * @param tool tool (command) to be invoked
 * @param argc argc of the invocation
 * @param argv argv of the invocation
 *
 * @return (void)
 */
static void show_command_line(int tool, int argc, char *argv[]) {
    printf("Running command %d with %d arguments:", tool, argc);

    for (int i = 0; i < argc; i++) {
        putchar(' ');

        char *str = argv[i];

        while (*str != '\0') {
            if (isprint(*str)) {
                putchar(*str);
            } else {
                putchar('.');
            }

            str++;
        }
    }

    printf("\n");
}

void handle_command_file(
        FILE *const file,
        int tool,
        char *const scratch_buffer,
        int (*call_wrapper)(int (*command)(int argc, char *argv[]), int argc,
                            char *argv[],
                            char const *setstdin, bool closestdout,
                            bool closestderr)) {

    // Check tool number validity
    assert(tool >= 0);
    assert(tool <= (int) (sizeof(command_list) / sizeof(command_list[0])));

    uint8_t selected_tool;

    FULL_READ(&selected_tool, sizeof(selected_tool), file);

    if (tool >= 0 && tool != selected_tool) {
        printf("Initial byte of the input did not match the one "
               "specified in the command line (%d), which is to be used.\n",
               (int) tool);
        selected_tool = (uint8_t) tool;
    }

    if ((size_t) selected_tool >=
        sizeof(command_list) / sizeof(command_list[0])) {
        return; // LCOV_EXCL_LINE
    }

    assert(tool >= 0);

    // Start building a command line
    puts("Building command line\n");

    char *command_line[MAX_COMMANDS];
    int32_t parameter_index = 1;
    command_line[0] = "fake_command_name_argv0";

    char const *setstdin = NULL;
    bool closestdout = false;
    bool closestderr = false;

    // Local copy buffer

    int32_t scratch_buffer_used = 0;
    int32_t files_used = 0;

    char zero_len_string[1];
    zero_len_string[0] = '\0';

    while (scratch_buffer_used < SCRATCH_BUFFER_SIZE
           && parameter_index < MAX_COMMANDS
           && files_used < MAX_TEMP_FILES) {
        uint8_t command;

        FULL_READ(&command, sizeof(command), file);

        switch (command) {

            case CMD_INPLACE_PARAMETER:

                command_line[parameter_index++] =
                        scratch_buffer + scratch_buffer_used;

                char next_byte;

                do {
                    FULL_READ(&next_byte, sizeof(next_byte), file);


                    if (scratch_buffer_used == SCRATCH_BUFFER_SIZE) {
                        command_line[parameter_index - 1] = zero_len_string;
                        break;
                    }

                    scratch_buffer[scratch_buffer_used++] = next_byte;
                } while (next_byte != 0);

                //show_command_line(tool, parameter_index, command_line);

                break;
            case CMD_FILE:

                command_line[parameter_index++] =
                        scratch_buffer + scratch_buffer_used;

                if (scratch_buffer_used + 5 > SCRATCH_BUFFER_SIZE) {
                    break;
                }

                (void) memcpy(scratch_buffer + scratch_buffer_used,
                              temporary_file_names[files_used], 5);

                scratch_buffer_used += 5;
                files_used++;

                show_command_line(tool, parameter_index, command_line);

                break;
            case CMD_SETSTDIN:
                setstdin = temporary_file_names[files_used];
                files_used++;
                break;
            case CMD_CLOSESTDOUT:
                closestdout = true;
                break;
            case CMD_CLOSESTDERR:
                closestderr = true;
                break;
            case CMD_END_LIST:
            default:
                scratch_buffer_used = SCRATCH_BUFFER_SIZE;
                break;
        }

        show_command_line(tool, parameter_index, command_line);
    }

    show_command_line(tool, parameter_index, command_line);

    log_debug("Populating files\n");

    // Populate files.
    for (int32_t file_index = 0; file_index < files_used; file_index++) {

        uint8_t command = 0;

        (void) fread(&command, sizeof(command), 1, file);

        log_debug("populate command %d\n", (int) command);

        assert(!ferror(file));

        if (command != 0) {
            FILE *const f = fake_fopen(temporary_file_names[file_index], "w");

            if (f == NULL) {
                abort(); // LCOV_EXCL_LINE
            }

            int64_t file_size = 0;

            do {
                uint8_t next_byte = 0;

                (void) fread(&next_byte, sizeof(next_byte), 1, file);

                if (next_byte == 0) {
                    (void) fread(&next_byte, sizeof(next_byte), 1, file);

                    if (feof(file) || next_byte != 0) {
                        break;
                    }
                }

                if (fwrite(&next_byte, sizeof(next_byte), 1, f) != 1) {
                    abort(); // LCOV_EXCL_LINE
                }

                file_size++;
            } while (file_size < 1000000000);

            if (fclose(f)) {
                abort(); // LCOV_EXCL_LINE
            }

            log_debug("File %s filled with %ld bytes\n",
                   temporary_file_names[file_index], file_size);
        }
    }

    // Debug info
    show_command_line(tool, parameter_index, command_line);

    if (setstdin != NULL) {
        log_debug("Stdin set to %s\n", setstdin);
    }

    // Run command
    const int return_code = call_wrapper(command_list[tool], parameter_index,
                                         command_line, setstdin,
                                         closestdout, closestderr);

    if (return_code == 0) {
        log_debug("Jackpot!\n"); // this is just here to let the fuzzer know that we care for a different return_code.
    }

    get_rid_of_temp_files(files_used);
}
