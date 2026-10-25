/* ast.c — AST cleanup and S-expression rendering. */
#include "ast.h"
#include "alloc.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char *const kNodeNames[N__COUNT] = {
    "literal", "symbol", "array", "bytearray", "dict",
    "ident", "self", "unary", "binary", "logical", "ternary",
    "assign", "call", "index", "member", "new", "instanceof", "has",
    "expr-stmt", "var", "block", "if", "while", "dowhile", "for",
    "switch", "return", "break", "continue", "throw", "try",
    "function", "class", "module", "const", "enum", "using",
    "program",
};

const char *node_type_name(NodeType t) {
    if ((unsigned)t >= N__COUNT) return "?";
    return kNodeNames[t];
}

void ast_free(Ast *ast) {
    if (!ast) return;
    for (int i = 0; i < ast->alloc_count; i++) vm_free(ast->allocs[i]);
    if (ast->allocs) vm_free(ast->allocs);
    vm_free(ast);
}

/* ------------------------------------------------ string-builder writer ---- */

typedef struct { char *buf; size_t len, cap; bool grow; } SB;

static void sb_ensure(SB *sb, size_t extra) {
    if (sb->len + extra + 1 <= sb->cap) return;
    if (!sb->grow) return; /* fixed buffer: caller truncates */
    size_t ncap = sb->cap < 64 ? 64 : sb->cap;
    while (sb->len + extra + 1 > ncap) ncap *= 2;
    sb->buf = (char *)vm_realloc(sb->buf, ncap);
    sb->cap = ncap;
}

static void sb_putc(SB *sb, char c) {
    sb_ensure(sb, 1);
    if (sb->len + 1 < sb->cap) sb->buf[sb->len++] = c;
}

static void sb_puts(SB *sb, const char *s) {
    size_t n = strlen(s);
    sb_ensure(sb, n);
    for (size_t i = 0; i < n && sb->len + 1 < sb->cap; i++) sb->buf[sb->len++] = s[i];
}

static void sb_printf(SB *sb, const char *fmt, ...) {
    char tmp[64];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    sb_puts(sb, tmp);
}

/* Emit a string literal with C-style escaping for stable, readable output. */
static void sb_put_escaped(SB *sb, const char *s, int len) {
    sb_putc(sb, '"');
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
        case '"':  sb_puts(sb, "\\\""); break;
        case '\\': sb_puts(sb, "\\\\"); break;
        case '\n': sb_puts(sb, "\\n");  break;
        case '\t': sb_puts(sb, "\\t");  break;
        case '\r': sb_puts(sb, "\\r");  break;
        default:
            if (c < 32 || c >= 127) sb_printf(sb, "\\x%02x", c);
            else sb_putc(sb, (char)c);
        }
    }
    sb_putc(sb, '"');
}

/* ----------------------------------------------------------- emit tree ----- */

static void emit(const Node *n, SB *sb);

static void emit_name(SB *sb, const char *name, int len) {
    sb_ensure(sb, (size_t)len);
    for (int i = 0; i < len && sb->len + 1 < sb->cap; i++) sb->buf[sb->len++] = name[i];
}

/* Emit a child or `_` when the child pointer is NULL (optional slots). */
static void emit_opt(const Node *n, SB *sb) {
    if (n) emit(n, sb);
    else sb_putc(sb, '_');
}

