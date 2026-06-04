#include "assembler.h"
#include "cpu.h"
#include "isa.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define max_program ram_words

// the out instruction prints a word as a signed decimal, one per line.
static void console_out(word value) {
    printf("%d\n", (int)(int16_t)value);
}

// the in instruction reads a decimal word from standard input.
static word console_in(void) {
    long v = 0;
    if (scanf("%ld", &v) != 1)
        v = 0;
    return (word)v;
}

// read a whole file into a freshly allocated, null-terminated string.
static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)n + 1);
    if (buf) {
        size_t got = fread(buf, 1, (size_t)n, f);
        buf[got] = '\0';
    }
    fclose(f);
    return buf;
}

// does the path end with the given suffix?
static int ends_with(const char *s, const char *suffix) {
    size_t ls = strlen(s), lt = strlen(suffix);
    return ls >= lt && strcmp(s + ls - lt, suffix) == 0;
}

// assemble a source file into prog; returns the word count or -1.
static int assemble_file(const char *path, word *prog, int cap) {
    char *src = read_file(path);
    if (!src) {
        fprintf(stderr, "cpux: cannot read '%s'\n", path);
        return -1;
    }
    char errbuf[256];
    int n = assemble(src, prog, cap, errbuf, sizeof(errbuf));
    free(src);
    if (n < 0)
        fprintf(stderr, "cpux: %s\n", errbuf);
    return n;
}

// load a raw binary of little-endian words; returns the word count or -1.
static int load_binary(const char *path, word *prog, int cap) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cpux: cannot read '%s'\n", path);
        return -1;
    }
    int n = 0;
    while (n < cap) {
        int lo = fgetc(f);
        int hi = fgetc(f);
        if (lo == EOF || hi == EOF)
            break;
        prog[n++] = (word)((hi << 8) | lo);
    }
    fclose(f);
    return n;
}

static int cmd_asm(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "usage: cpux asm <input.asm> [-o <output.bin>]\n");
        return 1;
    }
    const char *in = argv[0];
    const char *out = "out.bin";
    for (int i = 1; i + 1 < argc; i++) {
        if (strcmp(argv[i], "-o") == 0)
            out = argv[i + 1];
    }
    word *prog = malloc(sizeof(word) * max_program);
    int n = assemble_file(in, prog, max_program);
    if (n < 0) {
        free(prog);
        return 1;
    }
    FILE *f = fopen(out, "wb");
    if (!f) {
        fprintf(stderr, "cpux: cannot write '%s'\n", out);
        free(prog);
        return 1;
    }
    for (int i = 0; i < n; i++) {
        fputc(prog[i] & 0xff, f);        // little-endian: low byte first
        fputc((prog[i] >> 8) & 0xff, f);
    }
    fclose(f);
    free(prog);
    printf("assembled %d words -> %s\n", n, out);
    return 0;
}

static int cmd_run(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "usage: cpux run <program.asm|program.bin> [--trace]\n");
        return 1;
    }
    const char *path = argv[0];
    int trace = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--trace") == 0)
            trace = 1;
    }

    word *prog = malloc(sizeof(word) * max_program);
    int n = ends_with(path, ".bin")
                ? load_binary(path, prog, max_program)
                : assemble_file(path, prog, max_program);
    if (n < 0) {
        free(prog);
        return 1;
    }

    cpu *c = calloc(1, sizeof(cpu));
    cpu_init(c);
    c->on_out = console_out;
    c->on_in = console_in;
    cpu_load(c, prog, n);

    long steps = 0;
    const long cap = 10000000;
    while (!c->halted && steps < cap) {
        if (trace)
            fprintf(stderr, "pc=%-5u ins=0x%04x\n", cpu_pc(c), ram_read(&c->mem, cpu_pc(c)));
        cpu_step(c);
        steps++;
    }
    if (steps >= cap)
        fprintf(stderr, "cpux: step limit reached; the program may not halt\n");

    free(c);
    free(prog);
    return 0;
}

static void usage(void) {
    printf("cpux -- a sixteen-bit cpu emulator built up from a single nand gate\n\n");
    printf("usage:\n");
    printf("  cpux run <program.asm|program.bin> [--trace]   assemble (if needed) and run\n");
    printf("  cpux asm <program.asm> [-o <output.bin>]       assemble to a binary\n");
    printf("  cpux help                                      show this message\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        return 1;
    }
    if (strcmp(argv[1], "run") == 0)
        return cmd_run(argc - 2, argv + 2);
    if (strcmp(argv[1], "asm") == 0)
        return cmd_asm(argc - 2, argv + 2);
    if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        usage();
        return 0;
    }
    fprintf(stderr, "cpux: unknown command '%s' (try 'cpux help')\n", argv[1]);
    return 1;
}
