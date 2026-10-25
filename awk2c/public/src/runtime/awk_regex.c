/* awk2c runtime — regex matching, field splitting, sub/gsub.
 *
 * Every pattern is compiled where it is used and freed again on the way out.
 * A pattern is a run-time string (`$0 ~ x` names no literal), so reusing a
 * compiled one means a cache keyed on the pattern text, and the whole of it —
 * the keyed store, its growth, its never-evicted lifetime — buys only a
 * constant factor per match. The regcomp is paid instead. */
#include "awk_internal.h"

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Compile an ERE into `re`, which the caller regfrees. Fatal on a bad pattern:
 * the text comes from the program, and there is no way to carry on without it. */
static void compile(regex_t *re, const char *pat) {
    int rc = regcomp(re, pat, REG_EXTENDED);
    if (rc) {
        char buf[256];
        regerror(rc, re, buf, sizeof buf);
        fprintf(stderr, "awk2c: bad regex /%s/: %s\n", pat, buf);
        awk_fatal("regex compile error");
    }
}

/* ---- Matching ---------------------------------------------------- */

/* First match of `pat` in `s`, reported in *m. Consumes both. */
static bool match_first(AwkValue s, AwkValue pat, regmatch_t *m) {
    char *ps = awk_to_str(pat), *str = awk_to_str(s);
    regex_t re;
    compile(&re, ps);
    bool found = regexec(&re, str, 1, m, 0) == 0;
    regfree(&re);
    awk_str_release(ps);
    awk_str_release(str);
    return found;
}

bool awk_regex_match(AwkValue s, AwkValue pat) {
    regmatch_t m;
    return match_first(s, pat, &m);
}

/* match(): the position of the first match, also published as RSTART/RLENGTH. */
AwkValue awk_match(AwkValue s, AwkValue pat) {
    regmatch_t m;
    double start = 0, len = -1;
    if (match_first(s, pat, &m)) {
        start = (double)(m.rm_so + 1);
        len = (double)(m.rm_eo - m.rm_so);
    }
    awk_store(&awk_v_RSTART, awk_num(start));
    awk_store(&awk_v_RLENGTH, awk_num(len));
    return awk_num(start);
}

/* ---- Field splitting -------------------------------------------- */

void awk_field_split(const char *s, const char *fs, StrVec *out, bool nl_sep) {
    /* FS, when the caller named no separator of its own — as a value, so a
     * number assigned to it is the string it converts to (`FS = 0` splits on
     * "0"), not "unset". */
    char *own = fs ? NULL : awk_special_str(&awk_v_FS);
    if (own) fs = own;
    if (fs[0] == '\0') {
        /* empty FS: one field per character (byte, in the C locale). Checked
         * first so the fs[1] tests below never read past a 1-byte string. */
        for (const char *p = s; *p; p++) awk_vec_push(out, p, 1);
    } else if (fs[0] == ' ' && fs[1] == '\0') {
        /* default: runs of blanks, leading/trailing ignored */
        const char *p = s;
        while (*p) {
            while (*p == ' ' || *p == '\t' || *p == '\n') p++;
            if (!*p) break;
            const char *st = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
            awk_vec_push(out, st, (size_t)(p - st));
        }
    } else if (fs[1] == '\0') {
        /* single-character separator (empty fields allowed). This is also the
         * only mode paragraph mode adds newline to: the default FS already
         * splits on it, and an empty or multi-character FS does not gain it —
         * `RS=""; FS="[,]"` keeps a newline inside a field, per the oracle. */
        char c = fs[0];
        const char *st = s;
        for (const char *p = s; *s; p++) {   /* an empty record has no fields */
            if (*p == c || (nl_sep && *p == '\n') || *p == '\0') {
                awk_vec_push(out, st, (size_t)(p - st));
                if (*p == '\0') break;
                st = p + 1;
            }
        }
    } else {
        /* multi-character FS: ERE regex split */
        regex_t re;
        compile(&re, fs);
        const char *p = s;
        regmatch_t m;
        int flags = 0;
        while (*p && regexec(&re, p, 1, &m, flags) == 0) {
            if (m.rm_eo == 0) break;   /* empty match: stop to avoid a loop */
            awk_vec_push(out, p, (size_t)m.rm_so);
            p += m.rm_eo;
            flags = REG_NOTBOL;
        }
        awk_vec_push(out, p, strlen(p));
        regfree(&re);
    }
    awk_str_release(own);
}

size_t awk_split(AwkValue s, AwkArray *arr, const char *fs) {
    char *str = awk_to_str(s);
    awk_array_clear(arr);
    StrVec pieces = { 0 };
    awk_field_split(str, fs, &pieces, false);
    for (size_t i = 0; i < pieces.len; i++) {
        AwkValue key = awk_num((double)(i + 1));
        awk_array_set(arr, &key, 1, awk_strnum_own(pieces.v[i]));   /* takes the piece */
    }
    free(pieces.v);
    awk_str_release(str);
    return pieces.len;
}

/* ---- sub / gsub -------------------------------------------------- */

/* Expand a replacement string: `&` = matched text, `\&` = literal &. */
static void append_repl(Buf *o, const char *repl, const char *matched, size_t mlen) {
    for (const char *r = repl; *r; r++) {
        if (*r == '\\' && (r[1] == '&' || r[1] == '\\')) { bputc(o, r[1]); r++; }
        else if (*r == '&') bputs(o, matched, mlen);
        else bputc(o, *r);
    }
}

AwkValue awk_subst(AwkValue pat, AwkValue repl, AwkValue target, bool global, size_t *count) {
    char *ps = awk_to_str(pat);
    char *rstr = awk_to_str(repl);
    char *str = awk_to_str(target);
    regex_t re;
    compile(&re, ps);
    Buf o = { 0 };
    size_t cnt = 0;
    int flags = 0;
    const char *p = str;
    regmatch_t m;
    while (regexec(&re, p, 1, &m, flags) == 0) {
        bputs(&o, p, (size_t)m.rm_so);
        append_repl(&o, rstr, p + m.rm_so, (size_t)(m.rm_eo - m.rm_so));
        cnt++;
        if (m.rm_eo == m.rm_so) {
            /* empty match: substituted above, then keep one char and advance */
            if (p[m.rm_eo] == '\0') { p += m.rm_eo; break; }
            bputc(&o, p[m.rm_eo]);
            p += m.rm_eo + 1;
        } else {
            p += m.rm_eo;
        }
        flags = REG_NOTBOL;
        if (!global || *p == '\0') break;
    }
    bputs(&o, p, strlen(p));
    *count = cnt;
    regfree(&re);
    awk_str_release(ps);
    awk_str_release(rstr);
    awk_str_release(str);
    return awk_str_own(bfinish(&o));
}
