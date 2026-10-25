/* value.c — operations over the tagged Value: refcounting glue, equality,
 * hashing and debug rendering. */
#include "value.h"
#include "object.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------- reference counting --- */

Value retain(Value v) {
    if (v.type == VAL_OBJ && v.as.obj) obj_incref(v.as.obj);
    return v;
}

void release(Value v) {
    if (v.type == VAL_OBJ && v.as.obj) obj_decref(v.as.obj);
}

/* ----------------------------------------------------- equality / hash ----- */

bool value_equals(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
    case VAL_NULL:   return true;
    case VAL_BOOL:   return a.as.boolean == b.as.boolean;
    case VAL_NUMBER: return a.as.num == b.as.num;
    case VAL_LONG:   return a.as.lng == b.as.lng;
    case VAL_FLOAT:  return a.as.f32 == b.as.f32;
    case VAL_DOUBLE: return a.as.f64 == b.as.f64;
    case VAL_CHAR:   return a.as.ch == b.as.ch;
    case VAL_SYMBOL: return a.as.sym == b.as.sym;
    case VAL_OBJ: {
        Obj *oa = a.as.obj, *ob = b.as.obj;
        if (oa == ob) return true;
        if (!oa || !ob) return false;
        /* Interned strings share identity, but compare by content defensively. */
        if (obj_type(oa) == OBJ_STRING && obj_type(ob) == OBJ_STRING) {
            ObjString *sa = (ObjString *)oa, *sb = (ObjString *)ob;
            return sa->length == sb->length &&
                   memcmp(sa->data, sb->data, sa->length) == 0;
        }
        return false; /* other objects: identity, already checked */
    }
    }
    return false;
}

static uint32_t hash_bytes(const void *p, size_t n) {
    const uint8_t *b = (const uint8_t *)p;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 16777619u; }
    return h;
}

uint32_t value_hash(Value v) {
    switch (v.type) {
    case VAL_NULL:   return 0u;
    case VAL_BOOL:   return v.as.boolean ? 1u : 2u;
    case VAL_NUMBER: return (uint32_t)v.as.num;
    case VAL_LONG:   return hash_bytes(&v.as.lng, sizeof(v.as.lng));
    /* Normalise -0.0 to +0.0 (adding 0.0 does this per IEEE) so that values that
     * compare equal always hash equal — the hash-table invariant. */
    case VAL_FLOAT:  { float  f = v.as.f32 + 0.0f; return hash_bytes(&f, sizeof f); }
    case VAL_DOUBLE: { double d = v.as.f64 + 0.0;  return hash_bytes(&d, sizeof d); }
    case VAL_CHAR:   return v.as.ch;
    case VAL_SYMBOL: return v.as.sym * 2654435761u;
    case VAL_OBJ:
        if (v.as.obj && obj_type(v.as.obj) == OBJ_STRING)
            return ((ObjString *)v.as.obj)->hash;
        return v.as.obj ? obj_id(v.as.obj) * 2654435761u : 0u;
    }
    return 0u;
}

/* --------------------------------------------------------------- repr ------ */

size_t value_repr(Value v, char *buf, size_t n) {
    switch (v.type) {
    case VAL_NULL:   return (size_t)snprintf(buf, n, "null");
    case VAL_BOOL:   return (size_t)snprintf(buf, n, "%s", v.as.boolean ? "true" : "false");
    case VAL_NUMBER: return (size_t)snprintf(buf, n, "%" PRId32, v.as.num);
    case VAL_LONG:   return (size_t)snprintf(buf, n, "%" PRId64, v.as.lng);
    case VAL_FLOAT:  return (size_t)snprintf(buf, n, "%g", (double)v.as.f32);
    case VAL_DOUBLE: return (size_t)snprintf(buf, n, "%g", v.as.f64);
    case VAL_CHAR: {
        /* Encode the code point as UTF-8 (a Char is a full unicode code point). */
        uint32_t c = v.as.ch;
        char u[5]; int k = 0;
        if (c < 0x80) {
            u[k++] = (char)c;
        } else if (c < 0x800) {
            u[k++] = (char)(0xC0 | (c >> 6));
            u[k++] = (char)(0x80 | (c & 0x3F));
        } else if (c < 0x10000) {
            u[k++] = (char)(0xE0 | (c >> 12));
            u[k++] = (char)(0x80 | ((c >> 6) & 0x3F));
            u[k++] = (char)(0x80 | (c & 0x3F));
        } else {
            u[k++] = (char)(0xF0 | (c >> 18));
            u[k++] = (char)(0x80 | ((c >> 12) & 0x3F));
            u[k++] = (char)(0x80 | ((c >> 6) & 0x3F));
            u[k++] = (char)(0x80 | (c & 0x3F));
        }
        u[k] = '\0';
        return (size_t)snprintf(buf, n, "%s", u);
    }
    case VAL_SYMBOL: {
        const char *nm = symbol_name(v.as.sym);
        return (size_t)snprintf(buf, n, ":%s", nm ? nm : "?");
    }
    case VAL_OBJ:
        if (!v.as.obj) return (size_t)snprintf(buf, n, "null");
        if (obj_type(v.as.obj) == OBJ_STRING) {
            /* Copy raw bytes (strings may contain embedded NULs); never read past
             * the string's length. Result is always NUL-terminated. */
            ObjString *s = (ObjString *)v.as.obj;
            size_t len = s->length;
            if (n) {
                size_t c = len < n - 1 ? len : n - 1;
                memcpy(buf, s->data, c);
                buf[c] = '\0';
            }
            return len;
        }
        /* Default object rendering uses the heap object id. */
        return (size_t)snprintf(buf, n, "<%s:%u>",
                                obj_type_name(obj_type(v.as.obj)), obj_id(v.as.obj));
    }
    return (size_t)snprintf(buf, n, "?");
}
