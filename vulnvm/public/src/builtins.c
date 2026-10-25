/* builtins.c — native methods for Toybox.Lang types + OP_INVOKE dispatch.
 *
 * Each method has signature (vm, recv, name, argc, args, *result) -> bool: on
 * success it stores an OWNED value in *result and returns true; on failure it
 * calls vm_raise (message + line) and returns false. The invoking OP_INVOKE
 * handler releases receiver+args on both paths.
 */
#include "builtins.h"
#include "alloc.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* --------------------------------------------------- small numeric helpers - */

static bool bi_is_numeric(Value v) {
    return v.type == VAL_NUMBER || v.type == VAL_LONG ||
           v.type == VAL_FLOAT || v.type == VAL_DOUBLE;
}
static double bi_as_f64(Value v) {
    switch (v.type) {
    case VAL_NUMBER: return v.as.num;
    case VAL_LONG:   return (double)v.as.lng;
    case VAL_FLOAT:  return v.as.f32;
    case VAL_DOUBLE: return v.as.f64;
    default:         return 0;
    }
}
static int64_t bi_as_i64(Value v) {
    switch (v.type) {
    case VAL_NUMBER: return v.as.num;
    case VAL_LONG:   return v.as.lng;
    case VAL_FLOAT:  return (int64_t)v.as.f32;
    case VAL_DOUBLE: return (int64_t)v.as.f64;
    case VAL_CHAR:   return v.as.ch;
    default:         return 0;
    }
}

static const char *recv_type_name(Value v) {
    switch (v.type) {
    case VAL_NUMBER: return "Number";
    case VAL_LONG:   return "Long";
    case VAL_FLOAT:  return "Float";
    case VAL_DOUBLE: return "Double";
    case VAL_BOOL:   return "Boolean";
    case VAL_CHAR:   return "Char";
    case VAL_SYMBOL: return "Symbol";
    case VAL_NULL:   return "Null";
    case VAL_OBJ:    return obj_type_name(obj_type(v.as.obj));
    }
    return "?";
}

/* ------------------------------------------------------ string builder ----- */

typedef struct { char *buf; size_t len, cap; } SB;
static void sb_reserve(SB *s, size_t extra) {
    if (s->len + extra + 1 <= s->cap) return;
    size_t nc = s->cap < 32 ? 32 : s->cap;
    while (s->len + extra + 1 > nc) nc *= 2;
    s->buf = (char *)vm_realloc(s->buf, nc);
    s->cap = nc;
}
static void sb_write(SB *s, const char *p, size_t n) { sb_reserve(s, n); memcpy(s->buf + s->len, p, n); s->len += n; }
static void sb_cstr(SB *s, const char *p) { sb_write(s, p, strlen(p)); }

/* ------------------------------------------------------ value_to_string ---- */

static void append_display(SB *s, Value v, int depth) {
    if (depth > 32) { sb_cstr(s, "..."); return; } /* cycle / deep-nest guard */
    if (IS_STRING(v)) { sb_write(s, AS_STRING(v)->data, AS_STRING(v)->length); return; }
    if (IS_ARRAY(v)) {
        ObjArray *a = AS_ARRAY(v);
        sb_cstr(s, "[");
        for (uint32_t i = 0; i < a->count; i++) {
            if (i) sb_cstr(s, ", ");
            append_display(s, a->items[i], depth + 1);
        }
        sb_cstr(s, "]");
        return;
    }
    if (IS_BYTEARRAY(v)) {
        ObjByteArray *b = AS_BYTEARRAY(v);
        char n[16];
        sb_cstr(s, "[");
        for (uint32_t i = 0; i < b->count; i++) {
            if (i) sb_cstr(s, ", ");
            snprintf(n, sizeof(n), "%u", b->bytes[i]);
            sb_cstr(s, n);
        }
        sb_cstr(s, "]b");
        return;
    }
    if (IS_DICT(v)) {
        ObjDict *d = AS_DICT(v);
        sb_cstr(s, "{");
        bool first = true;
        for (uint32_t i = 0; i < d->capacity; i++) {
            if (d->entries[i].state != DICT_USED) continue;
            if (!first) sb_cstr(s, ", ");
            first = false;
            append_display(s, d->entries[i].key, depth + 1);
            sb_cstr(s, "=>");
            append_display(s, d->entries[i].val, depth + 1);
        }
        sb_cstr(s, "}");
        return;
    }
    char tmp[256];
    value_repr(v, tmp, sizeof(tmp));
    sb_cstr(s, tmp);
}

ObjString *value_to_string(Value v) {
    if (IS_STRING(v)) { obj_incref(AS_OBJ(v)); return AS_STRING(v); }
    SB s = { NULL, 0, 0 };
    append_display(&s, v, 0);
    ObjString *r = string_new(s.len ? s.buf : "", s.len);
    if (s.buf) vm_free(s.buf);
    return r;
}

/* ------------------------------------------------------------- helpers ----- */

