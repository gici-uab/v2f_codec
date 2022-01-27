/**
 * @file
 *
 * @brief Tools to measure execution time.
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

/// Maximum tolerance stored in the timer, in seconds
#define TIMER_TOLERANCE ((double) 1e-2)

/// Maximum number of concurrent timers
#define MAX_TIMERS 256
/// Maximum name size for each timer
#define NAME_SIZE 256

/**
 * @struct timer_entry_t
 *
 * Structure representing one timer entry.
 */
typedef struct {
    /// Timer name
    char name[NAME_SIZE];

    /// @name Current run values (for the last start/stop cycle)

    /// Is the timer running?
    bool running;
    /// Clock value when the timer was started.
    clock_t clock_before;
    /// Clock value when the timer was stopped (if it was stoppeD).
    clock_t clock_after;
    /// Wall time value when the timer was started.
    double wall_before;
    /// Wall time value when the timer was stopped (if it was stoppeD).
    double wall_after;

    /// @name Global run tracking

    /// Number of start/stop cycles a timer has been run
    uint64_t count;
    /// Total CPU time in seconds for all start/stop cycles
    double total_cpu_s;
    /// Total wall time in seconds for all start/stop cycles
    double total_wall_s;
} timer_entry_t;

/**
 * @struct global_timer_t
 *
 * Struct holding timer entries.
 */
typedef struct {
    /// Timer entries
    timer_entry_t entries[MAX_TIMERS];
    /// Number of used entries
    uint16_t entry_count;
} global_timer_t;

extern global_timer_t global_timer;

/**
 * @return the current wall time
 */
double timer_get_wall_time(void);

/**
 * Start a named timer. The name must be unique.
 *
 * @param name \0 ended string, case sensitive, that identifies this timer.
 *   Must have length <= 255.
 */
void timer_start(char const *const name);

/**
 * Stop a named timer. The name must have been used.
 *
 * @param name \0 ended string, case sensitive, that identifies this timer.
 *   Must have length <= 255.
 */
void timer_stop(char const *const name);

/**
 * Get the current or total process time of a started or finished named timer.
 * @param name name of the timer
 * @return CPU execution time in seconds, or -1 if the name is not found in
 *   @ref global_timer.
 */
double timer_get_cpu_s(char const *const name);

/**
 * Get the current or total wall time of a started or finished named timer.
 * @param name name of the timer
 * @return wall execution time, or -1 if the name is not found in
 *   @ref global_timer
 */
double timer_get_wall_s(char const *const name);

/**
 * Report the timer state into @a output_file in CSV format.
 * @param output_file file where the report is output (e.g., stdout)
 */
void timer_report_csv(FILE *const output_file);

/**
 * Report the timer state into @a output_file in a human-readable way
 * @param output_file file where the report is output (e.g., stdout)
 */
void timer_report_human(FILE *const output_file);

/**
 * Resets the timer erasing any previous information
 */
void timer_reset(void);

#endif // TIMER_H
