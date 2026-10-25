/* awk2c runtime — AwkValue scalars, operators, and special variables.
 *
 * An AwkValue has one authoritative representation per kind (AWK_UNINIT /
 * AWK_NUM / AWK_STR / AWK_STRNUM); the other form is derived on demand and
 * never cached back, because a number's string form is context-dependent (OFMT
 * vs CONVFMT vs integral). The AWK_STR/AWK_STRNUM split is the lever that
 * decides comparison semantics — it is what keeps `$1 < $2` numeric and
 * `"10" < "9"` a string comparison — and a value is classified once, where it
 * is created.
 *
 * Strings are refcounted and shared: a `char *` is a heap block with a
 * {refs,len} header sitting just before the characters, touched only through
 * awk_str_retain / awk_str_release / awk_str_len — never free(), never strlen().
 * Two rules make that sharing safe. Strings are immutable once built, since one
 * buffer may simultaneously be a variable, a field, an array key and an array
 * element; build a new one rather than writing through. And every store
 * consumes an owned value, which is what makes `x = x`, `$0 = $0` and `$1 = $1`
 * safe with no ordering care — the incoming reference keeps the buffer alive
 * while the slot drops its old one.
 *
 * Ownership is stated by the parameter shape (see awk_runtime.h): AwkValue is
 * consumed, AwkValue * is borrowed, char * is an owned string reference and
 * const char * a borrowed one. Every conversion out of a value consumes it, and
 * there is one entry point per operation with no borrowing sibling — a
 * borrowing entry point would push naming and freeing the argument back onto
 * the generated code, which owes no frees today. */
#include "awk_internal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Special variables (POSIX defaults). Strings are owned AwkValues.    */
/* ------------------------------------------------------------------ */
AwkValue awk_v_NR, awk_v_NF, awk_v_FS, awk_v_OFS, awk_v_ORS,
         awk_v_RS, awk_v_SUBSEP, awk_v_CONVFMT, awk_v_OFMT,
         awk_v_RSTART, awk_v_RLENGTH;

_Noreturn void awk_fatal(const char *msg) {
    fflush(stdout);
    fputs("awk2c: ", stderr);
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(2);
}

void *awk_xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) awk_fatal("out of memory");
    return p;
}
void *awk_xrealloc(void *p, size_t n) {
    p = realloc(p, n);
    if (!p) awk_fatal("out of memory");
    return p;
}

/* ------------------------------------------------------------------ */
/* Refcounted strings.                                                 */
/* ------------------------------------------------------------------ */
/* One heap block per string, laid out [Hdr][characters][NUL] with the pointer
 * everything else passes around aimed at the characters — so a string is still
 * a plain `char *` and only its lifetime rules change. Sharing is a counter
 * bump, which is what keeps `x = $0`, an array read and a function argument
 * from copying. The value graph is acyclic (a scalar cannot hold an array), so
 * counting is complete on its own; no cycle collection is needed. Strings are
 * immutable once finished — a shared buffer may be read by anyone. */
typedef struct { size_t refs, len; } Hdr;
#define HDR(s) ((Hdr *)(void *)(s) - 1)

char *awk_str_grow(char *s, size_t cap) {
    Hdr *h = s ? awk_xrealloc(HDR(s), sizeof *h + cap) : awk_xmalloc(sizeof *h + cap);
    if (!s) h->refs = 1;
    return (char *)(h + 1);
}
char *awk_str_finish(char *s, size_t len) {
    HDR(s)->len = len;
    s[len] = '\0';
    return s;
}
char *awk_str_alloc(size_t len) { return awk_str_finish(awk_str_grow(NULL, len + 1), len); }
char *awk_str_new(const char *src, size_t len) {
    char *s = awk_str_alloc(len);
    memcpy(s, src, len);
    return s;
}
char *awk_str_retain(char *s) {
    if (s) HDR(s)->refs++;
    return s;
}
void awk_str_release(char *s) {
    if (s && --HDR(s)->refs == 0) free(HDR(s));
}
size_t awk_str_len(const char *s) { return s ? HDR(s)->len : 0; }