#define EQ(str) (strcmp(name, (str)) == 0)
#define NEED_ARGC(n) do { if (argc != (n)) \
    return vm_raise(vm, "%s.%s expects %d argument(s), got %d", \
                    recv_type_name(recv), name, (n), argc); } while (0)

/* Element equality for indexOf/remove: strict value equality. */
static bool elem_eq(Value a, Value b) {
    if (bi_is_numeric(a) && bi_is_numeric(b)) return bi_as_f64(a) == bi_as_f64(b) && a.type == b.type;
    return value_equals(a, b);
}

/* ------------------------------------------------------- object base ------- */

static bool object_base_method(VM *vm, Value recv, const char *name,
                               int argc, Value *args, Value *result) {
    if (EQ("toString")) { NEED_ARGC(0); *result = mk_obj((Obj *)value_to_string(recv)); return true; }
    if (EQ("hashCode")) { NEED_ARGC(0); *result = mk_number((int32_t)value_hash(recv)); return true; }
    if (EQ("equals"))   { NEED_ARGC(1); *result = mk_bool(value_equals(recv, args[0])); return true; }
    if (EQ("method")) {
        NEED_ARGC(1);
        if (!IS_SYMBOL(args[0])) return vm_raise(vm, "method() expects a Symbol");
        ObjMethod *m = method_new(recv, args[0].as.sym);
        if (!m) return vm_raise(vm, "out of memory");
        *result = mk_obj((Obj *)m);
        return true;
    }
    if (EQ("weak")) {
        NEED_ARGC(0);
        if (!IS_OBJ(recv)) { *result = retain(recv); return true; } /* immutables return self */
        ObjWeakRef *w = weakref_new(obj_id(AS_OBJ(recv)));
        if (!w) return vm_raise(vm, "out of memory");
        *result = mk_obj((Obj *)w);
        return true;
    }
    return vm_raise(vm, "undefined method '%s' on %s", name, recv_type_name(recv));
}

/* Safely apply one numeric value to a printf-style format. Only numeric/char
 * conversions are allowed; %n/%s/%p and '*' width are rejected so a
 * user-controlled format string cannot be weaponised. snprintf bounds every
 * write. Each conversion receives the same value. */
static bool safe_format(VM *vm, Value num, const char *fmt, ObjString **out) {
    SB s = { NULL, 0, 0 };
    for (const char *p = fmt; *p;) {
        if (*p != '%') { sb_write(&s, p, 1); p++; continue; }
        p++;
        if (*p == '%') { sb_write(&s, "%", 1); p++; continue; }
        char spec[40]; int si = 0; spec[si++] = '%';
        while (*p && strchr("-+ 0#", *p)) { if (si < 30) spec[si++] = *p; p++; }
        while (*p >= '0' && *p <= '9')    { if (si < 30) spec[si++] = *p; p++; }
        if (*p == '.') { if (si < 30) spec[si++] = *p; p++;
                         while (*p >= '0' && *p <= '9') { if (si < 30) spec[si++] = *p; p++; } }
        while (*p && strchr("lhLqjzt", *p)) p++; /* drop caller length modifiers */
        char conv = *p; if (conv) p++;
        char outbuf[512];
        switch (conv) {
        case 'd': case 'i':
            spec[si++]='l'; spec[si++]='l'; spec[si++]='d'; spec[si]=0;
            snprintf(outbuf, sizeof(outbuf), spec, (long long)bi_as_i64(num)); break;
        case 'u': case 'o': case 'x': case 'X':
            spec[si++]='l'; spec[si++]='l'; spec[si++]=conv; spec[si]=0;
            snprintf(outbuf, sizeof(outbuf), spec, (unsigned long long)bi_as_i64(num)); break;
        case 'f': case 'F': case 'e': case 'E': case 'g': case 'G':
            spec[si++]=conv; spec[si]=0;
            snprintf(outbuf, sizeof(outbuf), spec, bi_as_f64(num)); break;
        case 'c':
            spec[si++]='c'; spec[si]=0;
            snprintf(outbuf, sizeof(outbuf), spec, (int)bi_as_i64(num)); break;
        default:
            if (s.buf) vm_free(s.buf);
            return vm_raise(vm, "unsupported/unsafe format conversion '%%%c'", conv ? conv : '?');
        }
        sb_cstr(&s, outbuf);
    }
    *out = string_new(s.len ? s.buf : "", s.len);
    if (s.buf) vm_free(s.buf);
    return true;
}

/* --------------------------------------------------------- Number/etc. ----- */

