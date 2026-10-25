/* disasm.c — bytecode disassembler: human-readable dump (--bytecode) and a
 * compact single-line op stream used by tests. */
#include "chunk.h"
#include "value.h"

#include <string.h>

static const char *const kOpNames[OP__COUNT] = {
#define X(name) #name,
    OPCODE_LIST(X)
#undef X
};

const char *opcode_name(OpCode op) {
    if ((unsigned)op >= OP__COUNT) return "?OP";
    return kOpNames[op];
}

typedef enum {
    OPND_NONE, OPND_CONST, OPND_U16, OPND_JUMP, OPND_LOOP, OPND_ARGC, OPND_INVOKE
} OpndKind;

static OpndKind operand_kind(OpCode op) {
    switch (op) {
    case OP_CONSTANT: case OP_GET_GLOBAL: case OP_SET_GLOBAL: case OP_DEFINE_GLOBAL:
    case OP_GET_PROPERTY: case OP_SET_PROPERTY: case OP_HAS: case OP_CLOSURE:
    case OP_CLASS: case OP_METHOD: case OP_MEMBER: case OP_MODULE:
        return OPND_CONST;
    case OP_GET_LOCAL: case OP_SET_LOCAL: case OP_GET_UPVALUE: case OP_SET_UPVALUE:
    case OP_BUILD_ARRAY: case OP_BUILD_BYTEARRAY: case OP_BUILD_DICT:
        return OPND_U16;
    case OP_JUMP: case OP_JUMP_IF_FALSE: case OP_JUMP_IF_TRUE: case OP_TRY_PUSH:
        return OPND_JUMP;
    case OP_LOOP:
        return OPND_LOOP;
    case OP_CALL: case OP_NEW:
        return OPND_ARGC;
    case OP_INVOKE: case OP_INVOKE_SUPER:
        return OPND_INVOKE;
    default:
        return OPND_NONE;
    }
}

static int instr_length(OpCode op) {
    switch (operand_kind(op)) {
    case OPND_NONE:   return 1;
    case OPND_ARGC:   return 2;
    case OPND_CONST:
    case OPND_U16:
    case OPND_JUMP:
    case OPND_LOOP:   return 3;
    case OPND_INVOKE: return 4;
    }
    return 1;
}

/* --------------------------------------------------------- pretty dump ----- */

/* Render a constant value, guarding against an out-of-range index (robust to
 * corrupt/crafted bytecode, not just compiler output). */
static void safe_const_repr(const Chunk *c, uint16_t idx, char *buf, size_t cap) {
    if (idx < c->constants.count) value_repr(c->constants.values[idx], buf, cap);
    else snprintf(buf, cap, "?const#%u", idx);
}

int chunk_disasm_instruction(const Chunk *c, int offset, FILE *out) {
    fprintf(out, "%04d ", offset);
    if (offset > 0 && c->lines[offset] == c->lines[offset - 1]) fprintf(out, "   | ");
    else fprintf(out, "%4d ", c->lines[offset]);

    OpCode op = (OpCode)c->code[offset];
    const char *name = opcode_name(op);
    char buf[256];
    /* Refuse to read operand bytes past the end of the code (truncated tail). */
    if (offset + instr_length(op) > c->count) {
        fprintf(out, "%s <truncated>\n", name);
        return c->count;
    }
    switch (operand_kind(op)) {
    case OPND_NONE:
        fprintf(out, "%s\n", name);
        break;
    case OPND_CONST: {
        uint16_t idx = chunk_read16(c, offset + 1);
        safe_const_repr(c, idx, buf, sizeof(buf));
        fprintf(out, "%-16s %4d '%s'\n", name, idx, buf);
        break;
    }
    case OPND_U16:
        fprintf(out, "%-16s %4d\n", name, chunk_read16(c, offset + 1));
        break;
    case OPND_JUMP: {
        uint16_t d = chunk_read16(c, offset + 1);
        fprintf(out, "%-16s %4d -> %d\n", name, d, offset + 3 + d);
        break;
    }
    case OPND_LOOP: {
        uint16_t d = chunk_read16(c, offset + 1);
        fprintf(out, "%-16s %4d -> %d\n", name, d, offset + 3 - d);
        break;
    }
    case OPND_ARGC:
        fprintf(out, "%-16s %4d\n", name, c->code[offset + 1]);
        break;
    case OPND_INVOKE: {
        uint16_t idx = chunk_read16(c, offset + 1);
        uint8_t argc = c->code[offset + 3];
        safe_const_repr(c, idx, buf, sizeof(buf));
        fprintf(out, "%-16s %4d '%s' (%d args)\n", name, idx, buf, argc);
        break;
    }
    }
    return offset + instr_length(op);
}

void chunk_disassemble(const Chunk *c, const char *name, FILE *out) {
    fprintf(out, "== %s ==\n", name ? name : "<chunk>");
    for (int off = 0; off < c->count;)
        off = chunk_disasm_instruction(c, off, out);
}

/* --------------------------------------------------------- compact form ---- */

size_t chunk_ops_compact(const Chunk *c, char *buf, size_t cap) {
    size_t len = 0;
    char tmp[256];
    #define APPEND(...) do { \
        int _n = snprintf(tmp, sizeof(tmp), __VA_ARGS__); \
        for (int _i = 0; _i < _n && len + 1 < cap; _i++) buf[len++] = tmp[_i]; \
    } while (0)

    for (int off = 0; off < c->count;) {
        OpCode op = (OpCode)c->code[off];
        if (len && len + 1 < cap) buf[len++] = ' ';
        const char *name = opcode_name(op);
        char vr[200];
        if (off + instr_length(op) > c->count) { APPEND("%s<truncated>", name); break; }
        switch (operand_kind(op)) {
        case OPND_NONE:  APPEND("%s", name); break;
        case OPND_CONST: {
            uint16_t idx = chunk_read16(c, off + 1);
            safe_const_repr(c, idx, vr, sizeof(vr));
            APPEND("%s(%s)", name, vr);
            break;
        }
        case OPND_U16:  APPEND("%s(%d)", name, chunk_read16(c, off + 1)); break;
        case OPND_JUMP: APPEND("%s(->%d)", name, off + 3 + chunk_read16(c, off + 1)); break;
        case OPND_LOOP: APPEND("%s(->%d)", name, off + 3 - chunk_read16(c, off + 1)); break;
        case OPND_ARGC: APPEND("%s(%d)", name, c->code[off + 1]); break;
        case OPND_INVOKE: {
            uint16_t idx = chunk_read16(c, off + 1);
            safe_const_repr(c, idx, vr, sizeof(vr));
            APPEND("%s(%s,%d)", name, vr, c->code[off + 3]);
            break;
        }
        }
        off += instr_length(op);
    }
    if (cap) buf[len] = '\0';
    #undef APPEND
    return len;
}
