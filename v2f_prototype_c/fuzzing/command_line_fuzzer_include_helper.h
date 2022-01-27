/**
 * @file
 *
 * @brief This file enables \#include within \#define for @ref command_line_fuzzer.c
 */

/**
 * A helper macro for token concatenation.
 *
 * @param X first token
 * @param Y second token
 */
#define CONCATENATE2(X, Y) X##Y

/**
 * A helper macro for token concatenation.
 *
 * @param X first token
 * @param Y second token
 */
#define CONCATENATE(X, Y) CONCATENATE2(X,Y)

// Renames for common elements in .c files (main and show_usage_string).

/// Macro to get the correct show_usage_string for the given tool
#define show_usage_string CONCATENATE(show_usage_string_, TOOL_NAME)

/// Macro to get the correct main function for the given tool
#define main CONCATENATE(main_, TOOL_NAME)

// Declaration for renamed main function.
//! @cond Doxygen_Suppress
int CONCATENATE(main_, TOOL_NAME)(int argc, char *argv[]);
//! @endcond

// Inclusion of the main file.

#include TOOL_FILE

// Clean-up

#undef CONCATENATE
#undef CONCATENATE2
#undef show_usage_string
#undef main
