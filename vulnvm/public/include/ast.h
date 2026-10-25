/* ast.h — MonkeyC abstract syntax tree.
 *
 * Nodes are a tagged union. Every node, child-pointer array, and copied string is
 * allocated from the custom arena and recorded in the owning Ast's `allocs` list,
 * so the whole tree (and its strings) is freed in one pass by ast_free — no
 * recursive destructor needed. Strings are copied into Ast-owned storage, so the
 * tree is self-contained and does not depend on the source buffer or TokenList
 * outliving it.
 */
#ifndef VULNVM_AST_H
#define VULNVM_AST_H

#include <stdio.h>
#include "token.h"

/* Member access / declaration modifiers (bitmask). */
enum {
    MOD_PUBLIC    = 1 << 0,
    MOD_PRIVATE   = 1 << 1,
    MOD_PROTECTED = 1 << 2,
    MOD_HIDDEN    = 1 << 3,
    MOD_STATIC    = 1 << 4,
    MOD_NATIVE    = 1 << 5,
    MOD_CONST     = 1 << 6,
};

typedef enum {
    /* expressions */
    N_LITERAL, N_SYMBOL, N_ARRAY, N_BYTEARRAY, N_DICT,
    N_IDENT, N_SELF, N_UNARY, N_BINARY, N_LOGICAL, N_TERNARY,
    N_ASSIGN, N_CALL, N_INDEX, N_MEMBER, N_NEW, N_INSTANCEOF, N_HAS,
    /* statements */
    N_EXPR_STMT, N_VAR, N_BLOCK, N_IF, N_WHILE, N_DOWHILE, N_FOR,
    N_SWITCH, N_RETURN, N_BREAK, N_CONTINUE, N_THROW, N_TRY,
    /* declarations */
    N_FUNCTION, N_CLASS, N_MODULE, N_CONST, N_ENUM, N_USING,
    /* top level */
    N_PROGRAM,
    N__COUNT
} NodeType;

typedef struct Node Node;

struct Node {
    NodeType type;
    int line, col;
    union {
        /* N_LITERAL — kind is the source TokenType (TOK_NUMBER/LONG/FLOAT/DOUBLE/
         * CHAR/STRING/TRUE/FALSE/NULL). */
        struct { TokenType kind; int64_t i; double d; float f; uint32_t ch;
                 char *str; int slen; } lit;

        /* N_IDENT, N_SYMBOL — a copied name. */
        struct { char *name; int len; } name;

        /* N_ARRAY, N_BYTEARRAY, N_BLOCK, N_PROGRAM — a child list. */
        struct { Node **items; int count; } list;

        /* N_DICT — parallel key/value lists. */
        struct { Node **keys; Node **vals; int count; } dict;

        /* N_UNARY — prefix op. */
        struct { TokenType op; Node *operand; bool postfix; } unary;

        /* N_BINARY, N_LOGICAL. */
        struct { TokenType op; Node *left; Node *right; } binary;

        /* N_TERNARY. */
        struct { Node *cond; Node *then_e; Node *else_e; } ternary;

        /* N_ASSIGN — op is TOK_ASSIGN or a compound assign token. */
        struct { TokenType op; Node *target; Node *value; } assign;

        /* N_CALL. */
        struct { Node *callee; Node **args; int argc; } call;

        /* N_INDEX. */
        struct { Node *object; Node *index; } index;

        /* N_MEMBER — object.name. */
        struct { Node *object; char *name; int len; } member;

        /* N_NEW — new cls(args). */
        struct { Node *cls; Node **args; int argc; } newexpr;

        /* N_INSTANCEOF — expr instanceof type. */
        struct { Node *expr; Node *type; } instof;

        /* N_HAS — expr has :sym. */
        struct { Node *expr; Node *sym; } has;

        /* N_EXPR_STMT, N_RETURN, N_THROW — a single expression (may be NULL). */
        struct { Node *expr; } expr;

        /* N_VAR, N_CONST — name = init (init may be NULL for var). */
        struct { char *name; int len; Node *init; unsigned modifiers; } var;

        /* N_IF. */
        struct { Node *cond; Node *then_s; Node *else_s; } ifs;

        /* N_WHILE, N_DOWHILE. */
        struct { Node *cond; Node *body; } loop;

        /* N_FOR — any of init/cond/update may be NULL. */
        struct { Node *init; Node *cond; Node *update; Node *body; } forl;

        /* N_SWITCH — case_vals[i] == NULL marks the default clause; case_bodies[i]
         * is an N_BLOCK. Clause order is preserved for fallthrough. */
        struct { Node *expr; Node **case_vals; Node **case_bodies; int count; } sw;

        /* N_TRY — catch_types[i] NULL means catch-all; catch_names[i] is the bound
         * exception variable (may be NULL). */
        struct { Node *block; char **catch_names; int *catch_name_lens;
                 Node **catch_types; Node **catch_bodies; int catch_count;
                 Node *finally_block; } tryc;

        /* N_FUNCTION. */
        struct { char *name; int len; char **params; int *param_lens;
                 int param_count; Node *body; unsigned modifiers; } func;

        /* N_CLASS — super may be NULL. */
        struct { char *name; int len; Node *super; Node **members; int count;
                 unsigned modifiers; } cls;

        /* N_MODULE. */
        struct { char *name; int len; Node **members; int count; } mod;

        /* N_ENUM — values[i] NULL means auto-increment. */
        struct { char **names; int *name_lens; Node **values; int count; } enm;

        /* N_USING — dotted path, optional alias. */
        struct { char **path; int *path_lens; int path_count;
                 char *alias; int alias_len; } use;
    } as;
};

/* The owning container: the parsed tree plus bookkeeping for one-pass cleanup and
 * error reporting. */
typedef struct {
    Node *root;              /* N_PROGRAM (may be partial if had_error) */
    bool  had_error;
    char  error_msg[256];
    int   error_line, error_col;

    void **allocs;           /* every arena allocation owned by this tree */
    int    alloc_count, alloc_cap;
} Ast;

/* Free the whole tree and everything it owns. */
void ast_free(Ast *ast);

/* Write an S-expression rendering of a node (used by `--ast` and tests). */
void ast_dump(const Node *n, FILE *out);

/* Render into a caller buffer (always NUL-terminated); returns bytes written. */
size_t ast_snprint(const Node *n, char *buf, size_t cap);

const char *node_type_name(NodeType t);

#endif /* VULNVM_AST_H */
