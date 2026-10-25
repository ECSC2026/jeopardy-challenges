/* awk2c runtime — AwkArray: string-keyed vector of key/value entries, searched
 * by linear scan. Simplicity over asymptotics: an array operation is O(n). */
#include "awk_internal.h"

#include <stdlib.h>
#include <string.h>

/* AwkEntry (key/value pair) is defined in awk_runtime.h; key is an owned string
 * reference, shared with whatever produced it. */

/* Join subscripts with SUBSEP into an owned key string; consumes subs. */
static char *make_key(AwkValue *subs, size_t n) {
    char *ss = awk_special_str(&awk_v_SUBSEP);
    size_t sslen = awk_str_len(ss);
    Buf o = { 0 };
    for (size_t i = 0; i < n; i++) {
        if (i) bputs(&o, ss, sslen);
        char *part = awk_to_str(subs[i]);
        bputs(&o, part, awk_str_len(part));
        awk_str_release(part);
    }
    awk_str_release(ss);
    return bfinish(&o);
}

static AwkEntry *find(AwkArray *a, const char *key) {
    for (size_t i = 0; i < a->len; i++)
        if (strcmp(a->ents[i].key, key) == 0) return &a->ents[i];
    return NULL;
}

/* Find the element named by subs (consumed), creating it when `create`. The
 * entry pointer is valid only until the next insertion moves the vector. */
static AwkEntry *lookup(AwkArray *a, AwkValue *subs, size_t n, bool create) {
    char *key = make_key(subs, n);
    AwkEntry *e = find(a, key);
    if (!e && create) {
        if (a->len == a->cap) {
            a->cap = a->cap ? a->cap * 2 : 8;
            a->ents = awk_xrealloc(a->ents, a->cap * sizeof *a->ents);
        }
        e = &a->ents[a->len++];
        e->key = key;   /* takes over the key */
        e->val = awk_uninit();
        return e;
    }
    awk_str_release(key);
    return e;
}

AwkValue awk_array_get(AwkArray *a, AwkValue *subs, size_t n) {
    return awk_dup(&lookup(a, subs, n, true)->val);   /* referencing creates it */
}

void awk_array_set(AwkArray *a, AwkValue *subs, size_t n, AwkValue v) {
    awk_store(&lookup(a, subs, n, true)->val, v);
}

bool awk_array_in(AwkArray *a, AwkValue *subs, size_t n) {
    return lookup(a, subs, n, false) != NULL;
}

void awk_array_del(AwkArray *a, AwkValue *subs, size_t n) {
    char *key = make_key(subs, n);
    AwkEntry *e = find(a, key);
    if (e) {
        awk_str_release(e->key);
        awk_free(e->val);
        *e = a->ents[--a->len];   /* the last entry fills the hole; `for (k in
                                   * a)` order is unspecified anyway */
    }
    awk_str_release(key);
}

void awk_array_clear(AwkArray *a) {
    for (size_t i = 0; i < a->len; i++) {
        awk_str_release(a->ents[i].key);
        awk_free(a->ents[i].val);
    }
    a->len = 0;
}

AwkValue awk_array_length(AwkArray *a) {
    return awk_num((double)a->len);
}

/* Clear, then release the vector itself. For caller-side scratch arrays, which
 * back an omitted array argument and die at the end of the call expression. */
void awk_array_free(AwkArray *a) {
    awk_array_clear(a);
    free(a->ents);
    a->ents = NULL;
    a->cap = 0;
}
