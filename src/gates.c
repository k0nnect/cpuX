#include "gates.h"

// the universal gate. everything in cpux is, ultimately, a pile of these.
bit nand(bit a, bit b) {
    return (bit)(!(a && b));
}

// not a = nand(a, a).
bit g_not(bit a) {
    return nand(a, a);
}

// and a b = not(nand(a, b)).
bit g_and(bit a, bit b) {
    return g_not(nand(a, b));
}

// or a b = nand(not a, not b), by de morgan's law.
bit g_or(bit a, bit b) {
    return nand(g_not(a), g_not(b));
}

// the classic four-nand exclusive-or.
bit g_xor(bit a, bit b) {
    bit n = nand(a, b);
    return nand(nand(a, n), nand(b, n));
}

// select b when sel is high, a when sel is low.
bit g_mux(bit a, bit b, bit sel) {
    return g_or(g_and(a, g_not(sel)), g_and(b, sel));
}

// steer a single bit toward one of two outputs.
void g_dmux(bit in, bit sel, bit *a, bit *b) {
    *a = g_and(in, g_not(sel));
    *b = g_and(in, sel);
}

word not16(word a) {
    word out = 0;
    for (int i = 0; i < word_bits; i++)
        out = (word)(out | ((word)g_not(busbit(a, i)) << i));
    return out;
}

word and16(word a, word b) {
    word out = 0;
    for (int i = 0; i < word_bits; i++)
        out = (word)(out | ((word)g_and(busbit(a, i), busbit(b, i)) << i));
    return out;
}

word or16(word a, word b) {
    word out = 0;
    for (int i = 0; i < word_bits; i++)
        out = (word)(out | ((word)g_or(busbit(a, i), busbit(b, i)) << i));
    return out;
}

word xor16(word a, word b) {
    word out = 0;
    for (int i = 0; i < word_bits; i++)
        out = (word)(out | ((word)g_xor(busbit(a, i), busbit(b, i)) << i));
    return out;
}

word mux16(word a, word b, bit sel) {
    word out = 0;
    for (int i = 0; i < word_bits; i++)
        out = (word)(out | ((word)g_mux(busbit(a, i), busbit(b, i), sel) << i));
    return out;
}

bit or16way(word a) {
    bit r = 0;
    for (int i = 0; i < word_bits; i++)
        r = g_or(r, busbit(a, i));
    return r;
}
