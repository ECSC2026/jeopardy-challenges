/* awk2c runtime support library — public interface.
 *
 * Memory model (linear / consume-produce): every AwkValue rvalue is owned by
 * exactly one holder. Operation helpers CONSUME their AwkValue arguments
 * (releasing any string reference) and RETURN a freshly-owned AwkValue. Reading
 * a variable yields an owned value (awk_dup). This makes each expression
 * temporary consumed exactly once by its parent, so no explicit frees are
 * needed in generated code except awk_free on a discarded statement result.
 *
 * How an argument is passed is what says which of the two it is:
 *   AwkValue    — CONSUMED. The callee owns it and releases it.
 *   AwkValue *  — BORROWED for the duration of the call. The callee only reads
 *                 it; the caller keeps ownership. `const` wherever C allows.
 * Two shapes cannot follow that, and say so at their own declaration: an
 * *lvalue slot* (awk_store, awk_getline_into) is written through rather than
 * read, and a *vector* (AwkValue *args / *subs) is a pointer only because C
 * cannot pass an array by value — its elements are consumed like by-value
 * arguments.
 *
 * Ownership of a *string* is a reference count, so "owned" costs a counter bump
 * rather than a copy: awk_dup, a field read and an array read all hand out the
 * same buffer. Strings are therefore immutable once built.
 * A string is a heap block whose {refs,len} header sits immediately *before*
 * the characters, so a value still passes a plain NUL-terminated `char *`
 * around, and the same rule applies: `char *` is an owned reference, `const
 * char *` a borrowed one. Generated code needs only the two entry points below
 * (never free(), and never write through one). */
#ifndef AWK_RUNTIME_H
#define AWK_RUNTIME_H

#include <stddef.h>   /* size_t; `bool` is a keyword in C23, which this is */

void  awk_str_release(char *s);  /* -1 reference; frees at zero. NULL-safe. */
char *awk_str_retain(char *s);   /* +1 reference; returns s. NULL-safe. */

/* ------------------------------------------------------------------ */
/* Value model: a 4-state tagged scalar.                              */
/* ------------------------------------------------------------------ */
typedef enum { AWK_UNINIT, AWK_NUM, AWK_STR, AWK_STRNUM } AwkKind;

typedef struct {
    AwkKind kind;
    double  num;
    char   *str;   /* one owned reference when non-NULL */
} AwkValue;

AwkValue awk_uninit(void);
AwkValue awk_num(double n);
AwkValue awk_str(const char *s);       /* AWK_STR (copies) */
AwkValue awk_str_own(char *s);         /* AWK_STR wrapping an owned ref, no copy */
AwkValue awk_dup(const AwkValue *v);   /* owned copy: shares the string */
void     awk_free(AwkValue v);         /* discard: release its string reference */

/* Conversions out of a value, named for what they produce. Each follows the
 * convention above and CONSUMES its argument. */
double awk_to_num(AwkValue v);
bool   awk_to_bool(AwkValue v);
/* The string form, as an owned reference the caller awk_str_releases. Whenever
 * the value holds a string that *is* the reference returned, so nothing is
 * copied and no counter moves — which is why there is deliberately no `_ref`
 * sibling: it would have to retain a reference that the release a few lines
 * later only undoes. */
char  *awk_to_str(AwkValue v);

/* Store into an lvalue slot: releases the old value, takes ownership of v. */
void awk_store(AwkValue *slot, AwkValue v);

/* Arithmetic / string operators — each consumes its args. */
AwkValue awk_add(AwkValue a, AwkValue b);
AwkValue awk_sub(AwkValue a, AwkValue b);
AwkValue awk_mul(AwkValue a, AwkValue b);
AwkValue awk_div(AwkValue a, AwkValue b);
AwkValue awk_mod(AwkValue a, AwkValue b);
AwkValue awk_pow(AwkValue a, AwkValue b);
AwkValue awk_concat(AwkValue a, AwkValue b);

/* Three-way comparison under the POSIX strnum rules (<0, 0, >0). Every AWK
 * comparison lowers to this against 0, so there is no per-operator entry point.
 * Consumes its args. */
int awk_cmp(AwkValue a, AwkValue b);