/* A vector of such strings, grown by doubling. Clearing keeps the capacity:
 * the field vector is refilled once per input record. */
void awk_vec_push(StrVec *sv, const char *s, size_t len) {
    if (sv->len == sv->cap) {
        sv->cap = sv->cap ? sv->cap * 2 : 8;
        sv->v = awk_xrealloc(sv->v, sv->cap * sizeof *sv->v);
    }
    sv->v[sv->len++] = awk_str_new(s, len);
}
void awk_vec_clear(StrVec *sv) {
    for (size_t i = 0; i < sv->len; i++) awk_str_release(sv->v[i]);
    sv->len = 0;
}

/* True if s is a POSIX numeric string (optional blanks around a number). */
static bool awk_looks_numeric(const char *s) {
    const char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\n') p++;
    if (*p == '\0') return false;
    char *end;
    strtod(p, &end);
    if (end == p) return false;
    while (*end == ' ' || *end == '\t' || *end == '\n') end++;
    return *end == '\0';
}

AwkValue awk_uninit(void) { return (AwkValue){ AWK_UNINIT, 0.0, NULL }; }
AwkValue awk_num(double n) { return (AwkValue){ AWK_NUM, n, NULL }; }

AwkValue awk_str_own(char *s) { return (AwkValue){ AWK_STR, 0.0, s }; }

/* Strnum classification happens here, once, at value creation. */
AwkValue awk_strnum_own(char *s) {
    if (awk_looks_numeric(s)) return (AwkValue){ AWK_STRNUM, strtod(s, NULL), s };
    return (AwkValue){ AWK_STR, 0.0, s };
}

AwkValue awk_str(const char *s) { return awk_str_own(awk_str_new(s, strlen(s))); }

AwkValue awk_dup(const AwkValue *v) {
    AwkValue r = *v;
    awk_str_retain(r.str);
    return r;
}

void awk_free(AwkValue v) {
    awk_str_release(v.str);
}

/* ---- Conversions out of a value ---------------------------------- */
/* Named for what they produce, and each consumes its argument like every other
 * by-value AwkValue parameter — there is no borrowing sibling to choose
 * between, here or at the public boundary. */

double awk_to_num(AwkValue v) {
    double d = 0.0;
    if (v.kind == AWK_NUM || v.kind == AWK_STRNUM) d = v.num;
    else if (v.kind == AWK_STR) d = strtod(v.str, NULL);
    awk_free(v);
    return d;
}

bool awk_to_bool(AwkValue v) {
    bool t = false;
    if (v.kind == AWK_NUM || v.kind == AWK_STRNUM) t = v.num != 0.0;
    else if (v.kind == AWK_STR) t = v.str[0] != '\0';
    awk_free(v);
    return t;
}

/* Format a number as AWK would: integral values without a decimal point,
 * otherwise via `fmt` (CONVFMT / OFMT). */
static char *fmt_num(double n, const char *fmt) {
    char buf[350];   /* fits "%.0f" of any finite double (~309 digits) */
    int len;
    if (isfinite(n) && n == trunc(n)) {
        /* Integral values print as their full digits whatever CONVFMT/OFMT
         * says, including ones no integer type holds (1e20 ->
         * 100000000000000000000). -0 is normalized on the way: awk prints it
         * as "0", where "%.0f" on its own would give "-0". */
        if (n == 0) n = 0;
        len = snprintf(buf, sizeof buf, "%.0f", n);
    } else {
        len = snprintf(buf, sizeof buf, fmt, n);
    }
    /* A wide user CONVFMT ("%500f") truncates; snprintf still reports what it
     * would have written, so clamp before trusting it as a length. */
    if (len >= (int)sizeof buf) len = (int)sizeof buf - 1;
    return awk_str_new(buf, (size_t)len);
}