static void emit(const Node *n, SB *sb) {
    if (!n) { sb_putc(sb, '_'); return; }
    switch (n->type) {
    case N_LITERAL:
        switch (n->as.lit.kind) {
        case TOK_NUMBER: sb_printf(sb, "%lld", (long long)n->as.lit.i); break;
        case TOK_LONG:   sb_printf(sb, "%lldL", (long long)n->as.lit.i); break;
        case TOK_FLOAT:  sb_printf(sb, "%gf", (double)n->as.lit.f); break;
        case TOK_DOUBLE: sb_printf(sb, "%gd", n->as.lit.d); break;
        case TOK_CHAR:   sb_printf(sb, "#%u", n->as.lit.ch); break;
        case TOK_TRUE:   sb_puts(sb, "true"); break;
        case TOK_FALSE:  sb_puts(sb, "false"); break;
        case TOK_NULL:   sb_puts(sb, "null"); break;
        case TOK_STRING: sb_put_escaped(sb, n->as.lit.str, n->as.lit.slen); break;
        default:         sb_puts(sb, "?lit"); break;
        }
        break;
    case N_SYMBOL:
        sb_putc(sb, ':'); emit_name(sb, n->as.name.name, n->as.name.len); break;
    case N_IDENT:
        emit_name(sb, n->as.name.name, n->as.name.len); break;
    case N_SELF:
        sb_puts(sb, "self"); break;
    case N_ARRAY:
        sb_puts(sb, "(array");
        for (int i = 0; i < n->as.list.count; i++) { sb_putc(sb, ' '); emit(n->as.list.items[i], sb); }
        sb_putc(sb, ')'); break;
    case N_BYTEARRAY:
        sb_puts(sb, "(bytearray");
        for (int i = 0; i < n->as.list.count; i++) { sb_putc(sb, ' '); emit(n->as.list.items[i], sb); }
        sb_putc(sb, ')'); break;
    case N_DICT:
        sb_puts(sb, "(dict");
        for (int i = 0; i < n->as.dict.count; i++) {
            sb_puts(sb, " ("); emit(n->as.dict.keys[i], sb);
            sb_putc(sb, ' '); emit(n->as.dict.vals[i], sb); sb_putc(sb, ')');
        }
        sb_putc(sb, ')'); break;
    case N_UNARY:
        sb_putc(sb, '(');
        if (n->as.unary.postfix) sb_puts(sb, "post");
        sb_puts(sb, token_type_name(n->as.unary.op));
        sb_putc(sb, ' '); emit(n->as.unary.operand, sb); sb_putc(sb, ')'); break;
    case N_BINARY:
    case N_LOGICAL:
        sb_putc(sb, '('); sb_puts(sb, token_type_name(n->as.binary.op));
        sb_putc(sb, ' '); emit(n->as.binary.left, sb);
        sb_putc(sb, ' '); emit(n->as.binary.right, sb); sb_putc(sb, ')'); break;
    case N_TERNARY:
        sb_puts(sb, "(?: "); emit(n->as.ternary.cond, sb);
        sb_putc(sb, ' '); emit(n->as.ternary.then_e, sb);
        sb_putc(sb, ' '); emit(n->as.ternary.else_e, sb); sb_putc(sb, ')'); break;
    case N_ASSIGN:
        sb_putc(sb, '('); sb_puts(sb, token_type_name(n->as.assign.op));
        sb_putc(sb, ' '); emit(n->as.assign.target, sb);
        sb_putc(sb, ' '); emit(n->as.assign.value, sb); sb_putc(sb, ')'); break;
    case N_CALL:
        sb_puts(sb, "(call "); emit(n->as.call.callee, sb);
        for (int i = 0; i < n->as.call.argc; i++) { sb_putc(sb, ' '); emit(n->as.call.args[i], sb); }
        sb_putc(sb, ')'); break;
    case N_INDEX:
        sb_puts(sb, "(index "); emit(n->as.index.object, sb);
        sb_putc(sb, ' '); emit(n->as.index.index, sb); sb_putc(sb, ')'); break;
    case N_MEMBER:
        sb_puts(sb, "(. "); emit(n->as.member.object, sb);
        sb_putc(sb, ' '); emit_name(sb, n->as.member.name, n->as.member.len);
        sb_putc(sb, ')'); break;
    case N_NEW:
        sb_puts(sb, "(new "); emit(n->as.newexpr.cls, sb);
        for (int i = 0; i < n->as.newexpr.argc; i++) { sb_putc(sb, ' '); emit(n->as.newexpr.args[i], sb); }
        sb_putc(sb, ')'); break;
    case N_INSTANCEOF:
        sb_puts(sb, "(instanceof "); emit(n->as.instof.expr, sb);
        sb_putc(sb, ' '); emit(n->as.instof.type, sb); sb_putc(sb, ')'); break;
    case N_HAS:
        sb_puts(sb, "(has "); emit(n->as.has.expr, sb);
        sb_putc(sb, ' '); emit(n->as.has.sym, sb); sb_putc(sb, ')'); break;
    case N_EXPR_STMT:
        emit(n->as.expr.expr, sb); break;
    case N_VAR:
        sb_puts(sb, "(var "); emit_name(sb, n->as.var.name, n->as.var.len);
        if (n->as.var.init) { sb_putc(sb, ' '); emit(n->as.var.init, sb); }
        sb_putc(sb, ')'); break;
    case N_BLOCK:
        sb_puts(sb, "(block");
        for (int i = 0; i < n->as.list.count; i++) { sb_putc(sb, ' '); emit(n->as.list.items[i], sb); }
        sb_putc(sb, ')'); break;
    case N_IF:
        sb_puts(sb, "(if "); emit(n->as.ifs.cond, sb);
        sb_putc(sb, ' '); emit(n->as.ifs.then_s, sb);
        if (n->as.ifs.else_s) { sb_putc(sb, ' '); emit(n->as.ifs.else_s, sb); }
        sb_putc(sb, ')'); break;
    case N_WHILE:
        sb_puts(sb, "(while "); emit(n->as.loop.cond, sb);
        sb_putc(sb, ' '); emit(n->as.loop.body, sb); sb_putc(sb, ')'); break;
    case N_DOWHILE:
        sb_puts(sb, "(dowhile "); emit(n->as.loop.body, sb);
        sb_putc(sb, ' '); emit(n->as.loop.cond, sb); sb_putc(sb, ')'); break;
    case N_FOR:
        sb_puts(sb, "(for "); emit_opt(n->as.forl.init, sb);
        sb_putc(sb, ' '); emit_opt(n->as.forl.cond, sb);
        sb_putc(sb, ' '); emit_opt(n->as.forl.update, sb);
        sb_putc(sb, ' '); emit(n->as.forl.body, sb); sb_putc(sb, ')'); break;
    case N_SWITCH:
        sb_puts(sb, "(switch "); emit(n->as.sw.expr, sb);
        for (int i = 0; i < n->as.sw.count; i++) {
            if (n->as.sw.case_vals[i]) { sb_puts(sb, " (case "); emit(n->as.sw.case_vals[i], sb); }
            else sb_puts(sb, " (default");
            sb_putc(sb, ' '); emit(n->as.sw.case_bodies[i], sb); sb_putc(sb, ')');
        }
        sb_putc(sb, ')'); break;
    case N_RETURN:
        if (n->as.expr.expr) { sb_puts(sb, "(return "); emit(n->as.expr.expr, sb); sb_putc(sb, ')'); }
        else sb_puts(sb, "(return)");
        break;
    case N_BREAK:    sb_puts(sb, "(break)"); break;
    case N_CONTINUE: sb_puts(sb, "(continue)"); break;
    case N_THROW:
        sb_puts(sb, "(throw "); emit(n->as.expr.expr, sb); sb_putc(sb, ')'); break;
    case N_TRY:
        sb_puts(sb, "(try "); emit(n->as.tryc.block, sb);
        for (int i = 0; i < n->as.tryc.catch_count; i++) {
            sb_puts(sb, " (catch ");
            if (n->as.tryc.catch_names[i]) emit_name(sb, n->as.tryc.catch_names[i], n->as.tryc.catch_name_lens[i]);
            else sb_putc(sb, '_');
            sb_putc(sb, ' '); emit_opt(n->as.tryc.catch_types[i], sb);
            sb_putc(sb, ' '); emit(n->as.tryc.catch_bodies[i], sb); sb_putc(sb, ')');
        }
        if (n->as.tryc.finally_block) { sb_puts(sb, " (finally "); emit(n->as.tryc.finally_block, sb); sb_putc(sb, ')'); }
        sb_putc(sb, ')'); break;
    case N_FUNCTION:
        sb_puts(sb, "(function "); emit_name(sb, n->as.func.name, n->as.func.len);
        sb_puts(sb, " (params");
        for (int i = 0; i < n->as.func.param_count; i++) { sb_putc(sb, ' '); emit_name(sb, n->as.func.params[i], n->as.func.param_lens[i]); }
        sb_puts(sb, ") "); emit(n->as.func.body, sb); sb_putc(sb, ')'); break;
    case N_CLASS:
        sb_puts(sb, "(class "); emit_name(sb, n->as.cls.name, n->as.cls.len);
        if (n->as.cls.super) { sb_puts(sb, " extends "); emit(n->as.cls.super, sb); }
        for (int i = 0; i < n->as.cls.count; i++) { sb_putc(sb, ' '); emit(n->as.cls.members[i], sb); }
        sb_putc(sb, ')'); break;
    case N_MODULE:
        sb_puts(sb, "(module "); emit_name(sb, n->as.mod.name, n->as.mod.len);
        for (int i = 0; i < n->as.mod.count; i++) { sb_putc(sb, ' '); emit(n->as.mod.members[i], sb); }
        sb_putc(sb, ')'); break;
    case N_CONST:
        sb_puts(sb, "(const "); emit_name(sb, n->as.var.name, n->as.var.len);
        sb_putc(sb, ' '); emit(n->as.var.init, sb); sb_putc(sb, ')'); break;
    case N_ENUM:
        sb_puts(sb, "(enum");
        for (int i = 0; i < n->as.enm.count; i++) {
            sb_puts(sb, " ("); emit_name(sb, n->as.enm.names[i], n->as.enm.name_lens[i]);
            sb_putc(sb, ' '); emit_opt(n->as.enm.values[i], sb); sb_putc(sb, ')');
        }
        sb_putc(sb, ')'); break;
    case N_USING:
        sb_puts(sb, "(using ");
        for (int i = 0; i < n->as.use.path_count; i++) {
            if (i) sb_putc(sb, '.');
            emit_name(sb, n->as.use.path[i], n->as.use.path_lens[i]);
        }
        if (n->as.use.alias) { sb_puts(sb, " as "); emit_name(sb, n->as.use.alias, n->as.use.alias_len); }
        sb_putc(sb, ')'); break;
    case N_PROGRAM:
        sb_puts(sb, "(program");
        for (int i = 0; i < n->as.list.count; i++) { sb_putc(sb, ' '); emit(n->as.list.items[i], sb); }
        sb_putc(sb, ')'); break;
    default:
        sb_puts(sb, "?node"); break;
    }
}

size_t ast_snprint(const Node *n, char *buf, size_t cap) {
    SB sb = { buf, 0, cap, false };
    if (cap == 0) return 0;
    emit(n, &sb);
    buf[sb.len] = '\0';
    return sb.len;
}

void ast_dump(const Node *n, FILE *out) {
    SB sb = { NULL, 0, 0, true };
    emit(n, &sb);
    if (sb.buf) { sb.buf[sb.len] = '\0'; fputs(sb.buf, out); vm_free(sb.buf); }
    fputc('\n', out);
}
