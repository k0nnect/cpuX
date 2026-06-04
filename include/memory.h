#ifndef cpux_memory_h
#define cpux_memory_h

#include "bit.h"

// the data flip-flop is the one sequential primitive of cpux: it remembers a
// single bit from one clock tick to the next. combinational gates forget the
// instant their inputs change; the flip-flop is what gives the machine memory.
typedef struct {
    bit q;
} dff;

bit dff_out(const dff *d);
void dff_tick(dff *d, bit in); // on the clock edge: q <- in

// a sixteen-bit register: sixteen flip-flops behind a load line. when load is
// high the bus is latched, otherwise the stored value holds.
typedef struct {
    dff bits[word_bits];
} reg16;

word reg16_out(const reg16 *r);
void reg16_tick(reg16 *r, word in, bit load);

// the register file: eight general-purpose registers, r0 through r7.
typedef struct {
    reg16 r[8];
} regfile;

word regfile_read(const regfile *rf, int idx);
void regfile_write(regfile *rf, int idx, word in, bit load);

// main memory: a flat array of words, addressed by a full sixteen-bit word.
#define ram_words 65536
typedef struct {
    word cell[ram_words];
} ram;

word ram_read(const ram *m, word addr);
void ram_write(ram *m, word addr, word in, bit load);

#endif
