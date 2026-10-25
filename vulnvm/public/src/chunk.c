/* chunk.c — bytecode chunk storage + constant pool. */
#include "chunk.h"
#include "alloc.h"

#include <string.h>

void chunk_init(Chunk *c) {
    memset(c, 0, sizeof(*c));
}

void chunk_free(Chunk *c) {
    /* Release any object constants this chunk owns (strings, nested functions). */
    for (int i = 0; i < c->constants.count; i++) release(c->constants.values[i]);
    if (c->constants.values) vm_free(c->constants.values);
    if (c->code)  vm_free(c->code);
    if (c->lines) vm_free(c->lines);
    memset(c, 0, sizeof(*c));
}

void chunk_write(Chunk *c, uint8_t byte, int line) {
    if (c->count + 1 > c->capacity) {
        int nc = c->capacity < 16 ? 16 : c->capacity * 2;
        c->code  = (uint8_t *)vm_realloc(c->code, (size_t)nc);
        c->lines = (int *)vm_realloc(c->lines, (size_t)nc * sizeof(int));
        c->capacity = nc;
    }
    c->code[c->count] = byte;
    c->lines[c->count] = line;
    c->count++;
}

void chunk_write16(Chunk *c, uint16_t v, int line) {
    chunk_write(c, (uint8_t)(v >> 8), line);
    chunk_write(c, (uint8_t)(v & 0xFF), line);
}

uint16_t chunk_read16(const Chunk *c, int offset) {
    return (uint16_t)((c->code[offset] << 8) | c->code[offset + 1]);
}

/* Per-opcode (length, net stack effect). For the BUILD/CALL/INVOKE/NEW opcodes
 * the effect depends on the operand and is computed inline. */
int chunk_max_stack(const Chunk *c) {
    int depth = 0, maxd = 0;
    for (int off = 0; off < c->count;) {
        OpCode op = (OpCode)c->code[off];
        int len = 1, eff = 0;
        switch (op) {
        case OP_CONSTANT:     len = 3; eff = +1; break;
        case OP_NULL: case OP_TRUE: case OP_FALSE: len = 1; eff = +1; break;
        case OP_POP:          len = 1; eff = -1; break;
        case OP_DUP:          len = 1; eff = +1; break;
        case OP_DUP2:         len = 1; eff = +2; break;
        case OP_GET_LOCAL: case OP_GET_GLOBAL: case OP_GET_UPVALUE:
                              len = 3; eff = +1; break;
        case OP_SET_LOCAL: case OP_SET_GLOBAL: case OP_SET_UPVALUE:
                              len = 3; eff = 0; break;
        case OP_DEFINE_GLOBAL: len = 3; eff = -1; break;
        case OP_GET_INDEX:    len = 1; eff = -1; break;
        case OP_SET_INDEX:    len = 1; eff = -2; break;
        case OP_GET_PROPERTY: len = 3; eff = 0; break;
        case OP_SET_PROPERTY: len = 3; eff = -1; break;
        case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: case OP_MOD:
        case OP_BAND: case OP_BOR: case OP_BXOR: case OP_SHL: case OP_SHR:
        case OP_EQ: case OP_NEQ: case OP_LT: case OP_GT: case OP_LE: case OP_GE:
                              len = 1; eff = -1; break;
        case OP_NEG: case OP_BNOT: case OP_NOT: len = 1; eff = 0; break;
        case OP_BUILD_ARRAY: case OP_BUILD_BYTEARRAY:
                              len = 3; eff = 1 - (int)chunk_read16(c, off + 1); break;
        case OP_BUILD_DICT:   len = 3; eff = 1 - 2 * (int)chunk_read16(c, off + 1); break;
        case OP_JUMP: case OP_LOOP: len = 3; eff = 0; break;
        case OP_JUMP_IF_FALSE: case OP_JUMP_IF_TRUE: len = 3; eff = -1; break;
        case OP_CALL: case OP_NEW: len = 2; eff = -(int)c->code[off + 1]; break;
        case OP_INVOKE: case OP_INVOKE_SUPER: len = 4; eff = -(int)c->code[off + 3]; break;
        case OP_NEW_ARRAY:    len = 1; eff = 0; break;
        case OP_RETURN:       len = 1; eff = -1; break;
        case OP_CLOSURE:      len = 3; eff = +1; break;
        case OP_CLASS:        len = 3; eff = +1; break;
        case OP_MODULE:       len = 3; eff = +1; break;
        case OP_INHERIT:      len = 1; eff = -1; break;
        case OP_METHOD:       len = 3; eff = -1; break;
        case OP_MEMBER:       len = 3; eff = -1; break;
        case OP_INSTANCEOF:   len = 1; eff = -1; break;
        case OP_HAS:          len = 3; eff = 0; break;
        case OP_THROW:        len = 1; eff = -1; break;
        case OP_TRY_PUSH:     len = 3; eff = 0; break;
        case OP_TRY_POP:      len = 1; eff = 0; break;
        default:              len = 1; eff = 0; break;
        }
        depth += eff;
        if (depth > maxd) maxd = depth;
        if (depth < 0) depth = 0; /* defensive; balanced code stays >= 0 */
        off += len;
    }
    return maxd;
}

int chunk_add_const(Chunk *c, Value v) {
    for (int i = 0; i < c->constants.count; i++)
        if (value_equals(c->constants.values[i], v)) return i;

    ValueArray *a = &c->constants;
    if (a->count + 1 > a->capacity) {
        int nc = a->capacity < 8 ? 8 : a->capacity * 2;
        a->values = (Value *)vm_realloc(a->values, (size_t)nc * sizeof(Value));
        a->capacity = nc;
    }
    a->values[a->count] = retain(v); /* chunk owns a reference to object constants */
    return a->count++;
}

int chunk_add_const_raw(Chunk *c, Value v) {
    ValueArray *a = &c->constants;
    if (a->count + 1 > a->capacity) {
        int nc = a->capacity < 8 ? 8 : a->capacity * 2;
        a->values = (Value *)vm_realloc(a->values, (size_t)nc * sizeof(Value));
        a->capacity = nc;
    }
    a->values[a->count] = retain(v);
    return a->count++;
}
