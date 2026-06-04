#include "cpu.h"
#include "gates.h"
#include "alu.h"
#include "isa.h"

void cpu_init(cpu *c) {
    for (int i = 0; i < 8; i++)
        regfile_write(&c->regs, i, 0, 1);
    reg16_tick(&c->pc, 0, 1);
    for (long i = 0; i < ram_words; i++)
        c->mem.cell[i] = 0;
    c->halted = 0;
    c->on_out = 0;
    c->on_in = 0;
}

void cpu_load(cpu *c, const word *program, int n) {
    for (int i = 0; i < n; i++)
        c->mem.cell[i] = program[i];
}

word cpu_pc(const cpu *c) {
    return reg16_out(&c->pc);
}

void cpu_step(cpu *c) {
    if (c->halted)
        return;

    // fetch: read the instruction the program counter points at.
    word pc = reg16_out(&c->pc);
    word ins = ram_read(&c->mem, pc);
    word next = inc16(pc); // by default control flows to the next word

    // decode: split out the fields and read the three named registers.
    int op = ins_op(ins);
    int d = ins_rd(ins), a = ins_ra(ins), b = ins_rb(ins);
    word vd = regfile_read(&c->regs, d);
    word va = regfile_read(&c->regs, a);
    word vb = regfile_read(&c->regs, b);
    word off = (word)sext6(ins_imm6(ins));

    // execute.
    switch (op) {
    case op_hlt:
        c->halted = 1;
        break;

    case op_alu: {
        word res = 0;
        switch (ins_fn(ins)) {
        case fn_add: res = add16(va, vb); break;
        case fn_sub: res = add16(va, inc16(not16(vb))); break; // va + (-vb)
        case fn_and: res = and16(va, vb); break;
        case fn_or:  res = or16(va, vb);  break;
        case fn_xor: res = xor16(va, vb); break;
        case fn_not: res = not16(va);     break;
        case fn_shl: res = (word)(va << (vb & 15)); break;
        case fn_shr: res = (word)(va >> (vb & 15)); break;
        }
        regfile_write(&c->regs, d, res, 1);
        break;
    }

    case op_li:
        regfile_write(&c->regs, d, (word)ins_imm8(ins), 1);
        break;

    case op_lui:
        regfile_write(&c->regs, d, (word)(((ins_imm8(ins) & 0xff) << 8) | (vd & 0xff)), 1);
        break;

    case op_addi:
        regfile_write(&c->regs, d, add16(va, off), 1);
        break;

    case op_ld:
        regfile_write(&c->regs, d, ram_read(&c->mem, add16(va, off)), 1);
        break;

    case op_st:
        ram_write(&c->mem, add16(va, off), vd, 1);
        break;

    case op_beq:
        if (vd == va) next = add16(pc, off);
        break;

    case op_bne:
        if (vd != va) next = add16(pc, off);
        break;

    case op_blt:
        if ((int16_t)vd < (int16_t)va) next = add16(pc, off);
        break;

    case op_jmp:
        next = vd;
        break;

    case op_jal:
        regfile_write(&c->regs, d, next, 1); // link register holds the return address
        next = va;
        break;

    case op_out:
        if (c->on_out) c->on_out(vd);
        break;

    case op_in:
        regfile_write(&c->regs, d, c->on_in ? c->on_in() : 0, 1);
        break;

    default:
        c->halted = 1; // an unknown opcode stops the machine
        break;
    }

    reg16_tick(&c->pc, next, 1);
}

long cpu_run(cpu *c, long max_steps) {
    long steps = 0;
    while (!c->halted && steps < max_steps) {
        cpu_step(c);
        steps++;
    }
    return steps;
}
