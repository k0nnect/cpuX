#ifndef cpux_isa_h
#define cpux_isa_h

#include "bit.h"

// cpux is a sixteen-bit machine with sixteen-bit instructions. every word is
// laid out the same way:
//
//   bits 15..12   opcode
//   bits 11.. 9   field d   (destination, or first operand)
//   bits  8.. 6   field a   (second operand / base register)
//   bits  5.. 3   field b   (third operand)
//   bits  2.. 0   funct     (selects the operation for an alu instruction)
//
//   imm8 = bits 7..0   imm6 = bits 5..0 (sign-extended where noted)

// the opcode occupies the top four bits, so there is room for sixteen of them.
enum opcode {
    op_hlt  = 0x0, // stop the machine
    op_alu  = 0x1, // rd = ra <funct> rb
    op_li   = 0x2, // rd = imm8                       (load the low byte)
    op_lui  = 0x3, // rd = (imm8 << 8) | (rd & 0xff)  (load the high byte)
    op_addi = 0x4, // rd = ra + sext(imm6)
    op_ld   = 0x5, // rd = mem[ra + sext(imm6)]
    op_st   = 0x6, // mem[ra + sext(imm6)] = rd
    op_beq  = 0x7, // if rd == ra: pc += sext(imm6)
    op_bne  = 0x8, // if rd != ra: pc += sext(imm6)
    op_blt  = 0x9, // if rd <  ra: pc += sext(imm6)   (signed)
    op_jmp  = 0xa, // pc = rd
    op_jal  = 0xb, // rd = pc + 1; pc = ra            (jump and link)
    op_out  = 0xc, // emit rd to the console
    op_in   = 0xd  // rd = read a word from the console
};

// the funct field selects which operation an op_alu instruction performs.
enum funct {
    fn_add = 0, // rd = ra + rb
    fn_sub = 1, // rd = ra - rb
    fn_and = 2, // rd = ra & rb
    fn_or  = 3, // rd = ra | rb
    fn_xor = 4, // rd = ra ^ rb
    fn_not = 5, // rd = ~ra
    fn_shl = 6, // rd = ra << (rb & 15)
    fn_shr = 7  // rd = ra >> (rb & 15)  (logical)
};

// field extractors: pull each piece out of an instruction word.
static inline int ins_op(word i)   { return (i >> 12) & 0xf; }
static inline int ins_rd(word i)   { return (i >>  9) & 0x7; }
static inline int ins_ra(word i)   { return (i >>  6) & 0x7; }
static inline int ins_rb(word i)   { return (i >>  3) & 0x7; }
static inline int ins_fn(word i)   { return  i        & 0x7; }
static inline int ins_imm8(word i) { return  i        & 0xff; }
static inline int ins_imm6(word i) { return  i        & 0x3f; }

// sign-extend a six-bit field to a full signed integer.
static inline int sext6(int v) {
    return (v & 0x20) ? (v | ~0x3f) : v;
}

#endif