/* ------------------------------------------------------------------ */
/* Associative arrays. Subscript vectors are consumed by the callee.  */
/* ------------------------------------------------------------------ */
/* A key/value pair. Exposed so `for (k in a)` can walk the entry vector inline
 * from generated code. */
typedef struct AwkEntry { char *key; AwkValue val; } AwkEntry;
/* A grown-by-doubling vector of entries, scanned linearly. Zero-initialized =
 * empty, so a declaration needs no constructor call. */
typedef struct AwkArray { AwkEntry *ents; size_t len, cap; } AwkArray;

AwkValue awk_array_get(AwkArray *a, AwkValue *subs, size_t n); /* creates if absent */
void     awk_array_set(AwkArray *a, AwkValue *subs, size_t n, AwkValue v);
bool     awk_array_in(AwkArray *a, AwkValue *subs, size_t n);  /* no create */
void     awk_array_del(AwkArray *a, AwkValue *subs, size_t n);
void     awk_array_clear(AwkArray *a);
void     awk_array_free(AwkArray *a);   /* clear + release the vector itself */
AwkValue awk_array_length(AwkArray *a); /* element count */

/* ------------------------------------------------------------------ */
/* Fields / records. Index values are consumed.                       */
/* ------------------------------------------------------------------ */
AwkValue awk_field_get(AwkValue idx);
void     awk_field_set(AwkValue idx, AwkValue v);
void     awk_set_nf(AwkValue v);       /* assigning NF rebuilds $0 */

/* ------------------------------------------------------------------ */
/* Special variables (globals; generated code reads via awk_dup).     */
/* ------------------------------------------------------------------ */
extern AwkValue awk_v_NR, awk_v_NF, awk_v_FS, awk_v_OFS, awk_v_ORS,
                awk_v_RS, awk_v_SUBSEP, awk_v_CONVFMT, awk_v_OFMT,
                awk_v_RSTART, awk_v_RLENGTH;

/* ------------------------------------------------------------------ */
/* Built-in functions. All consume their AwkValue arguments.          */
/* ------------------------------------------------------------------ */
void     awk_print(AwkValue *args, size_t n);
void     awk_printf(AwkValue *args, size_t n);
AwkValue awk_sprintf(AwkValue *args, size_t n);
AwkValue awk_str_length(AwkValue v);          /* strlen of string form */
AwkValue awk_substr(AwkValue s, AwkValue m, AwkValue n, bool have_n);
AwkValue awk_index(AwkValue s, AwkValue t);
AwkValue awk_toupper(AwkValue s);
AwkValue awk_tolower(AwkValue s);
double   awk_rand(void);
double   awk_srand(double seed, bool have_seed);

/* Regex + splitting (awk_regex.c). AwkValue patterns are consumed. */
bool     awk_regex_match(AwkValue s, AwkValue pat);     /* consumes both */
AwkValue awk_match(AwkValue s, AwkValue pat);           /* consumes both; sets RSTART/RLENGTH */
size_t   awk_split(AwkValue s, AwkArray *arr, const char *fs); /* fs NULL => FS */
/* sub/gsub: consumes pat+repl+target, returns the new value, writes count. */
AwkValue awk_subst(AwkValue pat, AwkValue repl, AwkValue target, bool global, size_t *count);

/* getline (main input stream only). Bare `getline` is the main loop's own read,
 * `awk_next_record` below, which the codegen calls directly. */
bool awk_getline_into(AwkValue *out);/* into *out; updates NR; read a record? */

/* ------------------------------------------------------------------ */
/* Lifecycle / input loop.                                            */
/* ------------------------------------------------------------------ */
void awk_runtime_start(void);
bool awk_next_record(void);   /* read the next record from stdin; false at EOF */
void awk_exit(double status); /* record requested exit status */
void awk_register_end(void (*f)(void)); /* generated END rules */
void awk_run_end(void);       /* run END rules, at most once */
_Noreturn void awk_exit_now(void); /* `exit` inside a function: END, then quit */
int  awk_runtime_end(void);   /* flush + return exit status */

/* Fatal error: print to stderr and exit(2). */
_Noreturn void awk_fatal(const char *msg);

#endif /* AWK_RUNTIME_H */
