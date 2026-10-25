/* serialize.c — .vmb program (de)serialization. See serialize.h.
 *
 * Format (all integers little-endian):
 *   header : magic "VVMB", u8 version, u8 reserved[3]
 *   then one Function (the top-level script).
 *
 *   Function:
 *     u8  arity
 *     u16 upvalue_count
 *     u32 frame_size
 *     u8  has_name ; if 1: u32 len, bytes            (function name)
 *     Chunk
 *   Chunk:
 *     u32 code_count ; bytes[code_count]
 *     u32 line_runs  ; { i32 line, u32 run } * line_runs   (RLE per code byte)
 *     u32 const_count ; Constant * const_count
 *   Constant:
 *     u8 tag, then:
 *       0 NULL            (nothing)
 *       1 BOOL   u8
 *       2 NUMBER i32
 *       3 LONG   i64
 *       4 FLOAT  u32 (bit pattern)
 *       5 DOUBLE u64 (bit pattern)
 *       6 CHAR   u32
 *       7 SYMBOL u32 len, bytes           (re-interned on load)
 *       8 STRING u32 len, bytes           (string_new on load)
 *       9 FUNCTION  Function              (recursive)
 */
#include "serialize.h"
#include "chunk.h"
#include "value.h"
#include "alloc.h"

#include <string.h>

enum {
    C_NULL = 0, C_BOOL, C_NUMBER, C_LONG, C_FLOAT, C_DOUBLE,
    C_CHAR, C_SYMBOL, C_STRING, C_FUNCTION
};

/* ============================================================ writing ====== */

static void wr8(FILE *f, uint8_t v)  { fputc(v, f); }
static void wr16(FILE *f, uint16_t v){ wr8(f, v & 0xFF); wr8(f, (v >> 8) & 0xFF); }
static void wr32(FILE *f, uint32_t v){ for (int i = 0; i < 4; i++) wr8(f, (v >> (8 * i)) & 0xFF); }
static void wr64(FILE *f, uint64_t v){ for (int i = 0; i < 8; i++) wr8(f, (v >> (8 * i)) & 0xFF); }

static void wr_str(FILE *f, const char *bytes, uint32_t len) {
    wr32(f, len);
    if (len) fwrite(bytes, 1, len, f);
}

static int write_function(FILE *f, const ObjFunction *fn); /* fwd */

static int write_constant(FILE *f, Value v) {
    switch (v.type) {
    case VAL_NULL:   wr8(f, C_NULL);   break;
    case VAL_BOOL:   wr8(f, C_BOOL);   wr8(f, v.as.boolean ? 1 : 0); break;
    case VAL_NUMBER: wr8(f, C_NUMBER); wr32(f, (uint32_t)v.as.num);  break;
    case VAL_LONG:   wr8(f, C_LONG);   wr64(f, (uint64_t)v.as.lng);  break;
    case VAL_FLOAT: {
        uint32_t bits; memcpy(&bits, &v.as.f32, 4);
        wr8(f, C_FLOAT); wr32(f, bits); break;
    }
    case VAL_DOUBLE: {
        uint64_t bits; memcpy(&bits, &v.as.f64, 8);
        wr8(f, C_DOUBLE); wr64(f, bits); break;
    }
    case VAL_CHAR:   wr8(f, C_CHAR);   wr32(f, v.as.ch);  break;
    case VAL_SYMBOL: {
        const char *nm = symbol_name(v.as.sym);
        if (!nm) return -1;
        wr8(f, C_SYMBOL); wr_str(f, nm, (uint32_t)strlen(nm)); break;
    }
    case VAL_OBJ: {
        Obj *o = v.as.obj;
        if (obj_type(o) == OBJ_STRING) {
            ObjString *s = (ObjString *)o;
            wr8(f, C_STRING); wr_str(f, s->data, s->length);
        } else if (obj_type(o) == OBJ_FUNCTION) {
            ObjFunction *fn = (ObjFunction *)o;
            if (fn->kind != FN_VM) return -1; /* never serialize natives */
            wr8(f, C_FUNCTION);
            return write_function(f, fn);
        } else {
            return -1; /* no other object kinds ever reach the constant pool */
        }
        break;
    }
    default: return -1;
    }
    return 0;
}

