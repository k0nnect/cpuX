#include "assembler.h"
#include "isa.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define max_syms 1024
#define max_tok  8

typedef struct {
    char name[32];
    int addr;
} sym;

// split a line in place into whitespace- and comma-separated tokens.
static int split_tokens(char *line, char *tok[], int maxtok) {
    int n = 0;
    char *p = line;
    while (*p && n < maxtok) {
        while (*p && (isspace((unsigned char)*p) || *p == ','))
            p++;
        if (!*p)
            break;
        tok[n++] = p;
        while (*p && !isspace((unsigned char)*p) && *p != ',')
            p++;
        if (*p)
            *p++ = '\0';
    }
    return n;
}

// lowercase a token in place (the codebase keeps everything lowercase, and so
// the mnemonics are matched case-insensitively for the writer's convenience).
static void lower(char *s) {
    for (; *s; s++)
        *s = (char)tolower((unsigned char)*s);
}

// strip a trailing comment introduced by ';' or '#'.
static void strip_comment(char *line) {
    for (char *p = line; *p; p++) {
        if (*p == ';' || *p == '#') {
            *p = '\0';
            return;
        }
    }
}

// parse a register token of the form r0..r7. returns 1 on success.
static int reg_num(const char *t, int *out) {
    if (t[0] == 'r' && t[1] >= '0' && t[1] <= '7' && t[2] == '\0') {
        *out = t[1] - '0';
        return 1;
    }
    return 0;
}

// parse an immediate: a decimal/hex number (base 0 handles 0x and a sign) or a
// known label. returns 1 on success.
static int imm_val(const char *t, const sym *syms, int nsyms, int *out) {
    char *end;
    long v = strtol(t, &end, 0);
    if (*end == '\0') {
        *out = (int)v;
        return 1;
    }
    for (int i = 0; i < nsyms; i++) {
        if (strcmp(syms[i].name, t) == 0) {
            *out = syms[i].addr;
            return 1;
        }
    }
    return 0;
}

// how many words a mnemonic emits. only the set pseudo-instruction is wider
// than one word, expanding to a li/lui pair.
static int insn_size(const char *mn) {
    if (strcmp(mn, "set") == 0)
        return 2;
    return 1;
}

// instruction encoders, one per instruction shape.
static word enc(int op, int d, int a, int b, int fn) {
    return (word)(((op & 0xf) << 12) | ((d & 7) << 9) | ((a & 7) << 6) | ((b & 7) << 3) | (fn & 7));
}
static word enc_imm8(int op, int d, int imm) {
    return (word)(((op & 0xf) << 12) | ((d & 7) << 9) | (imm & 0xff));
}
static word enc_imm6(int op, int d, int a, int imm) {
    return (word)(((op & 0xf) << 12) | ((d & 7) << 9) | ((a & 7) << 6) | (imm & 0x3f));
}

// small helpers to keep the error paths short and uniform.
#define fail(...) do { snprintf(err, (size_t)errcap, __VA_ARGS__); free(buf); return -1; } while (0)
#define need_reg(t, dst) do { if (!reg_num((t), (dst))) fail("line %d: expected a register, got '%s'", lineno, (t)); } while (0)
#define need_imm(t, dst) do { if (!imm_val((t), syms, nsyms, (dst))) fail("line %d: bad immediate or unknown label '%s'", lineno, (t)); } while (0)
#define emit(w) do { if (addr >= cap) fail("program too large at line %d", lineno); out[addr++] = (word)(w); } while (0)

