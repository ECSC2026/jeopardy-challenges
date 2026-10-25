/* awk2c runtime — output, string built-ins, the printf engine, rand/srand. */
#include "awk_internal.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---- Simple string functions ------------------------------------- */

AwkValue awk_str_length(AwkValue v) {
    char *s = awk_to_str(v);
    AwkValue r = awk_num((double)awk_str_len(s));
    awk_str_release(s);
    return r;
}

/* The bounds are computed in `double` rather than `long`: both come straight
 * from user data, so a huge start or length would overflow the integer (UB, and
 * `substr("hello", 2, 2^63-1)` wrapping negative reads as an empty result)
 * where the double just lands outside the string and clamps. The comparisons
 * are written so NaN takes the same branch as out-of-range. */
AwkValue awk_substr(AwkValue s, AwkValue m, AwkValue n, bool have_n) {
    char *str = awk_to_str(s);
    double slen = (double)awk_str_len(str);
    double from = trunc(awk_to_num(m));         /* 1-based, truncated */
    if (!(from >= 1)) from = 1;                 /* clamp start before length */
    double want = trunc(awk_to_num(n));         /* absent => uninit => 0 */
    double to = have_n ? from + want : slen + 1;
    if (to > slen + 1) to = slen + 1;
    /* from is now in [1, slen+1] whenever there is anything to copy, so the
     * casts below are back inside the range of a size_t. */
    AwkValue r = !(to > from) ? awk_str("")
                              : awk_str_own(awk_str_new(str + (size_t)from - 1,
                                                        (size_t)(to - from)));
    awk_str_release(str);
    return r;
}

AwkValue awk_index(AwkValue s, AwkValue t) {
    char *hs = awk_to_str(s), *ne = awk_to_str(t);
    char *pos = strstr(hs, ne);
    AwkValue r = awk_num(pos ? (double)(pos - hs + 1) : 0.0);
    awk_str_release(hs); awk_str_release(ne);
    return r;
}

/* The source string may be shared with a variable or field, so the conversion
 * writes into a fresh buffer rather than in place. */
static AwkValue map_case(AwkValue s, bool upper) {
    char *str = awk_to_str(s);
    size_t n = awk_str_len(str);
    char *out = awk_str_alloc(n);
    for (size_t i = 0; i < n; i++)
        out[i] = (char)(upper ? toupper((unsigned char)str[i])
                              : tolower((unsigned char)str[i]));
    awk_str_release(str);
    return awk_str_own(out);
}

AwkValue awk_toupper(AwkValue s) { return map_case(s, true); }
AwkValue awk_tolower(AwkValue s) { return map_case(s, false); }

/* ---- printf / sprintf engine ------------------------------------- */

/* printf one directive onto a Buf, with no length limit: measure it, then
 * format it straight into the buffer's own space. */
static void bfmt(Buf *o, const char *spec, ...) {
    va_list ap, ap2;
    va_start(ap, spec);
    va_copy(ap2, ap);
    int need = vsnprintf(NULL, 0, spec, ap);
    if (need > 0) {
        breserve(o, (size_t)need);
        vsnprintf(o->b + o->len, (size_t)need + 1, spec, ap2);
        o->len += (size_t)need;
    }
    va_end(ap);
    va_end(ap2);
}

/* The argument list of one printf call, walked left to right. Taking an
 * argument lifts it out of the vector, leaving the slot uninitialized, so the
 * consumed ones and the ones no directive reached are freed by the same loop at
 * the end. A directive that runs off the end sees an uninitialized value, which
 * converts to 0 / "" — as awk does. */
typedef struct { AwkValue *v; size_t n, i; } Args;

static AwkValue next_arg(Args *a) {
    if (a->i >= a->n) return awk_uninit();
    AwkValue v = a->v[a->i];
    a->v[a->i++] = awk_uninit();
    return v;
}
static double next_num(Args *a) { return awk_to_num(next_arg(a)); }

/* Format args[1..n-1] under format string args[0]. Consumes all args and
 * returns an owned string reference.
 *
 * Flags, width and precision are never interpreted here: the directive is
 * copied verbatim into `spec` and handed straight back to the C library, whose
 * printf is what defines them. Ours is only what C's does not do for us —
 * resolving a `*` field into the digits its argument stands for, picking the C
 * type each conversion wants to be handed, and the three cases the switch ends
 * with, where awk and C disagree. */