static bool numeric_method(VM *vm, Value recv, const char *name,
                           int argc, Value *args, Value *result) {
    if (EQ("abs")) {
        NEED_ARGC(0);
        switch (recv.type) {
        case VAL_NUMBER: *result = mk_number(recv.as.num < 0 ? (int32_t)(0u - (uint32_t)recv.as.num) : recv.as.num); break;
        case VAL_LONG:   *result = mk_long(recv.as.lng < 0 ? (int64_t)(0ull - (uint64_t)recv.as.lng) : recv.as.lng); break;
        case VAL_FLOAT:  *result = mk_float(recv.as.f32 < 0 ? -recv.as.f32 : recv.as.f32); break;
        default:         *result = mk_double(recv.as.f64 < 0 ? -recv.as.f64 : recv.as.f64); break;
        }
        return true;
    }
    if (EQ("toNumber")) { NEED_ARGC(0); *result = mk_number((int32_t)bi_as_i64(recv)); return true; }
    if (EQ("toLong"))   { NEED_ARGC(0); *result = mk_long(bi_as_i64(recv)); return true; }
    if (EQ("toFloat"))  { NEED_ARGC(0); *result = mk_float((float)bi_as_f64(recv)); return true; }
    if (EQ("toDouble")) { NEED_ARGC(0); *result = mk_double(bi_as_f64(recv)); return true; }
    if (EQ("toChar")) {
        NEED_ARGC(0);
        if (recv.type != VAL_NUMBER && recv.type != VAL_LONG)
            return vm_raise(vm, "toChar is only defined on integers");
        *result = mk_char((uint32_t)bi_as_i64(recv));
        return true;
    }
    if (EQ("compareTo")) {
        NEED_ARGC(1);
        if (!bi_is_numeric(args[0])) return vm_raise(vm, "compareTo expects a number");
        double x = bi_as_f64(recv), y = bi_as_f64(args[0]);
        *result = mk_number(x < y ? -1 : x > y ? 1 : 0);
        return true;
    }
    if (EQ("format")) {
        NEED_ARGC(1);
        if (!IS_STRING(args[0])) return vm_raise(vm, "format expects a String");
        ObjString *out;
        if (!safe_format(vm, recv, AS_STRING(args[0])->data, &out)) return false;
        *result = mk_obj((Obj *)out);
        return true;
    }
    return object_base_method(vm, recv, name, argc, args, result);
}

/* ---------------------------------------------------------------- Char ----- */

static bool char_method(VM *vm, Value recv, const char *name,
                        int argc, Value *args, Value *result) {
    uint32_t c = AS_CHAR(recv);
    if (EQ("toNumber")) { NEED_ARGC(0); *result = mk_number((int32_t)c); return true; }
    if (EQ("toLower"))  { NEED_ARGC(0); *result = mk_char(c >= 'A' && c <= 'Z' ? c + 32 : c); return true; }
    if (EQ("toUpper"))  { NEED_ARGC(0); *result = mk_char(c >= 'a' && c <= 'z' ? c - 32 : c); return true; }
    if (EQ("compareTo")) {
        NEED_ARGC(1);
        if (!IS_CHAR(args[0])) return vm_raise(vm, "Char.compareTo expects a Char");
        uint32_t o = AS_CHAR(args[0]);
        *result = mk_number(c < o ? -1 : c > o ? 1 : 0);
        return true;
    }
    return object_base_method(vm, recv, name, argc, args, result);
}

/* ----------------------------------------------------------- ByteArray ----- */

/* NUMBER_FORMAT_* codes (registered as globals with these values). */
enum { NF_UINT8 = 0, NF_SINT8 = 1, NF_UINT16 = 2, NF_SINT16 = 3,
       NF_UINT32 = 4, NF_SINT32 = 5, NF_FLOAT = 6, NF_DOUBLE = 7,
       NF_UINT64 = 8, NF_SINT64 = 9 };

static int nf_width(int fmt) {
    switch (fmt) {
    case NF_UINT8: case NF_SINT8: return 1;
    case NF_UINT16: case NF_SINT16: return 2;
    case NF_UINT32: case NF_SINT32: case NF_FLOAT: return 4;
    case NF_UINT64: case NF_SINT64: case NF_DOUBLE: return 8;
    default: return -1;
    }
}

static void read_options(ObjDict *opts, long *offset, int *little) {
    *offset = 0; *little = 0; /* default: offset 0, big-endian */
    if (!opts) return;
    Value v;
    if (dict_get(opts, mk_symbol(symbol_intern_cstr("offset")), &v) && IS_NUMBER(v)) *offset = v.as.num;
    if (dict_get(opts, mk_symbol(symbol_intern_cstr("endianness")), &v) && IS_NUMBER(v)) *little = (v.as.num == 1);
}

