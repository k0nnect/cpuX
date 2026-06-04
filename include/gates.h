#ifndef cpux_gates_h
#define cpux_gates_h

#include "bit.h"

// nand is the one primitive of cpux. every gate below is built only from it,
// the same way real digital logic can be derived from a single universal gate.
bit nand(bit a, bit b);

// the elementary one-bit gates, each derived from nand.
bit g_not(bit a);
bit g_and(bit a, bit b);
bit g_or(bit a, bit b);
bit g_xor(bit a, bit b);

// multiplexer: out = sel ? b : a.
bit g_mux(bit a, bit b, bit sel);
// demultiplexer: routes in to a when sel is 0, to b when sel is 1.
void g_dmux(bit in, bit sel, bit *a, bit *b);

// bus-wide gates: the one-bit gates applied across all sixteen lines.
word not16(word a);
word and16(word a, word b);
word or16(word a, word b);
word xor16(word a, word b);
word mux16(word a, word b, bit sel);

// or-reduce a bus down to a single bit: 1 if any line is high.
bit or16way(word a);

#endif
