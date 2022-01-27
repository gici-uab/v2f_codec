/**
 * @file
 *
 * Miscellaneous unit tests.
 */

#include <stdio.h>
#include "CUExtension.h"
#include "test_common.h"
#include "math.h"

#include "../src/timer.h"

/**
 * Test the timer with at most one repetition per label.
 */
void test_basic_usage(void);

/**
 * Test the timer with multiple repetitions of the same label.
 */
void test_multiple_count(void);

void test_basic_usage() {
    char *names[] = {
            "n",
            "name",
            "Name",
            "NAME",
            "naaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaame",
            "",
    };
    const uint8_t name_count = sizeof(names) / sizeof(char *);

    timer_reset();

    for (uint16_t i = 0; i < name_count; i++) {
        CU_ASSERT_EQUAL_FATAL(
                timer_get_cpu_s(names[i]), (double) -1);
        timer_start(names[i]);
    }
    for (uint16_t i = 0; i < name_count; i++) {
        CU_ASSERT_EQUAL_FATAL(
                0, strcmp(global_timer.entries[i].name, names[i]))
    }
    for (uint16_t i = 0; i < name_count; i++) {
        timer_stop(names[i]);
        global_timer.entries[i].clock_before =
                global_timer.entries[i].clock_after - CLOCKS_PER_SEC * i;
        global_timer.entries[i].wall_before =
                global_timer.entries[i].wall_after - i;
    }
    for (uint16_t i = 0; i < name_count; i++) {
        double time_diff = fabs(timer_get_cpu_s(names[i]) - (double) i);
        CU_ASSERT_FATAL(time_diff < TIMER_TOLERANCE);
    }
    FILE *tmp_file = tmpfile();
    timer_report_csv(tmp_file);
    fclose(tmp_file);
}

void test_multiple_count(void) {
    char *names[] = {
            "n",
            "name",
            "Name",
            "NAME",
            "naaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaame",
            "",
    };
    const uint8_t name_count = sizeof(names) / sizeof(char *);


    timer_reset();
    for (uint8_t i = 0; i < name_count; i++) {
        const uint16_t repetition_count = (uint16_t) (10 * (1 + (uint16_t) i));
        CU_ASSERT_EQUAL_FATAL(global_timer.entry_count, i);
        for (uint16_t j = 0; j < repetition_count; j++) {
            timer_start(names[i]);
            CU_ASSERT_EQUAL_FATAL(global_timer.entries[i].count, j);
            global_timer.entries[i].wall_before -= 1;
            global_timer.entries[i].clock_before -= CLOCKS_PER_SEC;
            timer_stop(names[i]);
            CU_ASSERT_EQUAL_FATAL(global_timer.entries[i].count, j + 1);
        }
        CU_ASSERT_FATAL(fabs(global_timer.entries[i].total_wall_s - repetition_count) < TIMER_TOLERANCE);
        CU_ASSERT_FATAL(fabs(global_timer.entries[i].total_cpu_s - repetition_count) < TIMER_TOLERANCE);
    }
}


CU_START_REGISTRATION(timer)
    CU_QADD_TEST(test_basic_usage)
    CU_QADD_TEST(test_multiple_count)
CU_END_REGISTRATION()