/* The string form of a value the caller is finished with. A value that already
 * holds a string hands over that very reference — nothing is copied and no
 * counter moves — where converting it as a *borrowed* value would retain a
 * reference that the release a few lines later only undoes. Every conversion
 * site in the runtime owns its value, so this consuming form is the only one
 * there is. Only AWK_NUM has anything to build, and only that case sees the
 * format, which is why the result is never cached back. */
static char *to_str_fmt(AwkValue v, const AwkValue *fmt) {
    if (v.str) return v.str;
    if (v.kind != AWK_NUM) return awk_str_alloc(0);
    if (fmt->str) return fmt_num(v.num, fmt->str);
    /* CONVFMT/OFMT may hold a number itself — `CONVFMT = 7` is the (useless but
     * legal) format string "7". Rendering *that* uses the built-in default
     * rather than CONVFMT again, which is what stops this from recursing. */
    char *f = fmt->kind == AWK_NUM ? fmt_num(fmt->num, "%.6g") : awk_str_alloc(0);
    char *r = fmt_num(v.num, f);
    awk_str_release(f);
    return r;
}

char *awk_to_str(AwkValue v) { return to_str_fmt(v, &awk_v_CONVFMT); }
char *awk_to_out_str(AwkValue v) { return to_str_fmt(v, &awk_v_OFMT); }

/* A store consumes an owned value, so a self-assignment (`x = x`) hands the slot
 * back a string it holds a *second* reference to — the old contents can be
 * released first without any ordering care. */
void awk_store(AwkValue *slot, AwkValue v) {
    awk_str_release(slot->str);
    *slot = v;
}

/* ---- Operators (consume args) ------------------------------------ */

AwkValue awk_add(AwkValue a, AwkValue b) { return awk_num(awk_to_num(a) + awk_to_num(b)); }
AwkValue awk_sub(AwkValue a, AwkValue b) { return awk_num(awk_to_num(a) - awk_to_num(b)); }
AwkValue awk_mul(AwkValue a, AwkValue b) { return awk_num(awk_to_num(a) * awk_to_num(b)); }

AwkValue awk_div(AwkValue a, AwkValue b) {
    double x = awk_to_num(a), y = awk_to_num(b);
    if (y == 0.0) awk_fatal("division by zero");
    return awk_num(x / y);
}
AwkValue awk_mod(AwkValue a, AwkValue b) {
    double x = awk_to_num(a), y = awk_to_num(b);
    if (y == 0.0) awk_fatal("division by zero in %");
    return awk_num(fmod(x, y));
}
AwkValue awk_pow(AwkValue a, AwkValue b) { return awk_num(pow(awk_to_num(a), awk_to_num(b))); }

/* Only a number has anything to build here: every other kind hands over the
 * string it already holds, so concatenating a field or a variable reaches the
 * copy below without a counter moving on the way. */
AwkValue awk_concat(AwkValue a, AwkValue b) {
    char *x = awk_to_str(a), *y = awk_to_str(b);
    size_t lx = awk_str_len(x), ly = awk_str_len(y);
    char *r = awk_str_alloc(lx + ly);
    memcpy(r, x, lx);
    memcpy(r + lx, y, ly);
    awk_str_release(x);
    awk_str_release(y);
    return awk_str_own(r);
}

/* Numeric comparison when both sides are number-capable (NUM/STRNUM/UNINIT);
 * string comparison otherwise. Returns <0, 0, >0. Consumes a and b. */
int awk_cmp(AwkValue a, AwkValue b) {
    bool a_num = a.kind == AWK_NUM || a.kind == AWK_STRNUM || a.kind == AWK_UNINIT;
    bool b_num = b.kind == AWK_NUM || b.kind == AWK_STRNUM || b.kind == AWK_UNINIT;
    if (a_num && b_num) {
        double x = awk_to_num(a), y = awk_to_num(b);
        return (x < y) ? -1 : (x > y) ? 1 : 0;
    }
    char *sa = awk_to_str(a), *sb = awk_to_str(b);
    int r = strcmp(sa, sb);
    awk_str_release(sa); awk_str_release(sb);
    return r;
}
