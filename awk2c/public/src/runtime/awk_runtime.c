/* awk2c runtime — lifecycle and special-variable init. */
#include "awk_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Exit handling ----------------------------------------------- */

static int g_exit_code;
void awk_exit(double status) { g_exit_code = awk_num_to_int(status); }

/* The generated END rules, registered by `main` so that an `exit` reached from
 * inside a user-defined function can still run them: there is no label to jump
 * to from another C function, so the exit path calls END here instead. */
static void (*g_end_rules)(void);
void awk_register_end(void (*f)(void)) { g_end_rules = f; }

void awk_run_end(void) {
    void (*f)(void) = g_end_rules;
    g_end_rules = NULL; /* END runs at most once, however it is reached */
    if (f) f();
}

_Noreturn void awk_exit_now(void) {
    awk_run_end();
    exit(awk_runtime_end());
}

int awk_runtime_end(void) {
    fflush(stdout);
    return g_exit_code;
}

/* ---- Startup ----------------------------------------------------- */

void awk_runtime_start(void) {
    awk_v_FS      = awk_str(" ");
    awk_v_OFS     = awk_str(" ");
    awk_v_ORS     = awk_str("\n");
    awk_v_RS      = awk_str("\n");
    awk_v_SUBSEP  = awk_str("\034");
    awk_v_CONVFMT = awk_str("%.6g");
    awk_v_OFMT    = awk_str("%.6g");
    awk_v_NR      = awk_num(0);
    awk_v_NF      = awk_num(0);
    awk_v_RSTART  = awk_num(0);
    awk_v_RLENGTH = awk_num(-1);
}