static char *format(AwkValue *args, size_t n) {
    Args a = { args, n, 0 };
    char *fmt = awk_to_str(next_arg(&a));   /* no arguments at all => "" */
    Buf o = { 0 };
    for (const char *p = fmt; *p;) {
        if (*p != '%') { bputc(&o, *p++); continue; }
        const char *start = p++;

        /* spec[64] holds any directive worth writing: 7 flags, two 15-digit
         * fields, a '.' and a 3-character length+conversion tail reach 42.
         * Copying stops at 48 characters, so an absurdly long one leaves its
         * own trailing digits to be read as an unknown conversion and printed
         * literally; the 16 bytes past that cover the widest `*` substitution
         * (11 for an int) and the conversion tail (4) on top of it. */
        char spec[64];
        int si = 1, dot = 0;    /* dot: index of the '.', 0 if there is none */
        spec[0] = '%';
        while (*p && strchr("-+ #0123456789.*", *p) && si < 48) {
            if (*p == '.') dot = si;
            if (*p != '*') { spec[si++] = *p++; continue; }
            p++;
            int field = awk_num_to_int(next_num(&a));
            /* A negative `*` precision means no precision at all, so it takes
             * the '.' with it; a negative width needs no such care, the '-'
             * snprintf writes in front of it being the left-justify flag. */
            if (field < 0 && dot && si == dot + 1) si = dot;
            else si += snprintf(spec + si, 16, "%d", field);
        }
        char conv = *p ? *p++ : 0;
        int noprec = dot ? dot : si;   /* the same spec without its precision */
        /* The tail. The integer conversions are handed a `long long`, so they
         * are the ones that need a length modifier in front of the letter. */
        if (conv && strchr("diouxX", conv)) { spec[si++] = 'l'; spec[si++] = 'l'; }
        spec[si] = conv; spec[si + 1] = '\0';

        switch (conv) {
        case 'd': case 'i': {
            double d = next_num(&a);
            if (d >= -9.2e18 && d <= 9.2e18) bfmt(&o, spec, (long long)d);
            else {
                /* Outside long long (or NaN/inf): awk prints the full integer
                 * digits, so hand the double to "%.0f" under the same flags and
                 * width. A precision is dropped with the integer conversion it
                 * belonged to — it could only have widened a number already
                 * printed in full. */
                memcpy(spec + noprec, ".0f", 4);
                bfmt(&o, spec, d);
            }
            break;
        }
        case 'o': case 'u': case 'x': case 'X':
            /* No full-digit answer exists for a value the radix conversions
             * cannot represent, so an out-of-range one clamps (awk prints its
             * decimal form instead — a divergence, in POSIX-undefined ground). */
            bfmt(&o, spec, (unsigned long long)awk_num_to_ll(next_num(&a)));
            break;
        case 'e': case 'E': case 'f': case 'F': case 'g': case 'G': case 'a': case 'A':
            bfmt(&o, spec, next_num(&a));
            break;
        case 'c': {
            /* A number prints as its character code, a string as its first —
             * uninitialized reads as the number 0 under either rule. A precision
             * is meaningless on one character and undefined in C, so it goes. */
            spec[noprec] = 'c'; spec[noprec + 1] = '\0';
            AwkValue v = next_arg(&a);
            bfmt(&o, spec, v.kind == AWK_STR ? (unsigned char)v.str[0] : awk_num_to_int(v.num));
            awk_free(v);
            break;
        }
        case 's': {
            char *sv = awk_to_str(next_arg(&a));
            bfmt(&o, spec, sv);
            awk_str_release(sv);
            break;
        }
        case '%':
            bputc(&o, '%');   /* any width it carries is dropped, as awk does */
            break;
        default:
            /* Unknown, or a `%` the format ended before finishing: literally. */
            bputs(&o, start, (size_t)(p - start));
            break;
        }
    }
    awk_str_release(fmt);
    for (size_t i = 0; i < n; i++) awk_free(args[i]);   /* whatever went unread */
    return bfinish(&o);
}

/* ---- Output ------------------------------------------------------ */

static void put_str(char *s) {   /* consumes s */
    fwrite(s, 1, awk_str_len(s), stdout);
    awk_str_release(s);
}

void awk_printf(AwkValue *args, size_t n) { put_str(format(args, n)); }

AwkValue awk_sprintf(AwkValue *args, size_t n) { return awk_str_own(format(args, n)); }

void awk_print(AwkValue *args, size_t n) {
    char *ofs = awk_special_str(&awk_v_OFS);
    for (size_t i = 0; i < n; i++) {
        if (i) fwrite(ofs, 1, awk_str_len(ofs), stdout);
        put_str(awk_to_out_str(args[i]));   /* numbers via OFMT, not CONVFMT */
    }
    awk_str_release(ofs);
    put_str(awk_special_str(&awk_v_ORS));
}

/* ---- rand / srand ------------------------------------------------ */

static unsigned prev_seed = 0;

double awk_rand(void) { return (double)rand() / ((double)RAND_MAX + 1.0); }

double awk_srand(double seed, bool have_seed) {
    unsigned old = prev_seed;
    prev_seed = have_seed ? (unsigned)awk_num_to_ll(seed) : (unsigned)time(NULL);
    srand(prev_seed);
    return (double)old;
}
