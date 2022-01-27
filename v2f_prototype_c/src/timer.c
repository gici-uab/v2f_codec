/**
 * @file
 *
 * Implementation of the timer tools.
 */

#include "timer.h"

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/time.h>

// LCOV_EXCL_START

/// Global instance to keep track of named timers
global_timer_t global_timer = {.entry_count = 0};

double timer_get_wall_time() {
    struct timeval time;
    if (gettimeofday(&time, NULL)) {
        return 0;
    }
    return (double) time.tv_sec + (double) time.tv_usec * .000001;
}

void timer_start(char const *const name) {
    bool add_new = true;
    uint16_t target_index = global_timer.entry_count;
    for (uint16_t i = 0; i < global_timer.entry_count; i++) {
        if (strcmp(global_timer.entries[i].name, name) == 0) {
            add_new = false;
            target_index = i;
            break;
        }
    }

    if (add_new && global_timer.entry_count == MAX_TIMERS) {
        fprintf(stderr, "[WARNING] Cannot add any more timers - ignoring.\n");
        return;
    }

    if (strlen(name) >= NAME_SIZE) {
        fprintf(stderr, "[WARNING] Name %s too long - ignoring\n", name);
        return;
    }

    if (!add_new && global_timer.entries[target_index].running) {
        fprintf(stderr, "[ERROR] Timer %s was already running\n", name);
        return;
    }

    global_timer.entries[target_index].clock_before =
            clock();
    global_timer.entries[target_index].clock_after = 0;
    global_timer.entries[target_index].wall_before = timer_get_wall_time();
    global_timer.entries[target_index].wall_after = 0;
    strncpy(global_timer.entries[target_index].name,
            name, NAME_SIZE - 1);

    global_timer.entries[target_index].running = true;

    if (add_new) {
        global_timer.entries[target_index].count = 0;
        global_timer.entries[target_index].total_cpu_s = 0;
        global_timer.entries[target_index].total_wall_s = 0;
        assert(target_index == global_timer.entry_count);
        global_timer.entry_count++;
    }
}

void timer_stop(char const *const name) {
    bool found = false;
    for (uint16_t i = 0; i < global_timer.entry_count; i++) {
        if (strcmp(global_timer.entries[i].name, name) == 0) {
            if (! global_timer.entries[i].running) {
                return;
            }

            global_timer.entries[i].clock_after = clock();
            global_timer.entries[i].wall_after = timer_get_wall_time();
            global_timer.entries[i].total_cpu_s +=
                    ((double) global_timer.entries[i].clock_after -
                     (double) global_timer.entries[i].clock_before)
                    / (double) CLOCKS_PER_SEC;
            global_timer.entries[i].total_wall_s += (
                    global_timer.entries[i].wall_after -
                    global_timer.entries[i].wall_before);
            global_timer.entries[i].running = false;
            global_timer.entries[i].count++;
            found = true;
            break;
        }
    }
    assert(found);
}

double timer_get_cpu_s(char const *const name) {
    for (uint16_t i = 0; i < global_timer.entry_count; i++) {
        if (strcmp(global_timer.entries[i].name, name) == 0) {
            if (global_timer.entries[i].clock_after == 0) {
                return ((double) clock()
                        - (double) global_timer.entries[i].clock_before)
                       / (double) CLOCKS_PER_SEC;
            } else {
                return ((double) global_timer.entries[i].clock_after
                        - (double) global_timer.entries[i].clock_before)
                       / (double) CLOCKS_PER_SEC;
            }
        }
    }

    // Not found
    return -1;
}

double timer_get_wall_s(char const *const name) {
    for (uint16_t i = 0; i < global_timer.entry_count; i++) {
        if (strcmp(global_timer.entries[i].name, name) == 0) {
            if (global_timer.entries[i].wall_after == 0) {
                return ((double) timer_get_wall_time()
                        - (double) global_timer.entries[i].wall_before);
            } else {
                return ((double) global_timer.entries[i].wall_after
                        - (double) global_timer.entries[i].wall_before);
            }
        }
    }

    // Not found
    return -1;
}

void timer_report_csv(FILE *const output_file) {
    fprintf(output_file, "name,finished,total_cpu_seconds,total_wall_seconds,"
                         "exec_count,cpu_s_per_exec,wall_s_per_exec\n");
    for (uint16_t i = 0; i < global_timer.entry_count; i++) {
        double cpu_seconds = ((double) global_timer.entries[i].clock_after
                              - (double) global_timer.entries[i].clock_before)
                             / (double) CLOCKS_PER_SEC;
        fprintf(output_file, "%s,%3.4lf,%.4lf,%s,%ld,%.4lf,%.4lf\n",
                global_timer.entries[i].name,
                global_timer.entries[i].total_cpu_s,
                global_timer.entries[i].total_wall_s,
                cpu_seconds >= 0 ? "true" : "false",
                global_timer.entries[i].count,
                global_timer.entries[i].total_cpu_s / (double) global_timer.entries[i].count,
                global_timer.entries[i].total_wall_s / (double) global_timer.entries[i].count);
    }
}

void timer_report_human(FILE *const output_file) {
    for (uint16_t i = 0; i < global_timer.entry_count; i++) {
        fprintf(output_file, "%s: total %.06lfs (%lu times)\n",
                global_timer.entries[i].name,
                global_timer.entries[i].total_cpu_s,
                global_timer.entries[i].count);
    }
}

void timer_reset() {
    global_timer.entry_count = 0;
}

// LCOV_EXCL_STOP
