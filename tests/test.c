// the cpux test suite. it walks the same path the project is built along:
// gates, then the alu, then memory, then a couple of whole programs run on the
// assembled machine. run it with `make test`.

#include "alu.h"
#include "assembler.h"
#include "cpu.h"
#include "gates.h"
#include "isa.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>

static int failures = 0;
static int checks = 0;

#define check(cond, msg) do {                              \
    checks++;                                              \
    if (!(cond)) {                                         \
        printf("  fail: %s\n", (msg));                     \
        failures++;                                        \
    }                                                      \
} while (0)

static void test_gates(void) {
    printf("gates\n");
    // nand truth table is the whole foundation; check it exhaustively.
    check(nand(0, 0) == 1 && nand(0, 1) == 1 && nand(1, 0) == 1 && nand(1, 1) == 0, "nand truth table");
    check(g_not(0) == 1 && g_not(1) == 0, "not");
    check(g_and(1, 1) == 1 && g_and(1, 0) == 0, "and");
    check(g_or(0, 0) == 0 && g_or(0, 1) == 1, "or");
    check(g_xor(0, 0) == 0 && g_xor(1, 0) == 1 && g_xor(1, 1) == 0, "xor");
    check(g_mux(0, 1, 0) == 0 && g_mux(0, 1, 1) == 1, "mux selects on sel");
    check(mux16(0x00ff, 0xff00, 1) == 0xff00, "mux16");
    check(and16(0xf0f0, 0x00ff) == 0x00f0, "and16");
    check(or16(0xf000, 0x000f) == 0xf00f, "or16");
    check(xor16(0xffff, 0x0f0f) == 0xf0f0, "xor16");
    check(not16(0x0000) == 0xffff, "not16");
    check(or16way(0) == 0 && or16way(0x0010) == 1, "or16way");
}

static void test_alu(void) {
    printf("alu\n");
    bit s, c;
    half_adder(1, 1, &s, &c);
    check(s == 0 && c == 1, "half adder 1+1");
    full_adder(1, 1, 1, &s, &c);
    check(s == 1 && c == 1, "full adder 1+1+1");
    check(add16(40, 2) == 42, "add16");
    check(inc16(41) == 42, "inc16");
    check((word)(add16(5, inc16(not16(8)))) == (word)(5 - 8), "two's complement subtraction");

    // the six control lines, exercised on the canonical functions.
    alu_ctrl add = {0, 0, 0, 0, 1, 0};   // x + y
    alu_ctrl band = {0, 0, 0, 0, 0, 0};  // x & y
    alu_ctrl zero = {1, 0, 1, 0, 1, 0};  // constant 0
    check(alu(20, 22, add).out == 42, "alu add");
    check(alu(0xf0f0, 0x0ff0, band).out == 0x00f0, "alu and");
    alu_out z = alu(123, 456, zero);
    check(z.out == 0 && z.zr == 1, "alu zero with zr flag");
    // add 0, then negate the output: 0 -> ~0 == 0xffff, whose top bit is set.
    alu_out neg = alu(0, 0, (alu_ctrl){0, 0, 0, 0, 1, 1});
    check(neg.out == 0xffff && neg.ng == 1 && neg.zr == 0, "alu negative flag");
}

static void test_memory(void) {
    printf("memory\n");
    reg16 r = {0};
    reg16_tick(&r, 0x1234, 1);          // load
    check(reg16_out(&r) == 0x1234, "register loads on tick");
    reg16_tick(&r, 0xffff, 0);          // hold
    check(reg16_out(&r) == 0x1234, "register holds when load is low");

    ram *m = calloc(1, sizeof(ram));
    ram_write(m, 100, 0xbeef, 1);
    check(ram_read(m, 100) == 0xbeef, "ram stores and reads back");
    ram_write(m, 100, 0x0000, 0);       // load low: no write
    check(ram_read(m, 100) == 0xbeef, "ram ignores a write when load is low");
    free(m);
}

// programs use the out instruction; capture what they emit.
static word captured[64];
static int ncaptured = 0;
static void capture(word v) {
    if (ncaptured < 64)
        captured[ncaptured++] = v;
}

static int run_source(const char *src) {
    word prog[4096];
    char err[256];
    int n = assemble(src, prog, 4096, err, sizeof(err));
    if (n < 0) {
        printf("  fail: assembler error: %s\n", err);
        failures++;
        return -1;
    }
    cpu *c = calloc(1, sizeof(cpu));
    cpu_init(c);
    c->on_out = capture;
    cpu_load(c, prog, n);
    ncaptured = 0;
    cpu_run(c, 1000000);
    int halted = c->halted;
    free(c);
    return halted ? ncaptured : -1;
}

static void test_programs(void) {
    printf("programs\n");

    // sum 1..5 should be 15.
    const char *sum =
        "  set r1, 0\n"
        "  set r2, 1\n"
        "  set r3, 6\n"
        "loop:\n"
        "  add r1, r1, r2\n"
        "  addi r2, r2, 1\n"
        "  blt r2, r3, loop\n"
        "  out r1\n"
        "  hlt\n";
    int n = run_source(sum);
    check(n == 1 && captured[0] == 15, "sum 1..5 == 15");

    // 6 * 7 by repeated addition, looping through a register-held address.
    const char *mul =
        "  set r0, 0\n"
        "  set r1, 6\n"
        "  set r2, 7\n"
        "  set r3, 0\n"
        "  set r4, 1\n"
        "  set r5, mul_loop\n"
        "mul_loop:\n"
        "  beq r2, r0, done\n"
        "  add r3, r3, r1\n"
        "  sub r2, r2, r4\n"
        "  jmp r5\n"
        "done:\n"
        "  out r3\n"
        "  hlt\n";
    n = run_source(mul);
    check(n == 1 && captured[0] == 42, "6 * 7 == 42");

    // load and store round-trip through memory.
    const char *mem =
        "  set r1, 777\n"
        "  set r2, 200\n"
        "  st r1, r2, 0\n"
        "  ld r3, r2, 0\n"
        "  out r3\n"
        "  hlt\n";
    n = run_source(mem);
    check(n == 1 && captured[0] == 777, "store then load returns 777");

    // the first ten fibonacci numbers.
    const char *fib =
        "  set r0, 0\n"
        "  set r1, 0\n"
        "  set r2, 1\n"
        "  set r3, 10\n"
        "  set r4, 1\n"
        "  set r5, fib_loop\n"
        "fib_loop:\n"
        "  out r2\n"
        "  add r6, r1, r2\n"
        "  mov r1, r2\n"
        "  mov r2, r6\n"
        "  sub r3, r3, r4\n"
        "  beq r3, r0, done\n"
        "  jmp r5\n"
        "done:\n"
        "  hlt\n";
    n = run_source(fib);
    check(n == 10 && captured[0] == 1 && captured[1] == 1 && captured[2] == 2 &&
          captured[9] == 55, "fibonacci 1,1,2,...,55");
}

int main(void) {
    test_gates();
    test_alu();
    test_memory();
    test_programs();
    printf("\n%d checks, %d failures\n", checks, failures);
    if (failures == 0)
        printf("all ok\n");
    return failures == 0 ? 0 : 1;
}
