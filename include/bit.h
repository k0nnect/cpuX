#ifndef cpux_bit_h
#define cpux_bit_h

#include <stdint.h>

// a single logic bit. by convention it is always 0 or 1.
typedef uint8_t bit;

// a machine word: the width of every bus and register in cpux.
typedef uint16_t word;

// how many bits live on a bus.
#define word_bits 16

// pull bit i out of a word (i counted from the least significant end).
static inline bit busbit(word w, int i) {
    return (bit)((w >> i) & 1u);
}

#endif
