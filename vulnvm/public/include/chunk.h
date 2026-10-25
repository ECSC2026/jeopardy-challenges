/* chunk.h — a bytecode chunk: code bytes, a constant pool, and per-byte line
 * info. Object constants (strings) are retained by the chunk and released when
 * the chunk is freed, so constant lifetimes follow the compiled function. */
#ifndef VULNVM_CHUNK_H
#define VULNVM_CHUNK_H

#include "value.h"
#include "opcodes.h"
#include <stdio.h>

typedef struct {
    int    count;
    int    capacity;
    Value *values;
} ValueArray;

typedef struct {
    uint8_t   *code;
    int       *lines;    /* source line per code byte (for diagnostics) */
    int        count;
    int        capacity;
    ValueArray constants;
} Chunk;

void chunk_init(Chunk *c);
void chunk_free(Chunk *c);                 /* releases object constants too */

void chunk_write(Chunk *c, uint8_t byte, int line);
void chunk_write16(Chunk *c, uint16_t v, int line); /* big-endian */

/* Add a constant, de-duplicating equal values. Retains object constants that are
 * newly appended; the caller keeps ownership of its own reference (release it
 * afterwards). Returns the constant index. */
int chunk_add_const(Chunk *c, Value v);

/* Like chunk_add_const but WITHOUT de-duplication: always appends, so that
 * constant indices are preserved exactly (used when loading a serialized
 * program, whose bytecode already refers to constants by their original index). */
int chunk_add_const_raw(Chunk *c, Value v);

/* Read a big-endian u16 from code at offset. */
uint16_t chunk_read16(const Chunk *c, int offset);

/* Conservative maximum value-stack depth this chunk's code reaches, computed by a
 * linear net-stack-effect walk (a safe upper bound). Used to size call frames so
 * the VM can reserve stack space and never overflow. */
int chunk_max_stack(const Chunk *c);

/* Disassembly (for the --bytecode flag and tests). */
void chunk_disassemble(const Chunk *c, const char *name, FILE *out);
int  chunk_disasm_instruction(const Chunk *c, int offset, FILE *out);

/* Compact single-line op stream for tests: "CONSTANT(1) ADD POP RETURN ...".
 * Constants render their value; jumps render the absolute target offset. */
size_t chunk_ops_compact(const Chunk *c, char *buf, size_t cap);

#endif /* VULNVM_CHUNK_H */
