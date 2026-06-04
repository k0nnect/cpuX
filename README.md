# cpuX

a sixteen-bit cpu, an assembler, and the digital logic underneath it — all written from scratch in c, and built strictly from the bottom up. the only primitive the whole machine is allowed to assume is a single nand gate; everything above it is constructed by hand.

> a note on style: the name is written `cpuX`, but the codebase and these docs otherwise keep a deliberate all-lowercase aesthetic. that is on purpose, not an oversight.

## what it is

cpuX answers a question worth asking once: if you start with nothing but a nand gate, how much machine can you build, and can you watch a real program run on it? the layers stack like this:

```
nand
 └─ gates        not, and, or, xor, mux, dmux        (combinational logic)
     └─ alu      adders, the arithmetic logic unit    (arithmetic)
         └─ dff  the one sequential primitive         (memory in time)
             └─ registers, register file, ram         (state)
                 └─ cpu   fetch / decode / execute     (the machine)
                     └─ assembler   text -> machine code
                         └─ programs
```

each layer is written only in terms of the layer below it. the gates are real gate logic, not c operators standing in for them; the adder is real full adders; the registers are real flip-flops behind a load line. the result is small enough to read in an afternoon and complete enough to run fibonacci.

## the architecture

- **word size:** 16 bits. every register, bus, and memory cell is one word.
- **registers:** eight general-purpose registers, `r0` through `r7`.
- **program counter:** a dedicated 16-bit register.
- **memory:** 65536 words in a single address space — code and data share it (a von neumann design). programs load at address 0 and the counter starts there.
- **instructions:** every instruction is exactly one word, laid out as

  ```
  bits 15..12  opcode
  bits 11.. 9  field d    destination, or first operand
  bits  8.. 6  field a    second operand / base register
  bits  5.. 3  field b    third operand
  bits  2.. 0  funct      operation selector for an alu instruction

  imm8 = bits 7..0     imm6 = bits 5..0   (sign-extended where noted)
  ```

## the instruction set

| opcode | mnemonic | form              | meaning                                  |
|:------:|:---------|:------------------|:-----------------------------------------|
| 0x0    | `hlt`    |                   | stop the machine                         |
| 0x1    | `add`    | `rd, ra, rb`      | `rd = ra + rb`                           |
| 0x1    | `sub`    | `rd, ra, rb`      | `rd = ra - rb`                           |
| 0x1    | `and`    | `rd, ra, rb`      | `rd = ra & rb`                           |
| 0x1    | `or`     | `rd, ra, rb`      | `rd = ra \| rb`                          |
| 0x1    | `xor`    | `rd, ra, rb`      | `rd = ra ^ rb`                           |
| 0x1    | `not`    | `rd, ra`          | `rd = ~ra`                               |
| 0x1    | `shl`    | `rd, ra, rb`      | `rd = ra << (rb & 15)`                   |
| 0x1    | `shr`    | `rd, ra, rb`      | `rd = ra >> (rb & 15)` (logical)         |
| 0x2    | `li`     | `rd, imm`         | `rd = imm` (low byte, zero-extended)     |
| 0x3    | `lui`    | `rd, imm`         | `rd = (imm << 8) \| (rd & 0xff)`         |
| 0x4    | `addi`   | `rd, ra, imm`     | `rd = ra + imm` (imm is 6-bit signed)    |
| 0x5    | `ld`     | `rd, ra, imm`     | `rd = mem[ra + imm]`                     |
| 0x6    | `st`     | `rd, ra, imm`     | `mem[ra + imm] = rd`                     |
| 0x7    | `beq`    | `rd, ra, label`   | branch to label if `rd == ra`            |
| 0x8    | `bne`    | `rd, ra, label`   | branch to label if `rd != ra`            |
| 0x9    | `blt`    | `rd, ra, label`   | branch to label if `rd < ra` (signed)    |
| 0xa    | `jmp`    | `rd`              | `pc = rd`                                |
| 0xb    | `jal`    | `rd, ra`          | `rd = pc + 1; pc = ra` (call)            |
| 0xc    | `out`    | `rd`              | print `rd` to the console                |
| 0xd    | `in`     | `rd`              | read a word from the console into `rd`   |

