/**
 * @file
 *
 * @brief Tools to show messages of different priorities.
 *
 * They can be configured to disable these messages.
 */

#ifndef DEBUG_H
#define DEBUG_H

// Error levels are selected in the Makefile. When a level is selected,
// only messages of level equal to or smaller are shown.

/// Error log level
#define LOG_ERROR_LEVEL 1
/// Warning log level
#define LOG_WARNING_LEVEL 2
/// Info log level
#define LOG_INFO_LEVEL 3
/// Debug log level
#define LOG_DEBUG_LEVEL 4

/// Default warning level
#define _LOG_DEFAULT_LEVEL LOG_WARNING_LEVEL

/// Constant controlling the logging behavior
#ifndef _LOG_LEVEL
/// Sets the maximum debug level to be printed (higher level: lower priority)
#define _LOG_LEVEL _LOG_DEFAULT_LEVEL
#endif

#ifdef _ENABLE_LOG_MESSAGES

#include <stdio.h>

#define _LOG_FILE stderr

/// Print a message with a custom label
#define _PRINT_PREFIX(level, label) fprintf(_LOG_FILE, "[%s:%s:%3d]: ", label, __FILE__, __LINE__)
/// Print a given message
#define _PRINT_MESSAGE(...) fprintf(_LOG_FILE, __VA_ARGS__)

/// Show a message without any decoration nor extra newline characters
#define _SHOW_LOG_NO_DECORATION(level, ...) do {\
    if (level <= _LOG_LEVEL) {\
        _PRINT_MESSAGE(__VA_ARGS__);\
    }} while(0)

/// Show a message with its level decoration and a newline character
#define _SHOW_LOG_MESSAGE(level, label, ...) do {\
    if (level <= _LOG_LEVEL) {\
        _PRINT_PREFIX(level, label);\
        _PRINT_MESSAGE(__VA_ARGS__);\
        _PRINT_MESSAGE("\n");\
    }} while(0)


#else

/// When debug is disabled, log messages can discarded during compilation
#define _SHOW_LOG_MESSAGE(level, ...) do {} while (0)
/// When debug is disabled, log messages without decoration can discarded
/// during compilation
#define _SHOW_LOG_NO_DECORATION(level, ...) do {} while (0)

#endif // _ENABLE_LOG_MESSAGES

/// Log a message with error priority
#define log_error(...) _SHOW_LOG_MESSAGE(LOG_ERROR_LEVEL, "Error",  __VA_ARGS__)
/// Log a message with warning priority
#define log_warning(...) _SHOW_LOG_MESSAGE(LOG_WARNING_LEVEL, "Warning", __VA_ARGS__)
/// Log a message with info priority
#define log_info(...) _SHOW_LOG_MESSAGE(LOG_INFO_LEVEL, "Info", __VA_ARGS__)
/// Log a message with debug priority
#define log_debug(...) _SHOW_LOG_MESSAGE(LOG_DEBUG_LEVEL, "Debug", __VA_ARGS__)
/// Log a message with arbitrary priority level and no decoration nor newline
#define log_no_newline(level, ...) _SHOW_LOG_NO_DECORATION(level, __VA_ARGS__)

#endif /* DEBUG_H */
