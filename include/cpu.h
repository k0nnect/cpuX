#ifndef cpux_cpu_h
#define cpux_cpu_h

#include "bit.h"
#include "memory.h"

// the whole machine: a register file, a program counter, and main memory,
// plus two hooks for console input and output. cpux is a von neumann design,
// so code and data share the one address space.
typedef struct cpu {
    regfile regs;          // r0..r7
    reg16   pc;            // the program counter
    ram     mem;           // 64k words of memory
    bit     halted;        // set once a hlt (or a bad opcode) executes
    void  (*on_out)(word value); // called by the out instruction
    word  (*on_in)(void);        // called by the in instruction
} cpu;

void cpu_init(cpu *c);
void cpu_load(cpu *c, const word *program, int n); // load a program at address 0
word cpu_pc(const cpu *c);
void cpu_step(cpu *c);                  // fetch, decode and execute one instruction
long cpu_run(cpu *c, long max_steps);   // run until halt or the step cap; returns steps run

#endif