the seven alu instructions share opcode `0x1` and are told apart by the `funct` field — a small, satisfying piece of the design, since the same three control-style bits steer the gate-level alu.

branches are pc-relative with a 6-bit signed offset, so a branch can reach about thirty words in either direction. for longer jumps, load an address into a register with `set` and use `jmp`.

## assembly language

- one instruction per line; blank lines are fine.
- comments run from `;` or `#` to the end of the line.
- labels are a name followed by `:`, alone on a line or in front of an instruction.
- operands are separated by spaces or commas, whichever reads better.
- immediates may be decimal (`42`, `-3`), hexadecimal (`0x2a`), or a label name.

**directives**

- `.word value` — emit one raw word of data.

**pseudo-instructions** (conveniences the assembler expands for you)

- `set rd, value` — load a full 16-bit constant or address. expands to `li` + `lui`.
- `mov rd, ra` — copy a register. expands to `or rd, ra, ra`.
- `nop` — do nothing. expands to `addi r0, r0, 0`.

a small example:

```asm
; count down from 5 to 1
  set r0, 0
  set r1, 5
  set r2, 1
loop:
  out r1            ; print the counter
  sub r1, r1, r2    ; counter -= 1
  blt r0, r1, loop  ; while 0 < counter, repeat
  hlt
```

## building

you need a c11 compiler and make.

```sh
make          # build the cpux binary
make test     # build and run the test suite
make clean    # remove build artifacts
```

## usage

```sh
# assemble (if needed) and run a program
./cpux run examples/fibonacci.asm

# trace every fetch as it happens
./cpux run examples/multiply.asm --trace

# assemble to a raw little-endian binary, then run that
./cpux asm examples/fibonacci.asm -o fib.bin
./cpux run fib.bin

# or through make
make run prog=examples/countdown.asm
```

the `out` instruction prints one signed decimal per line; `in` reads one decimal from standard input.

## examples

- `examples/countdown.asm` — counts down from 5, a first look at a loop and a branch.
- `examples/multiply.asm` — multiplies by repeated addition (the machine has no multiply), using a register-held jump target.
- `examples/fibonacci.asm` — prints the first ten fibonacci numbers.

## tests

`make test` walks the project the same way it is built: the nand truth table and every gate, the adders and the alu with its status flags, the flip-flop-backed registers and ram, and finally whole programs assembled and run on the machine with their output checked.

## project layout

```
include/    the interfaces, one header per layer
  bit.h         the bit and word types
  gates.h       nand and the gates built from it
  alu.h         adders and the arithmetic logic unit
  memory.h      the flip-flop, registers, and ram
  isa.h         opcodes, instruction encoding, field extractors
  cpu.h         the machine
  assembler.h   text to machine code
src/        the implementations
tests/      the test suite
examples/   sample programs in cpux assembly
```

## design notes

a few choices worth calling out:

- **one true primitive.** the gates are derived from `nand` alone, by de morgan and the classic four-nand xor, rather than reaching for c's `&` and `|`. the point of the project is to *not* take logic for granted.
- **the flip-flop is the only thing that remembers.** combinational logic forgets the moment its inputs change. the `dff` is the single sequential primitive, and every register and memory cell is built on top of it behind a load line driven by a mux — exactly the trick that turns gates into state.
- **two's complement throughout.** subtraction is `a + (~b + 1)`, branches compare signed words, and the alu reports zero and negative status bits, so the same hardware serves signed and unsigned code.
- **a real two-pass assembler.** the first pass fixes every label's address; the second emits code and resolves forward references, range-checking branch offsets and immediates as it goes.

## license

mit. see [license](LICENSE).
