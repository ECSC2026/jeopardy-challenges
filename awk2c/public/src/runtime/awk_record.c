/* awk2c runtime — record reading and field splitting.
 *
 * Standard input is the only input stream: there is no file list to advance
 * through, no FILENAME and no FNR, so reading a record is just taking the next
 * one off stdin.
 *
 * $0 and the field vector are two views of the same record, kept in step
 * lazily. Reading a record splits it into fields by FS at once, since NF and
 * $1.. are what most programs want; writing a field only marks $0 dirty, and
 * $0 is rebuilt from the fields with OFS between them the next time anything
 * asks for it. That keeps a loop of field assignments from rebuilding the whole
 * record once per assignment. Both directions hand out *shared* string
 * references rather than copies — a field read is the runtime's hottest
 * operation. */
#include "awk_internal.h"

#include <stdio.h>
#include <string.h>

/* ---- Current record and fields ----------------------------------- */

static char  *rec;         /* $0 (owned string reference) */
static bool   rec_dirty;   /* fields changed; $0 needs rebuild */
static StrVec fields;      /* $1..$NF (owned string references) */

static void set_nf_var(void) { awk_store(&awk_v_NF, awk_num((double)fields.len)); }

/* The record separator's first character, which is all of RS the runtime reads,
 * or '\0' for paragraph mode (RS == ""). */
static char rs_char(void) {
    char *rs = awk_special_str(&awk_v_RS);
    char c = rs[0];
    awk_str_release(rs);
    return c;
}

/* Split `rec` into fields according to FS (shared splitter in awk_regex.c). In
 * paragraph mode a newline separates fields too, whatever FS says. */
static void split_record(void) {
    awk_vec_clear(&fields);
    awk_field_split(rec, NULL, &fields, rs_char() == '\0');
    set_nf_var();
}

static void rebuild_record(void) {
    char *ofs = awk_special_str(&awk_v_OFS);
    size_t ofl = awk_str_len(ofs), total = 0;
    for (size_t i = 0; i < fields.len; i++)
        total += awk_str_len(fields.v[i]) + (i ? ofl : 0);
    char *s = awk_str_alloc(total);
    char *w = s;
    for (size_t i = 0; i < fields.len; i++) {
        if (i) { memcpy(w, ofs, ofl); w += ofl; }
        size_t l = awk_str_len(fields.v[i]);
        memcpy(w, fields.v[i], l); w += l;
    }
    awk_str_release(ofs);
    awk_str_release(rec);
    rec = s;
    rec_dirty = false;
}

/* Install a new $0 (takes over the caller's reference to `line`) and split it. */
static void set_record(char *line) {
    awk_str_release(rec);
    rec = line;
    rec_dirty = false;
    split_record();
}

char *awk_record_str(void) {
    if (rec_dirty) rebuild_record();
    if (!rec) rec = awk_str_alloc(0);   /* $0 before the first record */
    return rec;
}

/* Field index, with the checks every caller shares. An index is a position in
 * the field vector, so it is a `size_t`; the ceiling is a policy one rather than
 * the type's, since a `size_t` would take any of these but growing the vector to
 * one is an allocation crawl no program could reach the end of. INT_MAX is
 * exact as a double, so the test is exact. NaN fails it, both of its
 * comparisons being false. */
static size_t field_index(AwkValue idx) {
    double d = awk_to_num(idx);
    if (d < 0) awk_fatal("negative field index");
    if (!(d <= INT_MAX)) awk_fatal("field index out of range");
    return (size_t)d;
}

/* $0 and the fields are handed out by sharing, not copying — that is the whole
 * point of refcounting them, since a field read is the runtime's hottest one. */
AwkValue awk_field_get(AwkValue idx) {
    size_t i = field_index(idx);
    if (i == 0) return awk_strnum_own(awk_str_retain(awk_record_str()));
    if (i > fields.len) return awk_uninit();
    return awk_strnum_own(awk_str_retain(fields.v[i - 1]));
}

void awk_field_set(AwkValue idx, AwkValue v) {
    size_t i = field_index(idx);
    if (i == 0) {
        set_record(awk_to_str(v));
        return;
    }
    if (i > fields.len) {
        while (fields.len < i) awk_vec_push(&fields, "", 0);
        set_nf_var();
    }
    awk_str_release(fields.v[i - 1]);
    fields.v[i - 1] = awk_to_str(v);
    rec_dirty = true;
}

void awk_set_nf(AwkValue v) {
    double d = awk_to_num(v);
    if (!(d <= INT_MAX)) awk_fatal("NF out of range");   /* NaN too */
    size_t n = d < 0 ? 0 : (size_t)d;
    while (fields.len > n) awk_str_release(fields.v[--fields.len]);
    while (fields.len < n) awk_vec_push(&fields, "", 0);
    set_nf_var();
    rec_dirty = true;
}

/* ---- Record reading ---------------------------------------------- */

static char  *rbuf;
static size_t rbuf_cap;

/* Read one record from f by RS; returns owned string (delim stripped) or NULL. */
static char *read_record(FILE *f) {
    char delim = rs_char();
    if (delim != '\0') {
        ssize_t len = getdelim(&rbuf, &rbuf_cap, delim, f);
        if (len < 0) return NULL;
        if (len > 0 && rbuf[len - 1] == delim) len--;
        return awk_str_new(rbuf, (size_t)len);
    }
    /* paragraph mode: records separated by blank lines */
    int c;
    while ((c = getc(f)) == '\n') ;   /* skip leading blank lines */
    if (c == EOF) return NULL;
    ungetc(c, f);
    Buf out = { 0 };
    int nlcount = 0;
    while ((c = getc(f)) != EOF) {
        if (c == '\n') {
            if (++nlcount == 2) break;   /* blank line ends record */
        } else {
            while (nlcount--) bputc(&out, '\n');   /* embedded blank lines kept */
            nlcount = 0;
            bputc(&out, (char)c);
        }
    }
    return bfinish(&out);
}

/* ---- Input loop -------------------------------------------------- */

static void bump_nr(void) {
    awk_store(&awk_v_NR, awk_num(awk_to_num(awk_dup(&awk_v_NR)) + 1));
}

bool awk_next_record(void) {
    char *line = read_record(stdin);
    if (!line) return false;
    set_record(line);
    bump_nr();
    return true;
}

/* getline: the bare form is the main loop's own read (`awk_next_record`, which
 * the codegen calls directly); `getline var` reads into *out instead, leaving
 * $0 and NF alone. Both advance the same input stream and update NR. */
bool awk_getline_into(AwkValue *out) {
    char *line = read_record(stdin);
    if (!line) return false;
    bump_nr();
    *out = awk_strnum_own(line);
    return true;
}
