/* awk2c runtime — internal helpers shared across the runtime .c files but not
 * part of the public API (awk_runtime.h). Not included by generated code. */
#ifndef AWK_INTERNAL_H
#define AWK_INTERNAL_H

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#include "awk_runtime.h"

/* ---- Checked allocation (abort on OOM) --------------------------- */
void *awk_xmalloc(size_t n);
void *awk_xrealloc(void *p, size_t n);

/* ---- Refcounted strings (see the model note in awk_runtime.h) ----- */
/* awk_str_retain / awk_str_own are declared in awk_runtime.h: generated code
 * calls them to walk `for (k in a)` inline. */
size_t awk_str_len(const char *s);  /* length, without the strlen. NULL-safe. */
/* Allocate or resize the block behind `s` (NULL to allocate) to hold `cap`
 * bytes, the NUL included, keeping its one reference. Only ever applied to a
 * string still under construction, hence uniquely owned. */
char *awk_str_grow(char *s, size_t cap);
/* Fix the final length of such a string and NUL-terminate it. */
char *awk_str_finish(char *s, size_t len);
char *awk_str_alloc(size_t len);                 /* len characters, uninitialized */
char *awk_str_new(const char *src, size_t len);  /* len characters, copied */

/* Wrap an owned string reference in a value without copying it. awk_str_own is
 * in awk_runtime.h (see above). */
AwkValue awk_strnum_own(char *s);   /* AWK_STRNUM if numeric-looking */

/* awk_to_str under OFMT instead of CONVFMT: what `print` writes. */
char *awk_to_out_str(AwkValue v);

/* The string form of a special variable (FS, OFS, ORS, RS, SUBSEP), as an owned
 * reference the caller releases. A number assigned to one of them is the string
 * AWK converts it to — `OFS = 0` is `OFS = "0"` — so reading one is an ordinary
 * conversion and not a `v->str ? v->str : default` borrow, which would read
 * every assigned number as "unset". The value itself is left alone, so the
 * variable still holds the number it was assigned (`RS = RS + 1` works). */
static inline char *awk_special_str(const AwkValue *v) { return awk_to_str(awk_dup(v)); }

/* double -> integer with the out-of-range and NaN cases pinned to the ends of
 * the range rather than left to UB: every integer conversion in the runtime
 * takes its value straight from user data. 2^63 is exact as a double, so the
 * limit tests are exact too. */
static inline long long awk_num_to_ll(double n) {
    if (n >= 9223372036854775808.0) return LLONG_MAX;
    if (n < -9223372036854775808.0) return LLONG_MIN;
    return isnan(n) ? 0 : (long long)n;
}
static inline int awk_num_to_int(double n) {
    long long v = awk_num_to_ll(n);
    return v > INT_MAX ? INT_MAX : v < INT_MIN ? INT_MIN : (int)v;
}

/* ---- Growable vector of owned string references ------------------- */
/* Backs both the current record's fields and split(); the pieces are ordinary
 * refcounted strings, so handing one out is a retain. */
typedef struct { char **v; size_t len, cap; } StrVec;
void awk_vec_push(StrVec *sv, const char *s, size_t len);  /* copies s */
void awk_vec_clear(StrVec *sv);                            /* release all */

/* ---- Records and fields ------------------------------------------ */
/* The current $0, rebuilt from the fields first if any were assigned. Borrowed
 * from the record engine and never NULL: valid only until the next record is
 * read or a field is written, so retain it to keep it past the call that took
 * it. (`char *` rather than `const char *` only so awk_str_retain accepts it.) */
char *awk_record_str(void);
/* Split `s` by `fs` (NULL => FS), appending the pieces to `out`. With `nl_sep`,
 * a newline separates fields as well — paragraph mode, which is the record
 * engine's business alone (split() never does it, matching the oracle). */
void awk_field_split(const char *s, const char *fs, StrVec *out, bool nl_sep);

/* ---- Growable output buffer -------------------------------------- */
/* Builds a string in place, so bfinish hands out the buffer itself rather than
 * a copy of it. Everything that assembles a string (records, concatenation,
 * sub/gsub, printf) goes through here. */
typedef struct { char *b; size_t len, cap; } Buf;

static inline void breserve(Buf *o, size_t extra) {
    /* The +1 keeps room for the NUL and makes bfinish's breserve(o, 0) the
     * point where an empty Buf still gets a buffer to hand out. */
    if (o->len + extra + 1 > o->cap) {
        while (o->len + extra + 1 > o->cap) o->cap = o->cap ? o->cap * 2 : 64;
        o->b = awk_str_grow(o->b, o->cap);
    }
}
static inline void bputc(Buf *o, char c) { breserve(o, 1); o->b[o->len++] = c; }
static inline void bputs(Buf *o, const char *s, size_t l) {
    breserve(o, l); memcpy(o->b + o->len, s, l); o->len += l;
}
/* NUL-terminate and hand back the string, transferring its one reference. */
static inline char *bfinish(Buf *o) { breserve(o, 0); return awk_str_finish(o->b, o->len); }

#endif /* AWK_INTERNAL_H */
