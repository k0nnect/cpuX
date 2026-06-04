#ifndef cpux_alu_h
#define cpux_alu_h

#include "bit.h"

// half adder: the sum and carry of two bits.
void half_adder(bit a, bit b, bit *sum, bit *carry);
// full adder: the sum and carry of three bits.
void full_adder(bit a, bit b, bit c, bit *sum, bit *carry);

// a sixteen-bit ripple-carry adder, built from full adders.
word add16(word a, word b);
// add one, built from the adder.
word inc16(word a);

// the six control lines of the alu. with these alone the unit can compute
// every function it needs: each line pre-conditions an input or the output.
typedef struct {
    bit zx; // zero the x input
    bit nx; // negate the x input
    bit zy; // zero the y input
    bit ny; // negate the y input
    bit f;  // function: 1 selects add, 0 selects and
    bit no; // negate the output
} alu_ctrl;

// the alu returns its result together with two status bits.
typedef struct {
    word out;
    bit zr; // the result is zero
    bit ng; // the result is negative (its most significant bit is set)
} alu_out;

// run the arithmetic logic unit on x and y under the given control lines.
alu_out alu(word x, word y, alu_ctrl c);

#endif