static int write_function(FILE *f, const ObjFunction *fn) {
    if (fn->kind != FN_VM || !fn->chunk) return -1;
    const Chunk *c = fn->chunk;

    wr8(f, fn->arity);
    wr16(f, fn->upvalue_count);
    wr32(f, fn->frame_size);
    if (fn->name) { wr8(f, 1); wr_str(f, fn->name->data, fn->name->length); }
    else            wr8(f, 0);

    /* code */
    wr32(f, (uint32_t)c->count);
    if (c->count) fwrite(c->code, 1, (size_t)c->count, f);

    /* lines, RLE-encoded */
    long runs_pos = ftell(f);          /* backpatch the run count after counting */
    wr32(f, 0);
    uint32_t nruns = 0;
    for (int i = 0; i < c->count;) {
        int line = c->lines[i], j = i + 1;
        while (j < c->count && c->lines[j] == line) j++;
        wr32(f, (uint32_t)line);
        wr32(f, (uint32_t)(j - i));
        nruns++;
        i = j;
    }
    long end = ftell(f);
    fseek(f, runs_pos, SEEK_SET); wr32(f, nruns); fseek(f, end, SEEK_SET);

    /* constants */
    wr32(f, (uint32_t)c->constants.count);
    for (int i = 0; i < c->constants.count; i++)
        if (write_constant(f, c->constants.values[i]) != 0) return -1;
    return 0;
}

int program_write(FILE *out, const ObjFunction *script) {
    wr8(out, VMB_MAGIC0); wr8(out, VMB_MAGIC1);
    wr8(out, VMB_MAGIC2); wr8(out, VMB_MAGIC3);
    wr8(out, VMB_VERSION); wr8(out, 0); wr8(out, 0); wr8(out, 0);
    return write_function(out, script);
}

/* ============================================================ reading ====== */

typedef struct { const uint8_t *p; size_t len, pos; int err; } Rd;

static uint8_t  rd8(Rd *r)  { if (r->pos + 1 > r->len) { r->err = 1; return 0; } return r->p[r->pos++]; }
static uint16_t rd16(Rd *r) { uint16_t a = rd8(r), b = rd8(r); return (uint16_t)(a | (b << 8)); }
static uint32_t rd32(Rd *r) { uint32_t v = 0; for (int i = 0; i < 4; i++) v |= (uint32_t)rd8(r) << (8 * i); return v; }
static uint64_t rd64(Rd *r) { uint64_t v = 0; for (int i = 0; i < 8; i++) v |= (uint64_t)rd8(r) << (8 * i); return v; }

/* Read a length-prefixed byte string into caller-provided storage. Returns a
 * pointer into the input buffer (not NUL-terminated) and the length. */
static const char *rd_str(Rd *r, uint32_t *out_len) {
    uint32_t len = rd32(r);
    if (r->err || r->pos + len > r->len) { r->err = 1; *out_len = 0; return NULL; }
    const char *s = (const char *)(r->p + r->pos);
    r->pos += len;
    *out_len = len;
    return s;
}

static ObjFunction *read_function(Rd *r); /* fwd */

/* Read one constant and append it (order-preserving) to chunk `c`. */
static int read_constant(Rd *r, Chunk *c) {
    uint8_t tag = rd8(r);
    Value v;
    switch (tag) {
    case C_NULL:   v = mk_null(); break;
    case C_BOOL:   v = mk_bool(rd8(r) != 0); break;
    case C_NUMBER: v = mk_number((int32_t)rd32(r)); break;
    case C_LONG:   v = mk_long((int64_t)rd64(r)); break;
    case C_FLOAT:  { uint32_t b = rd32(r); float f;  memcpy(&f, &b, 4); v = mk_float(f); break; }
    case C_DOUBLE: { uint64_t b = rd64(r); double d; memcpy(&d, &b, 8); v = mk_double(d); break; }
    case C_CHAR:   v = mk_char(rd32(r)); break;
    case C_SYMBOL: {
        uint32_t len; const char *nm = rd_str(r, &len);
        if (r->err) return -1;
        v = mk_symbol(symbol_intern(nm, len));
        break;
    }
    case C_STRING: {
        uint32_t len; const char *bytes = rd_str(r, &len);
        if (r->err) return -1;
        ObjString *s = string_new(bytes, len);
        if (!s) return -1;
        chunk_add_const_raw(c, mk_obj((Obj *)s));
        release(mk_obj((Obj *)s));   /* chunk retained its own reference */
        return 0;
    }
    case C_FUNCTION: {
        ObjFunction *fn = read_function(r);
        if (!fn) return -1;
        chunk_add_const_raw(c, mk_obj((Obj *)fn));
        release(mk_obj((Obj *)fn));
        return 0;
    }
    default: return -1;
    }
    if (r->err) return -1;
    chunk_add_const_raw(c, v);  /* immediates: no ownership to transfer */
    return 0;
}

