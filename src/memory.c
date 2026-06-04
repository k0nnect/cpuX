#include "memory.h"
#include "gates.h"

bit dff_out(const dff *d) {
    return d->q;
}

void dff_tick(dff *d, bit in) {
    d->q = (bit)(in & 1u);
}

word reg16_out(const reg16 *r) {
    word out = 0;
    for (int i = 0; i < word_bits; i++)
        out = (word)(out | ((word)dff_out(&r->bits[i]) << i));
    return out;
}

void reg16_tick(reg16 *r, word in, bit load) {
    // the load line drives a mux in front of every flip-flop: when it is low
    // each bit feeds its own output back to itself and the value holds.
    word cur = reg16_out(r);
    word next = mux16(cur, in, load);
    for (int i = 0; i < word_bits; i++)
        dff_tick(&r->bits[i], busbit(next, i));
}

word regfile_read(const regfile *rf, int idx) {
    return reg16_out(&rf->r[idx & 7]);
}

void regfile_write(regfile *rf, int idx, word in, bit load) {
    reg16_tick(&rf->r[idx & 7], in, load);
}

word ram_read(const ram *m, word addr) {
    return m->cell[addr];
}

void ram_write(ram *m, word addr, word in, bit load) {
    // a real array of word registers; the load line gates the write.
    if (load)
        m->cell[addr] = in;
}
