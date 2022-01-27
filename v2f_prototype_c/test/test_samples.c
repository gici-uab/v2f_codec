/**
 * @file
 *
 * Definition of the list of all available test samples.
 *
 * @see test_samples.h for further information.
 *
 */

#include "test_samples.h"

v2f_test_sample_t all_test_samples[V2F_C_TEST_SAMPLE_COUNT] = {
    {
        .path = "./testdata/unittest_samples/example_sample_u16be-1x8x8.raw",
        .description = "Test from example_sample_u16be-1x8x8.raw",
        .bytes = 128,
    },
    {
        .path = "./testdata/unittest_samples/example_sample_u16be-1x1024x1024.raw",
        .description = "Test from example_sample_u16be-1x1024x1024.raw",
        .bytes = 2097152,
    },
    {
        .path = "./testdata/unittest_samples/example_sample_u8be-1x8x8.raw",
        .description = "Test from example_sample_u8be-1x8x8.raw",
        .bytes = 64,
    },
    {
        .path = "./testdata/unittest_samples/example_sample_u8be-1x257x257.raw",
        .description = "Test from example_sample_u8be-1x257x257.raw",
        .bytes = 66049,
    },
    {
        .path = "./testdata/unittest_samples/example_sample_u16be-1x257x257.raw",
        .description = "Test from example_sample_u16be-1x257x257.raw",
        .bytes = 132098,
    },
    {
        .path = "./testdata/unittest_samples/example_sample_u16be-1x256x256.raw",
        .description = "Test from example_sample_u16be-1x256x256.raw",
        .bytes = 131072,
    },
    {
        .path = "./testdata/unittest_samples/example_sample_u8be-1x256x256.raw",
        .description = "Test from example_sample_u8be-1x256x256.raw",
        .bytes = 65536,
    },
    {
        .path = "./testdata/unittest_samples/example_sample_u8be-1x1024x1024.raw",
        .description = "Test from example_sample_u8be-1x1024x1024.raw",
        .bytes = 1048576,
    },
};
