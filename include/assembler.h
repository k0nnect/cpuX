#ifndef cpux_assembler_h
#define cpux_assembler_h

#include "bit.h"

// assemble cpux source text into machine words.
//   source  the program text
//   out     where the assembled words are written
//   cap     capacity of out, in words
//   err     buffer for a lowercase error message on failure
//   errcap  capacity of err, in bytes
// returns the number of words assembled, or -1 on error (with err filled in).
int assemble(const char *source, word *out, int cap, char *err, int errcap);

#endif