static bool bytearray_method(VM *vm, Value recv, const char *name,
                             int argc, Value *args, Value *result) {
    ObjByteArray *b = AS_BYTEARRAY(recv);
    if (EQ("size")) { NEED_ARGC(0); *result = mk_number((int32_t)b->count); return true; }
    if (EQ("add")) {
        NEED_ARGC(1);
        if (!IS_NUMBER(args[0]) && !IS_CHAR(args[0])) return vm_raise(vm, "ByteArray.add expects a byte");
        bytearray_push(b, (uint8_t)bi_as_i64(args[0]));
        *result = retain(recv);
        return true;
    }
    if (EQ("addAll")) {
        NEED_ARGC(1);
        if (IS_BYTEARRAY(args[0])) {
            ObjByteArray *o = AS_BYTEARRAY(args[0]);
            uint32_t n = o->count;
            for (uint32_t i = 0; i < n; i++) bytearray_push(b, o->bytes[i]);
        } else if (IS_ARRAY(args[0])) {
            ObjArray *o = AS_ARRAY(args[0]);
            for (uint32_t i = 0; i < o->count; i++) {
                if (!IS_NUMBER(o->items[i]) && !IS_CHAR(o->items[i]))
                    return vm_raise(vm, "ByteArray.addAll: elements must be bytes");
                bytearray_push(b, (uint8_t)bi_as_i64(o->items[i]));
            }
        } else return vm_raise(vm, "ByteArray.addAll expects an Array or ByteArray");
        *result = retain(recv);
        return true;
    }
    if (EQ("indexOf")) {
        NEED_ARGC(1);
        int32_t idx = -1;
        uint8_t needle = (uint8_t)bi_as_i64(args[0]);
        for (uint32_t i = 0; i < b->count; i++) if (b->bytes[i] == needle) { idx = (int32_t)i; break; }
        *result = mk_number(idx);
        return true;
    }
    if (EQ("reverse")) {
        NEED_ARGC(0);
        ObjByteArray *r = bytearray_new(b->count);
        if (!r) return vm_raise(vm, "out of memory");
        for (uint32_t i = 0; i < b->count; i++) r->bytes[i] = b->bytes[b->count - 1 - i];
        r->count = b->count;
        *result = mk_obj((Obj *)r);
        return true;
    }
    if (EQ("slice")) {
        NEED_ARGC(2);
        long start = IS_NULL(args[0]) ? 0 : (IS_NUMBER(args[0]) ? args[0].as.num : 0);
        long end   = IS_NULL(args[1]) ? (long)b->count : (IS_NUMBER(args[1]) ? args[1].as.num : 0);
        if (start < 0) start += b->count;
        if (start < 0) start = 0;
        if (end < 0) end += b->count;
        if (end < 0) end = 0;
        if (start > (long)b->count) start = b->count;
        if (end > (long)b->count) end = b->count;
        if (end < start) end = start;
        ObjByteArray *r = bytearray_new((uint32_t)(end - start));
        if (!r) return vm_raise(vm, "out of memory");
        memcpy(r->bytes, b->bytes + start, (size_t)(end - start));
        r->count = (uint32_t)(end - start);
        *result = mk_obj((Obj *)r);
        return true;
    }
    if (EQ("decodeNumber")) {
        NEED_ARGC(2);
        if (!IS_NUMBER(args[0])) return vm_raise(vm, "decodeNumber: format must be a Number");
        int fmt = args[0].as.num, w = nf_width(fmt);
        if (w < 0) return vm_raise(vm, "decodeNumber: unknown number format %d", fmt);
        long off; int little;
        read_options(IS_DICT(args[1]) ? AS_DICT(args[1]) : NULL, &off, &little);
        if (off < 0 || off + w > (long)b->count)
            return vm_raise(vm, "decodeNumber: %d bytes at offset %ld exceed length %u", w, off, b->count);
        uint64_t raw = 0;
        for (int i = 0; i < w; i++) {
            uint8_t byte = b->bytes[off + (little ? i : w - 1 - i)];
            raw |= (uint64_t)byte << (8 * i);
        }
        switch (fmt) {
        case NF_UINT8:  *result = mk_number((int32_t)(uint8_t)raw); break;
        case NF_SINT8:  *result = mk_number((int32_t)(int8_t)raw); break;
        case NF_UINT16: *result = mk_number((int32_t)(uint16_t)raw); break;
        case NF_SINT16: *result = mk_number((int32_t)(int16_t)raw); break;
        case NF_SINT32: *result = mk_number((int32_t)raw); break;
        case NF_UINT32: *result = mk_long((int64_t)(uint32_t)raw); break;
        case NF_UINT64: case NF_SINT64: *result = mk_long((int64_t)raw); break;
        case NF_FLOAT:  { uint32_t u = (uint32_t)raw; float f; memcpy(&f, &u, 4); *result = mk_float(f); break; }
        case NF_DOUBLE: { double d; memcpy(&d, &raw, 8); *result = mk_double(d); break; }
        default: return vm_raise(vm, "decodeNumber: unknown format");
        }
        return true;
    }
    if (EQ("encodeNumber")) {
        NEED_ARGC(3);
        if (!IS_NUMBER(args[1])) return vm_raise(vm, "encodeNumber: format must be a Number");
        int fmt = args[1].as.num, w = nf_width(fmt);
        if (w < 0) return vm_raise(vm, "encodeNumber: unknown number format %d", fmt);
        long off; int little;
        read_options(IS_DICT(args[2]) ? AS_DICT(args[2]) : NULL, &off, &little);
        /* Require the destination to already be large enough (like decodeNumber);
         * this avoids an unbounded/huge-offset growth path. */
        if (off < 0 || off + w > (long)b->count)
            return vm_raise(vm, "encodeNumber: %d bytes at offset %ld exceed length %u", w, off, b->count);
        uint64_t raw;
        if (fmt == NF_FLOAT)  { float f = (float)bi_as_f64(args[0]); uint32_t u; memcpy(&u, &f, 4); raw = u; }
        else if (fmt == NF_DOUBLE) { double d = bi_as_f64(args[0]); memcpy(&raw, &d, 8); }
        else raw = (uint64_t)bi_as_i64(args[0]);
        for (int i = 0; i < w; i++)
            b->bytes[off + (little ? i : w - 1 - i)] = (uint8_t)(raw >> (8 * i));
        *result = retain(recv);
        return true;
    }
    return object_base_method(vm, recv, name, argc, args, result);
}

