#include "alu.h"
#include "gates.h"

// sum is the exclusive-or, carry is the and.
void half_adder(bit a, bit b, bit *sum, bit *carry) {
    *sum = g_xor(a, b);
    *carry = g_and(a, b);
}

// a full adder is two half adders with the carries or-ed together.
void full_adder(bit a, bit b, bit c, bit *sum, bit *carry) {
    bit s1, c1, c2;
    half_adder(a, b, &s1, &c1);
    half_adder(s1, c, sum, &c2);
    *carry = g_or(c1, c2);
}

// ripple the carry from the least significant bit upward.
word add16(word a, word b) {
    word out = 0;
    bit carry = 0;
    for (int i = 0; i < word_bits; i++) {
        bit s;
        full_adder(busbit(a, i), busbit(b, i), carry, &s, &carry);
        out = (word)(out | ((word)s << i));
    }
    return out;
}

word inc16(word a) {
    return add16(a, 1);
}

alu_out alu(word x, word y, alu_ctrl c) {
    // pre-condition the x input: optionally zero it, then optionally negate it.
    x = mux16(x, 0, c.zx);
    x = mux16(x, not16(x), c.nx);
    // pre-condition the y input the same way.
    y = mux16(y, 0, c.zy);
    y = mux16(y, not16(y), c.ny);
    // the function line picks addition or bitwise and.
    word out = mux16(and16(x, y), add16(x, y), c.f);
    // optionally negate the whole result.
    out = mux16(out, not16(out), c.no);

    alu_out r;
    r.out = out;
    r.zr = g_not(or16way(out));        // zero when no line is high
    r.ng = busbit(out, word_bits - 1); // negative when the top bit is set
    return r;
}
