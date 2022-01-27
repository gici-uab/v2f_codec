/**
 * @file
 *
 * This file implements functions common to the encoder and the decoder.
 *
 * @see v2f_common.h for additional details.
 */

#include <stdio.h>
#include <assert.h>

#include "v2f.h"
#include "common.h"

// LCOV_EXCL_START

uint32_t v2f_get_bit(uint8_t const *const buffer, const uint32_t index) {
    const uint32_t byte_index = index / 8;
    const uint32_t bit_index = index % 8;

    assert(bit_index < 8);

    return (buffer[byte_index] & (UINT8_C(0x80) >> bit_index)) ? UINT32_C(1)
                                                               : UINT32_C(0);
}


void v2f_set_bit(uint8_t *const buffer, const uint32_t index,
                    const uint32_t value) {
    const uint32_t byte_index = index / 8;
    const uint32_t bit_index = index % 8;

    assert(value <= 1);
    assert(bit_index <= 7);

    const uint8_t bit_offset = (uint8_t)(7 - bit_index);

    buffer[byte_index] |= (uint8_t)(UINT8_C(1) << bit_offset);
    buffer[byte_index] ^= (uint8_t)((UINT8_C(1) - value) << bit_offset);
}

bool
v2f_is_all_zero(uint8_t const *const vector, const uint32_t length_bits) {
    assert(length_bits >= 1);

    for (uint32_t i = 0; i < length_bits / 8; i++) {
        if (vector[i] != 0) {
            return false;
        }
    }

    uint32_t extra_bits = length_bits % 8;

    if (extra_bits) {
        if (vector[length_bits / 8] & (UINT8_C(0xFF) << (8 - extra_bits))) {
            return false;
        }
    }

    return true;
}

void debug_show_vector_contents(char *name, uint8_t *vector,
                                uint32_t vector_length_bits) {
    uint64_t checksum = 0;
    const uint32_t byte_size = (vector_length_bits + 7) / 8;

    for (uint32_t i = 0; i < byte_size; i++) {
        checksum += (i + 1) * vector[i];
    }
    printf("[%s:%u]: ", name, vector_length_bits);
    printf("%x %x %x %x %x %x ... %x %x %x %x %x %x :: checksum=%lx\n",
           (int) vector[0],
           byte_size >= 2 ? (int) vector[2] : 0xbad,
           byte_size >= 3 ? (int) vector[3] : 0xbad,
           byte_size >= 4 ? (int) vector[4] : 0xbad,
           byte_size >= 5 ? (int) vector[5] : 0xbad,
           byte_size >= 6 ? (int) vector[6] : 0xbad,

           byte_size >= 6 ? (int) vector[byte_size - 6] : 0xbad,
           byte_size >= 5 ? (int) vector[byte_size - 5] : 0xbad,
           byte_size >= 4 ? (int) vector[byte_size - 4] : 0xbad,
           byte_size >= 3 ? (int) vector[byte_size - 3] : 0xbad,
           byte_size >= 2 ? (int) vector[byte_size - 2] : 0xbad,
           (int) vector[byte_size - 1],
           checksum);
}
// LCOV_EXCL_STOP