/* --------------------------------------------------- Method / WeakRef ------ */

static bool method_method(VM *vm, Value recv, const char *name,
                          int argc, Value *args, Value *result) {
    ObjMethod *m = AS_METHOD(recv);
    if (EQ("invoke")) /* dispatch user methods too, not just native ones */
        return vm_invoke_value(vm, m->receiver, m->symbol, argc, args, result);
    return object_base_method(vm, recv, name, argc, args, result);
}

static bool weakref_method(VM *vm, Value recv, const char *name,
                           int argc, Value *args, Value *result) {
    ObjWeakRef *w = AS_WEAKREF(recv);
    if (EQ("get")) {
        NEED_ARGC(0);
        Obj *t = objmgr_get(w->target_id);
        *result = t ? retain(mk_obj(t)) : mk_null();
        return true;
    }
    if (EQ("stillAlive")) { NEED_ARGC(0); *result = mk_bool(objmgr_get(w->target_id) != NULL); return true; }
    return object_base_method(vm, recv, name, argc, args, result);
}

/* ------------------------------------------------------------- String ------ */

/* Decode one UTF-8 code point at s[i]; returns its byte length (1-4). Truncated
 * or invalid sequences are passed through as a single byte. */
static int utf8_decode(const char *s, uint32_t len, uint32_t i, uint32_t *cp) {
    unsigned char c = (unsigned char)s[i];
    if (c < 0x80) { *cp = c; return 1; }
    if ((c & 0xE0) == 0xC0 && i + 1 < len) {
        *cp = ((uint32_t)(c & 0x1F) << 6) | (s[i + 1] & 0x3F);
        return 2;
    }
    if ((c & 0xF0) == 0xE0 && i + 2 < len) {
        *cp = ((uint32_t)(c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F);
        return 3;
    }
    if ((c & 0xF8) == 0xF0 && i + 3 < len) {
        *cp = ((uint32_t)(c & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12) |
              ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F);
        return 4;
    }
    *cp = c;
    return 1;
}

/* Encode a code point as UTF-8 into out; returns bytes written (1-4). */
static int utf8_encode(uint32_t cp, uint8_t *out) {
    if (cp < 0x80)    { out[0] = (uint8_t)cp; return 1; }
    if (cp < 0x800)   { out[0] = (uint8_t)(0xC0 | (cp >> 6)); out[1] = (uint8_t)(0x80 | (cp & 0x3F)); return 2; }
    if (cp < 0x10000) { out[0] = (uint8_t)(0xE0 | (cp >> 12)); out[1] = (uint8_t)(0x80 | ((cp >> 6) & 0x3F)); out[2] = (uint8_t)(0x80 | (cp & 0x3F)); return 3; }
    out[0] = (uint8_t)(0xF0 | (cp >> 18)); out[1] = (uint8_t)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (uint8_t)(0x80 | ((cp >> 6) & 0x3F)); out[3] = (uint8_t)(0x80 | (cp & 0x3F));
    return 4;
}

/* Write the uppercase UTF-8 of `cp` into `out`; returns bytes written. A few code
 * points (Unicode SpecialCasing) uppercase to more than one character. */
static int upper_utf8(uint32_t cp, uint8_t *out) {
    if (cp >= 'a' && cp <= 'z')                        return utf8_encode(cp - 0x20, out);
    if (cp >= 0x00E0 && cp <= 0x00FE && cp != 0x00F7)  return utf8_encode(cp - 0x20, out); /* Latin-1 */
    if (cp >= 0x03B1 && cp <= 0x03C9 && cp != 0x03C2)  return utf8_encode(cp - 0x20, out); /* Greek   */
    switch (cp) {
    case 0x00DF: return utf8_encode(0x1E9E, out);                       /* ß  -> ẞ           */
    case 0x0149: { int n = utf8_encode(0x02BC, out); return n + utf8_encode(0x004E, out + n); } /* ŉ -> ʼN */
    case 0x01F0: { int n = utf8_encode(0x004A, out); return n + utf8_encode(0x030C, out + n); } /* ǰ -> J̌ */
    case 0x0390: { int n = utf8_encode(0x0399, out); n += utf8_encode(0x0308, out + n); return n + utf8_encode(0x0301, out + n); } /* ΐ -> Ϊ́ */
    case 0x03B0: { int n = utf8_encode(0x03A5, out); n += utf8_encode(0x0308, out + n); return n + utf8_encode(0x0301, out + n); } /* ΰ -> Ϋ́ */
    default:     return utf8_encode(cp, out);
    }
}

/* Write the lowercase UTF-8 of `cp` into `out`; returns bytes written. A few code
 * points (Unicode SpecialCasing) lowercase to more than one character. */
static int lower_utf8(uint32_t cp, uint8_t *out) {
    if (cp >= 'A' && cp <= 'Z')                        return utf8_encode(cp + 0x20, out);
    if (cp >= 0x00C0 && cp <= 0x00DE && cp != 0x00D7)  return utf8_encode(cp + 0x20, out); /* Latin-1 */
    if (cp >= 0x0391 && cp <= 0x03A9 && cp != 0x03A2)  return utf8_encode(cp + 0x20, out); /* Greek   */
    switch (cp) {
    case 0x0130: { int n = utf8_encode(0x0069, out); return n + utf8_encode(0x0307, out + n); } /* İ -> i̇ */
    default:     return utf8_encode(cp, out);
    }
}



static ObjString *new_case(ObjString *s, bool upper) {
    /* The result has one output byte per input byte for ASCII and the common
     * Latin/Greek case mappings, so size the scratch buffer from the input. */
    char *tmp = (char *)vm_alloc(s->length + 1);
    if (!tmp) return NULL;
    uint32_t w = 0;
    for (uint32_t i = 0; i < s->length; ) {
        uint32_t cp;
        i += (uint32_t)utf8_decode(s->data, s->length, i, &cp);
        w += (uint32_t)(upper ? upper_utf8(cp, (uint8_t *)tmp + w)
                              : lower_utf8(cp, (uint8_t *)tmp + w));
    }
    tmp[w] = '\0';
    ObjString *r = string_new(tmp, w);
    vm_free(tmp);
    return r;
}

static bool string_method(VM *vm, Value recv, const char *name,
                          int argc, Value *args, Value *result) {
    ObjString *s = AS_STRING(recv);
    if (EQ("length") || EQ("size")) { NEED_ARGC(0); *result = mk_number((int32_t)s->length); return true; }
    if (EQ("toUpper") || EQ("toLower")) {
        NEED_ARGC(0);
        ObjString *r = new_case(s, EQ("toUpper"));
        if (!r) return vm_raise(vm, "out of memory");
        *result = mk_obj((Obj *)r);
        return true;
    }
    if (EQ("substring")) {
        NEED_ARGC(2);
        long start = IS_NULL(args[0]) ? 0 : (IS_NUMBER(args[0]) ? args[0].as.num : -1);
        long end   = IS_NULL(args[1]) ? (long)s->length : (IS_NUMBER(args[1]) ? args[1].as.num : -1);
        if (start < 0 || end < 0)
            return vm_raise(vm, "String.substring indices must be Number or null");
        if (start > (long)s->length) start = s->length;
        if (end > (long)s->length) end = s->length;
        if (end < start) end = start;
        ObjString *r = string_new(s->data + start, (size_t)(end - start));
        if (!r) return vm_raise(vm, "out of memory");
        *result = mk_obj((Obj *)r);
        return true;
    }
    if (EQ("find")) {
        NEED_ARGC(1);
        if (!IS_STRING(args[0])) return vm_raise(vm, "String.find expects a String");
        ObjString *needle = AS_STRING(args[0]);
        if (needle->length == 0) { *result = mk_number(0); return true; }
        const char *hit = NULL;
        if (needle->length <= s->length) {
            for (uint32_t i = 0; i + needle->length <= s->length; i++)
                if (memcmp(s->data + i, needle->data, needle->length) == 0) { hit = s->data + i; break; }
        }
        *result = hit ? mk_number((int32_t)(hit - s->data)) : mk_null();
        return true;
    }
    if (EQ("toCharArray")) {
        NEED_ARGC(0);
        ObjArray *a = array_new(s->length);
        if (!a) return vm_raise(vm, "out of memory");
        for (uint32_t i = 0; i < s->length; i++)
            a->items[i] = mk_char((unsigned char)s->data[i]);
        a->count = s->length;
        *result = mk_obj((Obj *)a);
        return true;
    }
    if (EQ("toNumber")) {
        NEED_ARGC(0);
        char *end; long v = strtol(s->data, &end, 10);
        *result = (end != s->data && *end == '\0') ? mk_number((int32_t)v) : mk_null();
        return true;
    }
    if (EQ("toLong")) {
        NEED_ARGC(0);
        char *end; long long v = strtoll(s->data, &end, 10);
        *result = (end != s->data && *end == '\0') ? mk_long((int64_t)v) : mk_null();
        return true;
    }
    if (EQ("toFloat")) {
        NEED_ARGC(0);
        char *end; double v = strtod(s->data, &end);
        *result = (end != s->data && *end == '\0') ? mk_float((float)v) : mk_null();
        return true;
    }
    if (EQ("toDouble")) {
        NEED_ARGC(0);
        char *end; double v = strtod(s->data, &end);
        *result = (end != s->data && *end == '\0') ? mk_double(v) : mk_null();
        return true;
    }
    if (EQ("toUtf8Array")) {
        NEED_ARGC(0);
        ObjArray *a = array_new(s->length);
        if (!a) return vm_raise(vm, "out of memory");
        for (uint32_t i = 0; i < s->length; i++) a->items[i] = mk_number((unsigned char)s->data[i]);
        a->count = s->length;
        *result = mk_obj((Obj *)a);
        return true;
    }
    if (EQ("compareTo")) {
        NEED_ARGC(1);
        if (!IS_STRING(args[0])) return vm_raise(vm, "String.compareTo expects a String");
        ObjString *o = AS_STRING(args[0]);
        uint32_t n = s->length < o->length ? s->length : o->length;
        int m = memcmp(s->data, o->data, n);
        *result = mk_number(m ? (m < 0 ? -1 : 1) : (int)s->length - (int)o->length);
        return true;
    }
    return object_base_method(vm, recv, name, argc, args, result);
}

/* -------------------------------------------------------------- Array ------ */

static int cmp_values(const void *pa, const void *pb) {
    const Value *a = (const Value *)pa, *b = (const Value *)pb;
    if (bi_is_numeric(*a) && bi_is_numeric(*b)) {
        double x = bi_as_f64(*a), y = bi_as_f64(*b);
        return x < y ? -1 : x > y ? 1 : 0;
    }
    if (IS_STRING(*a) && IS_STRING(*b)) {
        ObjString *sa = AS_STRING(*a), *sb = AS_STRING(*b);
        uint32_t n = sa->length < sb->length ? sa->length : sb->length;
        int m = memcmp(sa->data, sb->data, n);
        return m ? m : (int)sa->length - (int)sb->length;
    }
    if (IS_CHAR(*a) && IS_CHAR(*b)) return (int)AS_CHAR(*a) - (int)AS_CHAR(*b);
    return 0;
}

static bool array_method(VM *vm, Value recv, const char *name,
                         int argc, Value *args, Value *result) {
    ObjArray *a = AS_ARRAY(recv);
    if (EQ("size")) { NEED_ARGC(0); *result = mk_number((int32_t)a->count); return true; }
    if (EQ("add"))  { NEED_ARGC(1); array_push(a, args[0]); *result = retain(recv); return true; }
    if (EQ("addAll")) {
        NEED_ARGC(1);
        if (!IS_ARRAY(args[0])) return vm_raise(vm, "Array.addAll expects an Array");
        ObjArray *o = AS_ARRAY(args[0]);
        /* Snapshot the count so addAll(self) appends a copy instead of looping
         * forever (the bound and the growing array would be the same field). */
        uint32_t n = o->count;
        for (uint32_t i = 0; i < n; i++) array_push(a, o->items[i]);
        *result = retain(recv);
        return true;
    }
    if (EQ("indexOf")) {
        NEED_ARGC(1);
        int32_t idx = -1;
        for (uint32_t i = 0; i < a->count; i++) if (elem_eq(a->items[i], args[0])) { idx = (int32_t)i; break; }
        *result = mk_number(idx);
        return true;
    }
    if (EQ("remove")) {
        NEED_ARGC(1);
        for (uint32_t i = 0; i < a->count; i++) {
            if (elem_eq(a->items[i], args[0])) {
                release(a->items[i]);
                memmove(&a->items[i], &a->items[i + 1], (size_t)(a->count - i - 1) * sizeof(Value));
                a->count--;
                *result = mk_bool(true);
                return true;
            }
        }
        *result = mk_bool(false);
        return true;
    }
    if (EQ("removeAll")) {
        NEED_ARGC(1);
        uint32_t w = 0; bool any = false;
        for (uint32_t i = 0; i < a->count; i++) {
            if (elem_eq(a->items[i], args[0])) { release(a->items[i]); any = true; }
            else a->items[w++] = a->items[i];
        }
        a->count = w;
        *result = mk_bool(any);
        return true;
    }
    if (EQ("reverse")) {
        NEED_ARGC(0);
        ObjArray *r = array_new(a->count);
        if (!r) return vm_raise(vm, "out of memory");
        for (uint32_t i = 0; i < a->count; i++) r->items[i] = retain(a->items[a->count - 1 - i]);
        r->count = a->count;
        *result = mk_obj((Obj *)r);
        return true;
    }
    if (EQ("slice")) {
        NEED_ARGC(2);
        long start = IS_NULL(args[0]) ? 0 : (IS_NUMBER(args[0]) ? args[0].as.num : -1);
        long end   = IS_NULL(args[1]) ? (long)a->count : (IS_NUMBER(args[1]) ? args[1].as.num : -1);
        if (start < 0) start += a->count;
        if (start < 0) start = 0;
        if (end < 0) end += a->count;
        if (end < 0) end = 0;
        if (start > (long)a->count) start = a->count;
        if (end > (long)a->count) end = a->count;
        if (end < start) end = start;
        ObjArray *r = array_new((uint32_t)(end - start));
        if (!r) return vm_raise(vm, "out of memory");
        for (long i = start; i < end; i++) r->items[i - start] = retain(a->items[i]);
        r->count = (uint32_t)(end - start);
        *result = mk_obj((Obj *)r);
        return true;
    }
    if (EQ("sort")) {
        if (argc == 1 && !IS_NULL(args[0]))
            return vm_raise(vm, "Array.sort with a custom comparator is not yet supported");
        if (argc > 1) NEED_ARGC(0);
        qsort(a->items, a->count, sizeof(Value), cmp_values);
        *result = mk_null();
        return true;
    }
    return object_base_method(vm, recv, name, argc, args, result);
}

/* --------------------------------------------------------- Dictionary ------ */

static bool dict_method(VM *vm, Value recv, const char *name,
                        int argc, Value *args, Value *result) {
    ObjDict *d = AS_DICT(recv);
    if (EQ("size"))    { NEED_ARGC(0); *result = mk_number((int32_t)d->count); return true; }
    if (EQ("isEmpty")) { NEED_ARGC(0); *result = mk_bool(d->count == 0); return true; }
    if (EQ("get")) {
        NEED_ARGC(1);
        Value v;
        *result = dict_get(d, args[0], &v) ? retain(v) : mk_null();
        return true;
    }
    if (EQ("put"))    { NEED_ARGC(2); dict_set(d, args[0], args[1]); *result = mk_null(); return true; }
    if (EQ("hasKey")) { NEED_ARGC(1); *result = mk_bool(dict_has(d, args[0])); return true; }
    if (EQ("remove")) { NEED_ARGC(1); dict_remove(d, args[0]); *result = mk_null(); return true; }
    if (EQ("keys") || EQ("values")) {
        NEED_ARGC(0);
        bool keys = EQ("keys");
        ObjArray *a = array_new(d->count);
        if (!a) return vm_raise(vm, "out of memory");
        for (uint32_t i = 0; i < d->capacity; i++)
            if (d->entries[i].state == DICT_USED)
                array_push(a, keys ? d->entries[i].key : d->entries[i].val);
        *result = mk_obj((Obj *)a);
        return true;
    }
    return object_base_method(vm, recv, name, argc, args, result);
}

/* ------------------------------------------------------------- dispatch ---- */

bool vm_invoke(VM *vm, Value recv, uint32_t sym, int argc, Value *args, Value *result) {
    const char *name = symbol_name(sym);
    if (!name) return vm_raise(vm, "invalid method symbol");
    if (IS_STRING(recv))    return string_method(vm, recv, name, argc, args, result);
    if (IS_ARRAY(recv))     return array_method(vm, recv, name, argc, args, result);
    if (IS_DICT(recv))      return dict_method(vm, recv, name, argc, args, result);
    if (IS_BYTEARRAY(recv)) return bytearray_method(vm, recv, name, argc, args, result);
    if (IS_METHOD(recv))    return method_method(vm, recv, name, argc, args, result);
    if (IS_WEAKREF(recv))   return weakref_method(vm, recv, name, argc, args, result);
    if (bi_is_numeric(recv)) return numeric_method(vm, recv, name, argc, args, result);
    if (IS_CHAR(recv))      return char_method(vm, recv, name, argc, args, result);
    return object_base_method(vm, recv, name, argc, args, result);
}

bool vm_has_method(Value recv, uint32_t sym) {
    const char *name = symbol_name(sym);
    if (!name) return false;
    static const char *base[]   = { "toString", "hashCode", "equals", "method", "weak", NULL };
    static const char *str[]    = { "length", "size", "toUpper", "toLower", "substring",
                                    "find", "toCharArray", "toNumber", "toFloat", "compareTo", NULL };
    static const char *arr[]    = { "size", "add", "addAll", "indexOf", "remove", "removeAll",
                                    "reverse", "slice", "sort", NULL };
    static const char *dic[]    = { "size", "isEmpty", "get", "put", "hasKey", "remove",
                                    "keys", "values", NULL };
    static const char *ba[]     = { "size", "add", "addAll", "indexOf", "reverse", "slice",
                                    "decodeNumber", "encodeNumber", NULL };
    static const char *numc[]   = { "abs", "toNumber", "toLong", "toFloat", "toDouble",
                                    "toChar", "compareTo", "format", NULL };
    static const char *chr[]    = { "toNumber", "toLower", "toUpper", "compareTo", NULL };
    static const char *meth[]   = { "invoke", NULL };
    static const char *weak[]   = { "get", "stillAlive", NULL };
    const char *const *lists[3] = { base, NULL, NULL };
    if (IS_STRING(recv)) lists[1] = str;
    else if (IS_ARRAY(recv)) lists[1] = arr;
    else if (IS_DICT(recv)) lists[1] = dic;
    else if (IS_BYTEARRAY(recv)) lists[1] = ba;
    else if (IS_METHOD(recv)) lists[1] = meth;
    else if (IS_WEAKREF(recv)) lists[1] = weak;
    else if (bi_is_numeric(recv)) lists[1] = numc;
    else if (IS_CHAR(recv)) lists[1] = chr;
    for (int l = 0; l < 3; l++) {
        if (!lists[l]) continue;
        for (const char *const *p = lists[l]; *p; p++) if (strcmp(*p, name) == 0) return true;
    }
    return false;
}