int assemble(const char *source, word *out, int cap, char *err, int errcap) {
    sym syms[max_syms];
    int nsyms = 0;
    char *buf = NULL;
    int addr;
    int lineno;
    char *save;

    // ---- pass one: walk the source, recording each label's address. ----
    buf = strdup(source);
    if (!buf)
        fail("out of memory");
    addr = 0;
    lineno = 0;
    for (char *line = strtok_r(buf, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        lineno++;
        strip_comment(line);
        char *tok[max_tok];
        int nt = split_tokens(line, tok, max_tok);
        if (nt == 0)
            continue;
        int ti = 0;
        size_t len = strlen(tok[0]);
        if (len > 0 && tok[0][len - 1] == ':') {
            tok[0][len - 1] = '\0';
            if (nsyms >= max_syms)
                fail("too many labels at line %d", lineno);
            strncpy(syms[nsyms].name, tok[0], sizeof(syms[nsyms].name) - 1);
            syms[nsyms].name[sizeof(syms[nsyms].name) - 1] = '\0';
            syms[nsyms].addr = addr;
            nsyms++;
            ti = 1;
        }
        if (ti >= nt)
            continue; // the line held only a label
        char *mn = tok[ti];
        lower(mn);
        if (strcmp(mn, ".word") == 0)
            addr += 1;
        else
            addr += insn_size(mn);
    }
    free(buf);

    // ---- pass two: emit machine words, resolving labels as we go. ----
    buf = strdup(source);
    if (!buf)
        fail("out of memory");
    addr = 0;
    lineno = 0;
    for (char *line = strtok_r(buf, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        lineno++;
        strip_comment(line);
        char *tok[max_tok];
        int nt = split_tokens(line, tok, max_tok);
        if (nt == 0)
            continue;
        int ti = 0;
        size_t len = strlen(tok[0]);
        if (len > 0 && tok[0][len - 1] == ':')
            ti = 1; // skip the label; pass one already recorded it
        if (ti >= nt)
            continue;

        char *mn = tok[ti];
        lower(mn);
        char **arg = &tok[ti + 1];   // operands begin after the mnemonic
        int nargs = nt - ti - 1;
        int rd, ra, rb, imm;
        int here = addr;             // address of this instruction, for branches

        if (strcmp(mn, ".word") == 0) {
            if (nargs != 1) fail("line %d: .word takes one value", lineno);
            need_imm(arg[0], &imm);
            emit((word)imm);

        } else if (strcmp(mn, "hlt") == 0) {
            emit(enc(op_hlt, 0, 0, 0, 0));

        } else if (strcmp(mn, "nop") == 0) {
            emit(enc_imm6(op_addi, 0, 0, 0)); // r0 = r0 + 0

        } else if (strcmp(mn, "add") == 0 || strcmp(mn, "sub") == 0 ||
                   strcmp(mn, "and") == 0 || strcmp(mn, "or")  == 0 ||
                   strcmp(mn, "xor") == 0 || strcmp(mn, "shl") == 0 ||
                   strcmp(mn, "shr") == 0) {
            int fn = strcmp(mn, "add") == 0 ? fn_add :
                     strcmp(mn, "sub") == 0 ? fn_sub :
                     strcmp(mn, "and") == 0 ? fn_and :
                     strcmp(mn, "or")  == 0 ? fn_or  :
                     strcmp(mn, "xor") == 0 ? fn_xor :
                     strcmp(mn, "shl") == 0 ? fn_shl : fn_shr;
            if (nargs != 3) fail("line %d: %s takes three registers", lineno, mn);
            need_reg(arg[0], &rd);
            need_reg(arg[1], &ra);
            need_reg(arg[2], &rb);
            emit(enc(op_alu, rd, ra, rb, fn));

        } else if (strcmp(mn, "not") == 0) {
            if (nargs != 2) fail("line %d: not takes two registers", lineno);
            need_reg(arg[0], &rd);
            need_reg(arg[1], &ra);
            emit(enc(op_alu, rd, ra, 0, fn_not));

        } else if (strcmp(mn, "mov") == 0) {
            if (nargs != 2) fail("line %d: mov takes two registers", lineno);
            need_reg(arg[0], &rd);
            need_reg(arg[1], &ra);
            emit(enc(op_alu, rd, ra, ra, fn_or)); // rd = ra | ra

        } else if (strcmp(mn, "li") == 0 || strcmp(mn, "lui") == 0) {
            if (nargs != 2) fail("line %d: %s takes a register and an immediate", lineno, mn);
            need_reg(arg[0], &rd);
            need_imm(arg[1], &imm);
            emit(enc_imm8(strcmp(mn, "li") == 0 ? op_li : op_lui, rd, imm & 0xff));

        } else if (strcmp(mn, "set") == 0) {
            if (nargs != 2) fail("line %d: set takes a register and a value", lineno);
            need_reg(arg[0], &rd);
            need_imm(arg[1], &imm);
            emit(enc_imm8(op_li, rd, imm & 0xff));         // low byte
            emit(enc_imm8(op_lui, rd, (imm >> 8) & 0xff)); // high byte

        } else if (strcmp(mn, "addi") == 0 || strcmp(mn, "ld") == 0 || strcmp(mn, "st") == 0) {
            if (nargs != 3) fail("line %d: %s takes rd, ra and an immediate", lineno, mn);
            need_reg(arg[0], &rd);
            need_reg(arg[1], &ra);
            need_imm(arg[2], &imm);
            if (imm < -32 || imm > 31) fail("line %d: immediate %d out of range [-32, 31]", lineno, imm);
            int op = strcmp(mn, "addi") == 0 ? op_addi : strcmp(mn, "ld") == 0 ? op_ld : op_st;
            emit(enc_imm6(op, rd, ra, imm));

        } else if (strcmp(mn, "beq") == 0 || strcmp(mn, "bne") == 0 || strcmp(mn, "blt") == 0) {
            if (nargs != 3) fail("line %d: %s takes two registers and a label", lineno, mn);
            need_reg(arg[0], &rd);
            need_reg(arg[1], &ra);
            need_imm(arg[2], &imm);
            int off = imm - here; // branches are relative to the branch itself
            if (off < -32 || off > 31) fail("line %d: branch target too far (offset %d)", lineno, off);
            int op = strcmp(mn, "beq") == 0 ? op_beq : strcmp(mn, "bne") == 0 ? op_bne : op_blt;
            emit(enc_imm6(op, rd, ra, off));

        } else if (strcmp(mn, "jmp") == 0) {
            if (nargs != 1) fail("line %d: jmp takes one register", lineno);
            need_reg(arg[0], &rd);
            emit(enc(op_jmp, rd, 0, 0, 0));

        } else if (strcmp(mn, "jal") == 0) {
            if (nargs != 2) fail("line %d: jal takes a link register and a target register", lineno);
            need_reg(arg[0], &rd);
            need_reg(arg[1], &ra);
            emit(enc(op_jal, rd, ra, 0, 0));

        } else if (strcmp(mn, "out") == 0 || strcmp(mn, "in") == 0) {
            if (nargs != 1) fail("line %d: %s takes one register", lineno, mn);
            need_reg(arg[0], &rd);
            emit(enc(strcmp(mn, "out") == 0 ? op_out : op_in, rd, 0, 0, 0));

        } else {
            fail("line %d: unknown instruction '%s'", lineno, mn);
        }
    }

    free(buf);
    return addr;
}