static ObjFunction *read_function(Rd *r) {
    uint8_t  arity = rd8(r);
    uint16_t upvals = rd16(r);
    uint32_t frame_size = rd32(r);

    ObjString *name = NULL;
    if (rd8(r) == 1) {
        uint32_t len; const char *nm = rd_str(r, &len);
        if (r->err) return NULL;
        name = string_new(nm, len);
        if (!name) return NULL;
    }
    if (r->err) { if (name) release(mk_obj((Obj *)name)); return NULL; }

    ObjFunction *fn = function_new(name, arity);
    if (name) release(mk_obj((Obj *)name));   /* function_new retained it */
    if (!fn) return NULL;
    fn->upvalue_count = upvals;
    fn->frame_size = frame_size;
    Chunk *c = fn->chunk;

    /* code */
    uint32_t code_count = rd32(r);
    if (r->err || r->pos + code_count > r->len) { release(mk_obj((Obj *)fn)); return NULL; }
    const uint8_t *code = r->p + r->pos;
    r->pos += code_count;

    /* lines (RLE) -> expand alongside the code bytes */
    uint32_t nruns = rd32(r);
    int *lines = NULL;
    if (code_count) {
        lines = (int *)vm_alloc((size_t)code_count * sizeof(int));
        if (!lines) { release(mk_obj((Obj *)fn)); return NULL; }
    }
    uint32_t filled = 0;
    for (uint32_t i = 0; i < nruns; i++) {
        int32_t  line = (int32_t)rd32(r);
        uint32_t run  = rd32(r);
        if (r->err) { if (lines) vm_free(lines); release(mk_obj((Obj *)fn)); return NULL; }
        for (uint32_t k = 0; k < run && filled < code_count; k++) lines[filled++] = line;
    }
    /* any bytes not covered by a run default to line 0 */
    for (uint32_t i = 0; i < code_count; i++)
        chunk_write(c, code[i], lines ? lines[i] : 0);
    if (lines) vm_free(lines);

    /* constants */
    uint32_t const_count = rd32(r);
    if (r->err) { release(mk_obj((Obj *)fn)); return NULL; }
    for (uint32_t i = 0; i < const_count; i++) {
        if (read_constant(r, c) != 0 || r->err) { release(mk_obj((Obj *)fn)); return NULL; }
    }
    return fn;
}

ObjFunction *program_read(const uint8_t *buf, size_t len, char *err, size_t errcap) {
    Rd r = { buf, len, 0, 0 };
    if (len < 8 || rd8(&r) != VMB_MAGIC0 || rd8(&r) != VMB_MAGIC1 ||
        rd8(&r) != VMB_MAGIC2 || rd8(&r) != VMB_MAGIC3) {
        if (err) snprintf(err, errcap, "not a .vmb program (bad magic)");
        return NULL;
    }
    uint8_t version = rd8(&r); rd8(&r); rd8(&r); rd8(&r);
    if (version != VMB_VERSION) {
        if (err) snprintf(err, errcap, "unsupported .vmb version %u (expected %u)",
                          version, VMB_VERSION);
        return NULL;
    }
    ObjFunction *fn = read_function(&r);
    if (!fn) { if (err) snprintf(err, errcap, "malformed or truncated .vmb program"); return NULL; }
    return fn;
}
