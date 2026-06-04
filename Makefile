cc     := cc
cflags := -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Iinclude -O2
build  := build

lib_src := src/gates.c src/alu.c src/memory.c src/cpu.c src/assembler.c
lib_obj := $(lib_src:src/%.c=$(build)/%.o)

.PHONY: all test clean run

all: cpux

cpux: $(lib_obj) $(build)/main.o
	$(cc) $(cflags) $^ -o cpux

$(build)/%.o: src/%.c | $(build)
	$(cc) $(cflags) -c $< -o $@

$(build)/test.o: tests/test.c | $(build)
	$(cc) $(cflags) -c $< -o $@

test: $(lib_obj) $(build)/test.o
	$(cc) $(cflags) $^ -o cpux_test
	./cpux_test

# run an example, e.g. `make run prog=examples/fibonacci.asm`
run: cpux
	./cpux run $(prog)

$(build):
	mkdir -p $(build)

clean:
	rm -rf $(build) cpux cpux_test out.bin
