/* compiler.c — MonkeyC AST -> VulnVM bytecode.
 *
 * A stack machine: expressions push their result; statements are stack-neutral.
 * Top-level declarations bind globals (by name); variables inside a function are
 * locals addressed by stack slot. Control flow uses forward jumps (patched once
 * the target is known) and backward LOOPs. Since our grammar only allows function
 * declarations at the top level (never nested in a block), no closures/upvalues
 * are needed here — nested scopes only introduce locals.
 */
#include "compiler.h"
#include "alloc.h"

#include <stdio.h>
#include <string.h>

typedef struct { const char *name; int len; int depth; } Local;

typedef struct Compiler {
    struct Compiler *enclosing;
    ObjFunction     *function;
    Local           *locals;
    int              local_count, local_cap;
    int              local_max;      /* high-water local count (for frame sizing) */
    int              scope_depth;
    int              try_depth;      /* active try handlers, for break/return cleanup */
    bool             has_try;        /* function contains a try block */
    bool             in_initializer; /* this function is a class initialize() */
} Compiler;

/* Names of the members (fields + methods) of the class whose instance method is
 * being compiled, so unqualified references resolve to self.<member>. Names are
 * borrowed from the (still-alive) AST. */
typedef struct {
    const char **names;
    const int   *lens;
    int          count;
} ClassCtx;

/* One entry per enclosing loop, for break/continue. */
typedef struct LoopCtx {
    struct LoopCtx *prev;
    int  continue_target;   /* absolute offset to LOOP back to on `continue` */
    int  base_depth;        /* scope depth outside the loop body             */
    int  try_depth;         /* try_depth outside the loop, for break/continue */
    int *breaks;            /* offsets of forward break jumps to patch       */
    int  break_count, break_cap;
} LoopCtx;

typedef struct {
    Compiler *current;
    LoopCtx  *loop;
    ClassCtx *klass;        /* member set for the instance method being compiled */
    Node     *program;      /* the N_PROGRAM root, for resolving inherited members */
    bool      had_error;
    char      error_msg[256];
    int       error_line;
    int       line;         /* current source line, for chunk_write */
} Comp;

/* ------------------------------------------------------------ diagnostics -- */

static void cerror(Comp *c, const Node *n, const char *msg) {
    if (c->had_error) return;
    c->had_error = true;
    c->error_line = n ? n->line : 0;
    snprintf(c->error_msg, sizeof(c->error_msg), "%s (line %d)", msg, n ? n->line : 0);
}

/* ------------------------------------------------------------- emit prims -- */

static Chunk *cur_chunk(Comp *c) { return c->current->function->chunk; }
static void emit(Comp *c, uint8_t b)       { chunk_write(cur_chunk(c), b, c->line); }
static void emit_op(Comp *c, OpCode op)    { emit(c, (uint8_t)op); }
static void emit16(Comp *c, uint16_t v)    { chunk_write16(cur_chunk(c), v, c->line); }

static int make_const(Comp *c, Value v) {
    int idx = chunk_add_const(cur_chunk(c), v);
    if (idx > 0xFFFF) { cerror(c, NULL, "too many constants in one chunk"); return 0; }
    return idx;
}
static void emit_const(Comp *c, Value v) { emit_op(c, OP_CONSTANT); emit16(c, (uint16_t)make_const(c, v)); }

static int const_string(Comp *c, const char *s, int len) {
    ObjString *o = string_new(s, len);
    int idx = make_const(c, mk_obj((Obj *)o));
    release(mk_obj((Obj *)o));
    return idx;
}
static int const_symbol(Comp *c, const char *s, int len) {
    return make_const(c, mk_symbol(symbol_intern(s, len)));
}

/* Jumps: emit a placeholder and return the operand offset to patch later. */
static int emit_jump(Comp *c, OpCode op) {
    emit_op(c, op);
    emit16(c, 0xFFFF);
    return cur_chunk(c)->count - 2;
}
static void patch_jump(Comp *c, int operand_offset) {
    int target = cur_chunk(c)->count;
    int dist = target - (operand_offset + 2);
    if (dist > 0xFFFF) { cerror(c, NULL, "jump too far"); return; }
    cur_chunk(c)->code[operand_offset]     = (uint8_t)((dist >> 8) & 0xFF);
    cur_chunk(c)->code[operand_offset + 1] = (uint8_t)(dist & 0xFF);
}
static void emit_loop(Comp *c, int loop_target) {
    emit_op(c, OP_LOOP);
    int dist = cur_chunk(c)->count + 2 - loop_target;
    if (dist > 0xFFFF) { cerror(c, NULL, "loop body too large"); return; }
    emit16(c, (uint16_t)dist);
}

/* ---------------------------------------------------------------- scopes --- */

static void add_local(Comp *c, const char *name, int len) {
    Compiler *cc = c->current;
    if (cc->local_count + 1 > cc->local_cap) {
        int nc = cc->local_cap < 8 ? 8 : cc->local_cap * 2;
        cc->locals = (Local *)vm_realloc(cc->locals, (size_t)nc * sizeof(Local));
        cc->local_cap = nc;
    }
    if (cc->local_count >= 0xFFFF) { cerror(c, NULL, "too many locals in function"); return; }
    cc->locals[cc->local_count].name = name;
    cc->locals[cc->local_count].len = len;
    cc->locals[cc->local_count].depth = cc->scope_depth;
    cc->local_count++;
    if (cc->local_count > cc->local_max) cc->local_max = cc->local_count;
}
static int resolve_local(Compiler *cc, const char *name, int len) {
    for (int i = cc->local_count - 1; i >= 0; i--)
        if (cc->locals[i].len == len && memcmp(cc->locals[i].name, name, (size_t)len) == 0)
            return i;
    return -1;
}
static bool in_global_scope(Comp *c) {
    return c->current->enclosing == NULL && c->current->scope_depth == 0;
}
static void begin_scope(Comp *c) { c->current->scope_depth++; }
static void end_scope(Comp *c) {
    Compiler *cc = c->current;
    cc->scope_depth--;
    while (cc->local_count > 0 && cc->locals[cc->local_count - 1].depth > cc->scope_depth) {
        emit_op(c, OP_POP);
        cc->local_count--;
    }
}
/* Emit POPs for locals deeper than `depth` WITHOUT removing them (for break/continue). */
static void emit_pops_above(Comp *c, int depth) {
    Compiler *cc = c->current;
    for (int i = cc->local_count - 1; i >= 0 && cc->locals[i].depth > depth; i--)
        emit_op(c, OP_POP);
}

/* ------------------------------------------------------- op token mappings - */

static OpCode binop_opcode(TokenType t) {
    switch (t) {
    case TOK_PLUS: return OP_ADD;   case TOK_MINUS: return OP_SUB;
    case TOK_STAR: return OP_MUL;   case TOK_SLASH: return OP_DIV;
    case TOK_PERCENT: return OP_MOD;
    case TOK_AMP: return OP_BAND;   case TOK_PIPE: return OP_BOR;
    case TOK_CARET: return OP_BXOR; case TOK_SHL: return OP_SHL;
    case TOK_SHR: return OP_SHR;
    case TOK_EQ: return OP_EQ;      case TOK_NEQ: return OP_NEQ;
    case TOK_LT: return OP_LT;      case TOK_GT: return OP_GT;
    case TOK_LE: return OP_LE;      case TOK_GE: return OP_GE;
    default: return OP__COUNT;
    }
}
static OpCode compound_opcode(TokenType t) {
    switch (t) {
    case TOK_PLUS_EQ: return OP_ADD;  case TOK_MINUS_EQ: return OP_SUB;
    case TOK_STAR_EQ: return OP_MUL;  case TOK_SLASH_EQ: return OP_DIV;
    case TOK_PERCENT_EQ: return OP_MOD;
    case TOK_AMP_EQ: return OP_BAND;  case TOK_PIPE_EQ: return OP_BOR;
    case TOK_CARET_EQ: return OP_BXOR;
    case TOK_SHL_EQ: return OP_SHL;   case TOK_SHR_EQ: return OP_SHR;
    default: return OP__COUNT;
    }
}

/* ---------------------------------------------------------- forward decls -- */

static void compile_expr(Comp *c, Node *n);
static void compile_stmt(Comp *c, Node *n);
static void compile_decl(Comp *c, Node *n);

typedef enum { REF_LOCAL, REF_MEMBER, REF_GLOBAL } RefKind;

static bool is_member(ClassCtx *k, const char *name, int len) {
    if (!k) return false;
    for (int i = 0; i < k->count; i++)
        if (k->lens[i] == len && memcmp(k->names[i], name, (size_t)len) == 0) return true;
    return false;
}

/* Classify an unqualified identifier: a local slot, an instance member of the
 * enclosing class (-> self.member), or a global. */
static RefKind resolve_ident(Comp *c, const char *name, int len, int *slot) {
    int s = resolve_local(c->current, name, len);
    if (s >= 0) { *slot = s; return REF_LOCAL; }
    if (is_member(c->klass, name, len)) return REF_MEMBER;
    return REF_GLOBAL;
}

/* Emit a load of an identifier reference. */
static void emit_get_var(Comp *c, const char *name, int len) {
    int slot;
    switch (resolve_ident(c, name, len, &slot)) {
    case REF_LOCAL:
        emit_op(c, OP_GET_LOCAL); emit16(c, (uint16_t)slot);
        break;
    case REF_MEMBER:
        emit_op(c, OP_GET_LOCAL); emit16(c, 0); /* self */
        emit_op(c, OP_GET_PROPERTY); emit16(c, (uint16_t)const_symbol(c, name, len));
        break;
    case REF_GLOBAL:
        emit_op(c, OP_GET_GLOBAL); emit16(c, (uint16_t)const_string(c, name, len));
        break;
    }
}

/* --------------------------------------------------------- assignment ------ */

static void compile_plain_assign(Comp *c, Node *target, Node *value) {
    switch (target->type) {
    case N_IDENT: {
        int slot;
        RefKind k = resolve_ident(c, target->as.name.name, target->as.name.len, &slot);
        if (k == REF_MEMBER) {
            emit_op(c, OP_GET_LOCAL); emit16(c, 0); /* self */
            compile_expr(c, value);
            emit_op(c, OP_SET_PROPERTY);
            emit16(c, (uint16_t)const_symbol(c, target->as.name.name, target->as.name.len));
        } else {
            compile_expr(c, value);
            if (k == REF_LOCAL) { emit_op(c, OP_SET_LOCAL); emit16(c, (uint16_t)slot); }
            else { emit_op(c, OP_SET_GLOBAL); emit16(c, (uint16_t)const_string(c, target->as.name.name, target->as.name.len)); }
        }
        break;
    }
    case N_MEMBER:
        compile_expr(c, target->as.member.object);
        compile_expr(c, value);
        emit_op(c, OP_SET_PROPERTY);
        emit16(c, (uint16_t)const_symbol(c, target->as.member.name, target->as.member.len));
        break;
    case N_INDEX:
        compile_expr(c, target->as.index.object);
        compile_expr(c, target->as.index.index);
        compile_expr(c, value);
        emit_op(c, OP_SET_INDEX);
        break;
    default:
        cerror(c, target, "invalid assignment target");
        break;
    }
}

/* Compound target op= value, or ++/-- (value == NULL -> literal 1, binop ADD/SUB). */
static void compile_compound_assign(Comp *c, Node *target, OpCode binop, Node *value) {
    switch (target->type) {
    case N_IDENT: {
        int slot;
        RefKind k = resolve_ident(c, target->as.name.name, target->as.name.len, &slot);
        if (k == REF_MEMBER) {
            int sym = const_symbol(c, target->as.name.name, target->as.name.len);
            emit_op(c, OP_GET_LOCAL); emit16(c, 0);       /* [self] */
            emit_op(c, OP_DUP);                            /* [self self] */
            emit_op(c, OP_GET_PROPERTY); emit16(c, (uint16_t)sym); /* [self val] */
            if (value) compile_expr(c, value); else emit_const(c, mk_number(1));
            emit_op(c, binop);
            emit_op(c, OP_SET_PROPERTY); emit16(c, (uint16_t)sym);
            break;
        }
        emit_get_var(c, target->as.name.name, target->as.name.len);
        if (value) compile_expr(c, value); else emit_const(c, mk_number(1));
        emit_op(c, binop);
        if (k == REF_LOCAL) { emit_op(c, OP_SET_LOCAL); emit16(c, (uint16_t)slot); }
        else { emit_op(c, OP_SET_GLOBAL); emit16(c, (uint16_t)const_string(c, target->as.name.name, target->as.name.len)); }
        break;
    }
    case N_MEMBER: {
        int sym = const_symbol(c, target->as.member.name, target->as.member.len);
        compile_expr(c, target->as.member.object);   /* [obj] */
        emit_op(c, OP_DUP);                            /* [obj obj] */
        emit_op(c, OP_GET_PROPERTY); emit16(c, (uint16_t)sym); /* [obj val] */
        if (value) compile_expr(c, value); else emit_const(c, mk_number(1));
        emit_op(c, binop);                             /* [obj result] */
        emit_op(c, OP_SET_PROPERTY); emit16(c, (uint16_t)sym);
        break;
    }
    case N_INDEX:
        compile_expr(c, target->as.index.object);      /* [obj] */
        compile_expr(c, target->as.index.index);       /* [obj idx] */
        emit_op(c, OP_DUP2);                            /* [obj idx obj idx] */
        emit_op(c, OP_GET_INDEX);                       /* [obj idx val] */
        if (value) compile_expr(c, value); else emit_const(c, mk_number(1));
        emit_op(c, binop);                              /* [obj idx result] */
        emit_op(c, OP_SET_INDEX);
        break;
    default:
        cerror(c, target, "invalid assignment target");
        break;
    }
}

/* ---------------------------------------------------------- expressions ---- */

static bool is_super_ident(Node *n) {
    return n->type == N_IDENT && n->as.name.len == 5 && memcmp(n->as.name.name, "super", 5) == 0;
}

static void compile_call(Comp *c, Node *n) {
    Node *callee = n->as.call.callee;
    if (n->as.call.argc > 255) { cerror(c, n, "too many call arguments"); return; }

    if (callee->type == N_MEMBER) {
        Node *obj = callee->as.member.object;
        if (is_super_ident(obj)) {
            /* super.method(args) -> dispatch from the defining class's superclass */
            if (!c->klass) { cerror(c, n, "'super' used outside a method"); return; }
            emit_op(c, OP_GET_LOCAL); emit16(c, 0); /* self as receiver */
            for (int i = 0; i < n->as.call.argc; i++) compile_expr(c, n->as.call.args[i]);
            emit_op(c, OP_INVOKE_SUPER);
            emit16(c, (uint16_t)const_symbol(c, callee->as.member.name, callee->as.member.len));
            emit(c, (uint8_t)n->as.call.argc);
            return;
        }
        /* method invocation: obj.name(args) -> INVOKE */
        compile_expr(c, obj);
        for (int i = 0; i < n->as.call.argc; i++) compile_expr(c, n->as.call.args[i]);
        emit_op(c, OP_INVOKE);
        emit16(c, (uint16_t)const_symbol(c, callee->as.member.name, callee->as.member.len));
        emit(c, (uint8_t)n->as.call.argc);
        return;
    }

    /* Unqualified call to a class member -> self.name(args). */
    if (callee->type == N_IDENT) {
        int slot;
        if (resolve_ident(c, callee->as.name.name, callee->as.name.len, &slot) == REF_MEMBER) {
            emit_op(c, OP_GET_LOCAL); emit16(c, 0); /* self */
            for (int i = 0; i < n->as.call.argc; i++) compile_expr(c, n->as.call.args[i]);
            emit_op(c, OP_INVOKE);
            emit16(c, (uint16_t)const_symbol(c, callee->as.name.name, callee->as.name.len));
            emit(c, (uint8_t)n->as.call.argc);
            return;
        }
    }

    compile_expr(c, callee);
    for (int i = 0; i < n->as.call.argc; i++) compile_expr(c, n->as.call.args[i]);
    emit_op(c, OP_CALL);
    emit(c, (uint8_t)n->as.call.argc);
}

static void compile_expr(Comp *c, Node *n) {
    if (!n) { emit_op(c, OP_NULL); return; }
    c->line = n->line;
    switch (n->type) {
    case N_LITERAL:
        switch (n->as.lit.kind) {
        case TOK_NUMBER: emit_const(c, mk_number((int32_t)n->as.lit.i)); break;
        case TOK_LONG:   emit_const(c, mk_long(n->as.lit.i)); break;
        case TOK_FLOAT:  emit_const(c, mk_float(n->as.lit.f)); break;
        case TOK_DOUBLE: emit_const(c, mk_double(n->as.lit.d)); break;
        case TOK_CHAR:   emit_const(c, mk_char(n->as.lit.ch)); break;
        case TOK_TRUE:   emit_op(c, OP_TRUE); break;
        case TOK_FALSE:  emit_op(c, OP_FALSE); break;
        case TOK_NULL:   emit_op(c, OP_NULL); break;
        case TOK_STRING: { emit_op(c, OP_CONSTANT); emit16(c, (uint16_t)const_string(c, n->as.lit.str, n->as.lit.slen)); break; }
        default: cerror(c, n, "bad literal"); break;
        }
        break;
    case N_SYMBOL:
        emit_op(c, OP_CONSTANT); emit16(c, (uint16_t)const_symbol(c, n->as.name.name, n->as.name.len));
        break;
    case N_IDENT:
        emit_get_var(c, n->as.name.name, n->as.name.len);
        break;
    case N_SELF:
        emit_op(c, OP_GET_LOCAL); emit16(c, 0); /* slot 0 is the receiver/function */
        break;
    case N_ARRAY:
        for (int i = 0; i < n->as.list.count; i++) compile_expr(c, n->as.list.items[i]);
        emit_op(c, OP_BUILD_ARRAY); emit16(c, (uint16_t)n->as.list.count);
        break;
    case N_BYTEARRAY:
        for (int i = 0; i < n->as.list.count; i++) compile_expr(c, n->as.list.items[i]);
        emit_op(c, OP_BUILD_BYTEARRAY); emit16(c, (uint16_t)n->as.list.count);
        break;
    case N_DICT:
        for (int i = 0; i < n->as.dict.count; i++) {
            compile_expr(c, n->as.dict.keys[i]);
            compile_expr(c, n->as.dict.vals[i]);
        }
        emit_op(c, OP_BUILD_DICT); emit16(c, (uint16_t)n->as.dict.count);
        break;
    case N_UNARY: {
        TokenType op = n->as.unary.op;
        if (op == TOK_PLUS_PLUS || op == TOK_MINUS_MINUS) {
            /* ++/-- desugar to target +=/-= 1 (statement-level sugar). */
            compile_compound_assign(c, n->as.unary.operand,
                                    op == TOK_PLUS_PLUS ? OP_ADD : OP_SUB, NULL);
        } else {
            compile_expr(c, n->as.unary.operand);
            switch (op) {
            case TOK_MINUS: emit_op(c, OP_NEG); break;
            case TOK_BANG: case TOK_NOT: emit_op(c, OP_NOT); break;
            case TOK_TILDE: emit_op(c, OP_BNOT); break;
            default: cerror(c, n, "bad unary operator"); break;
            }
        }
        break;
    }
    case N_BINARY: {
        OpCode op = binop_opcode(n->as.binary.op);
        if (op == OP__COUNT) { cerror(c, n, "bad binary operator"); break; }
        compile_expr(c, n->as.binary.left);
        compile_expr(c, n->as.binary.right);
        emit_op(c, op);
        break;
    }
    case N_LOGICAL: {
        bool is_and = (n->as.binary.op == TOK_AMP_AMP || n->as.binary.op == TOK_AND);
        compile_expr(c, n->as.binary.left);
        emit_op(c, OP_DUP);
        int j = emit_jump(c, is_and ? OP_JUMP_IF_FALSE : OP_JUMP_IF_TRUE);
        emit_op(c, OP_POP);                    /* discard the duplicated left */
        compile_expr(c, n->as.binary.right);
        patch_jump(c, j);
        break;
    }
    case N_TERNARY: {
        compile_expr(c, n->as.ternary.cond);
        int else_j = emit_jump(c, OP_JUMP_IF_FALSE);
        compile_expr(c, n->as.ternary.then_e);
        int end_j = emit_jump(c, OP_JUMP);
        patch_jump(c, else_j);
        compile_expr(c, n->as.ternary.else_e);
        patch_jump(c, end_j);
        break;
    }
    case N_ASSIGN: {
        if (n->as.assign.op == TOK_ASSIGN)
            compile_plain_assign(c, n->as.assign.target, n->as.assign.value);
        else {
            OpCode op = compound_opcode(n->as.assign.op);
            if (op == OP__COUNT) { cerror(c, n, "bad compound assignment"); break; }
            compile_compound_assign(c, n->as.assign.target, op, n->as.assign.value);
        }
        break;
    }
    case N_CALL:
        compile_call(c, n);
        break;
    case N_INDEX:
        compile_expr(c, n->as.index.object);
        compile_expr(c, n->as.index.index);
        emit_op(c, OP_GET_INDEX);
        break;
    case N_MEMBER:
        compile_expr(c, n->as.member.object);
        emit_op(c, OP_GET_PROPERTY);
        emit16(c, (uint16_t)const_symbol(c, n->as.member.name, n->as.member.len));
        break;
    case N_NEW:
        if (n->as.newexpr.cls == NULL) {
            /* new [size] */
            compile_expr(c, n->as.newexpr.args[0]);
            emit_op(c, OP_NEW_ARRAY);
        } else {
            compile_expr(c, n->as.newexpr.cls);
            for (int i = 0; i < n->as.newexpr.argc; i++) compile_expr(c, n->as.newexpr.args[i]);
            if (n->as.newexpr.argc > 255) { cerror(c, n, "too many constructor arguments"); break; }
            emit_op(c, OP_NEW);
            emit(c, (uint8_t)n->as.newexpr.argc);
        }
        break;
    case N_INSTANCEOF:
        compile_expr(c, n->as.instof.expr);
        compile_expr(c, n->as.instof.type);
        emit_op(c, OP_INSTANCEOF);
        break;
    case N_HAS:
        compile_expr(c, n->as.has.expr);
        if (n->as.has.sym->type != N_SYMBOL) { cerror(c, n, "'has' requires a symbol"); break; }
        emit_op(c, OP_HAS);
        emit16(c, (uint16_t)const_symbol(c, n->as.has.sym->as.name.name, n->as.has.sym->as.name.len));
        break;
    default:
        cerror(c, n, "unsupported expression");
        break;
    }
}

/* ------------------------------------------------------------ statements --- */

static void compile_block(Comp *c, Node *n) {
    begin_scope(c);
    for (int i = 0; i < n->as.list.count; i++) compile_stmt(c, n->as.list.items[i]);
    end_scope(c);
}

static void compile_var(Comp *c, Node *n) {
    if (n->as.var.init) compile_expr(c, n->as.var.init);
    else emit_op(c, OP_NULL);
    if (in_global_scope(c)) {
        emit_op(c, OP_DEFINE_GLOBAL);
        emit16(c, (uint16_t)const_string(c, n->as.var.name, n->as.var.len));
    } else {
        add_local(c, n->as.var.name, n->as.var.len);
    }
}

static void loop_push(Comp *c, LoopCtx *ctx, int continue_target) {
    ctx->prev = c->loop;
    ctx->continue_target = continue_target;
    ctx->base_depth = c->current->scope_depth;
    ctx->try_depth = c->current->try_depth;
    ctx->breaks = NULL; ctx->break_count = ctx->break_cap = 0;
    c->loop = ctx;
}

/* Emit OP_TRY_POP for each try handler active since `to_depth` (so a control-flow
 * exit out of a try body keeps the VM handler stack balanced). */
static void pop_tries_to(Comp *c, int to_depth) {
    for (int d = c->current->try_depth; d > to_depth; d--) emit_op(c, OP_TRY_POP);
}
static void loop_pop(Comp *c, LoopCtx *ctx) {
    for (int i = 0; i < ctx->break_count; i++) patch_jump(c, ctx->breaks[i]);
    if (ctx->breaks) vm_free(ctx->breaks);
    c->loop = ctx->prev;
}
static void loop_add_break(LoopCtx *ctx, int off) {
    if (ctx->break_count + 1 > ctx->break_cap) {
        int nc = ctx->break_cap < 4 ? 4 : ctx->break_cap * 2;
        ctx->breaks = (int *)vm_realloc(ctx->breaks, (size_t)nc * sizeof(int));
        ctx->break_cap = nc;
    }
    ctx->breaks[ctx->break_count++] = off;
}

static void compile_if(Comp *c, Node *n) {
    compile_expr(c, n->as.ifs.cond);
    int else_j = emit_jump(c, OP_JUMP_IF_FALSE);
    compile_stmt(c, n->as.ifs.then_s);
    if (n->as.ifs.else_s) {
        int end_j = emit_jump(c, OP_JUMP);
        patch_jump(c, else_j);
        compile_stmt(c, n->as.ifs.else_s);
        patch_jump(c, end_j);
    } else {
        patch_jump(c, else_j);
    }
}

static void compile_while(Comp *c, Node *n) {
    int start = cur_chunk(c)->count;
    compile_expr(c, n->as.loop.cond);
    int exit_j = emit_jump(c, OP_JUMP_IF_FALSE);
    LoopCtx ctx; loop_push(c, &ctx, start);
    compile_stmt(c, n->as.loop.body);
    emit_loop(c, start);
    patch_jump(c, exit_j);
    loop_pop(c, &ctx);
}

static void compile_dowhile(Comp *c, Node *n) {
    int start = cur_chunk(c)->count;
    LoopCtx ctx; loop_push(c, &ctx, start); /* continue re-checks below; see note */
    compile_stmt(c, n->as.loop.body);
    ctx.continue_target = cur_chunk(c)->count; /* continue jumps to the condition */
    compile_expr(c, n->as.loop.cond);
    int exit_j = emit_jump(c, OP_JUMP_IF_FALSE);
    emit_loop(c, start);
    patch_jump(c, exit_j);
    loop_pop(c, &ctx);
}

static void compile_for(Comp *c, Node *n) {
    begin_scope(c);
    if (n->as.forl.init) {
        if (n->as.forl.init->type == N_VAR) compile_var(c, n->as.forl.init);
        else { compile_expr(c, n->as.forl.init); emit_op(c, OP_POP); }
    }
    int cond_start = cur_chunk(c)->count;
    int exit_j = -1;
    if (n->as.forl.cond) {
        compile_expr(c, n->as.forl.cond);
        exit_j = emit_jump(c, OP_JUMP_IF_FALSE);
    }
    int body_j = emit_jump(c, OP_JUMP);       /* skip the update on first pass */
    int update_start = cur_chunk(c)->count;
    if (n->as.forl.update) { compile_expr(c, n->as.forl.update); emit_op(c, OP_POP); }
    emit_loop(c, cond_start);
    patch_jump(c, body_j);

    LoopCtx ctx; loop_push(c, &ctx, update_start);
    compile_stmt(c, n->as.forl.body);
    emit_loop(c, update_start);
    if (exit_j >= 0) patch_jump(c, exit_j);
    loop_pop(c, &ctx);
    end_scope(c);
}

static void compile_switch(Comp *c, Node *n) {
    compile_expr(c, n->as.sw.expr);            /* [S] stays on the stack */
    /* Register S as an anonymous local so local_count reflects the real stack
     * depth for anything compiled inside the switch (e.g. a try/catch: the
     * exception is pushed above S, so the catch var must bind above S too). */
    add_local(c, "", 0);
    LoopCtx ctx; loop_push(c, &ctx, 0);        /* only break is valid inside switch */

    int *body_jumps = NULL;
    if (n->as.sw.count > 0) body_jumps = (int *)vm_alloc((size_t)n->as.sw.count * sizeof(int));

    /* dispatch */
    for (int i = 0; i < n->as.sw.count; i++) {
        if (n->as.sw.case_vals[i] == NULL) continue;   /* default: handled below */
        emit_op(c, OP_DUP);                    /* [S S] */
        compile_expr(c, n->as.sw.case_vals[i]); /* [S S v] */
        emit_op(c, OP_EQ);                      /* [S bool] */
        body_jumps[i] = emit_jump(c, OP_JUMP_IF_TRUE); /* pops bool; -> body i */
    }
    int to_default = emit_jump(c, OP_JUMP);     /* no case matched */

    /* bodies (sequential, so stacked cases fall through) */
    for (int i = 0; i < n->as.sw.count; i++) {
        if (n->as.sw.case_vals[i] == NULL) { patch_jump(c, to_default); to_default = -1; }
        else patch_jump(c, body_jumps[i]);
        compile_block(c, n->as.sw.case_bodies[i]);
    }
    if (to_default >= 0) patch_jump(c, to_default); /* no default clause */

    /* break target / fallthrough end: pop the switch value S */
    loop_pop(c, &ctx);
    emit_op(c, OP_POP);
    c->current->local_count--;                 /* release the anonymous S slot */
    if (body_jumps) vm_free(body_jumps);
}

/* try { BODY } catch (e [instanceof T]) { ... } ... [finally { F }]
 *
 *     TRY_PUSH Lhandler
 *     <BODY>
 *     TRY_POP ; <F> ; JUMP Lend        (normal completion)
 *   Lhandler:  ; the exception E is on the stack
 *     for each catch clause: [DUP; <T>; INSTANCEOF; JUMP_IF_FALSE next] then
 *       bind e=E; <clause body>; pop e; <F>; JUMP Lend
 *     if no catch-all matched: <F>; THROW   (re-raise E after finally)
 *   Lend:
 * The finally block is emitted inline at each exit (a small, common tradeoff).
 * NOTE: return/break/continue inside the try DO run TRY_POP (handler-stack safe)
 * but skip the finally block — a documented baseline limitation.
 */
static void compile_try(Comp *c, Node *n) {
    Node *fin = n->as.tryc.finally_block;
    c->current->has_try = true;

    int handler_jump = emit_jump(c, OP_TRY_PUSH);
    c->current->try_depth++;
    compile_stmt(c, n->as.tryc.block);
    emit_op(c, OP_TRY_POP);
    c->current->try_depth--;
    if (fin) compile_stmt(c, fin);
    int normal_end = emit_jump(c, OP_JUMP);

    patch_jump(c, handler_jump); /* Lhandler: exception on the stack */

    int end_jumps[64]; int end_count = 0;
    int miss_jump = -1; /* pending JUMP_IF_FALSE from the previous typed clause */
    for (int i = 0; i < n->as.tryc.catch_count; i++) {
        if (miss_jump >= 0) { patch_jump(c, miss_jump); miss_jump = -1; }
        Node *ctype = n->as.tryc.catch_types[i];
        if (ctype) {
            emit_op(c, OP_DUP);
            compile_expr(c, ctype);
            emit_op(c, OP_INSTANCEOF);
            miss_jump = emit_jump(c, OP_JUMP_IF_FALSE);
        }
        begin_scope(c);
        char *cname = n->as.tryc.catch_names[i];
        add_local(c, cname ? cname : "", cname ? n->as.tryc.catch_name_lens[i] : 0);
        compile_stmt(c, n->as.tryc.catch_bodies[i]);
        end_scope(c); /* pops the exception bound to e */
        if (fin) compile_stmt(c, fin);
        if (end_count < 64) end_jumps[end_count++] = emit_jump(c, OP_JUMP);
    }
    /* If nothing here handles all exceptions (no catch-all clause), the exception
     * is uncaught by this try: run finally, then re-raise it (E is on the stack).
     * This also covers a try with only a finally (no catch clauses). */
    bool has_catch_all = false;
    for (int i = 0; i < n->as.tryc.catch_count; i++)
        if (n->as.tryc.catch_types[i] == NULL) { has_catch_all = true; break; }
    if (miss_jump >= 0) patch_jump(c, miss_jump);
    if (!has_catch_all) {
        if (fin) compile_stmt(c, fin);
        emit_op(c, OP_THROW);
    }

    patch_jump(c, normal_end);
    for (int i = 0; i < end_count; i++) patch_jump(c, end_jumps[i]);
}

static void compile_break(Comp *c, Node *n) {
    if (!c->loop) { cerror(c, n, "'break' outside a loop or switch"); return; }
    pop_tries_to(c, c->loop->try_depth);
    emit_pops_above(c, c->loop->base_depth);
    int off = emit_jump(c, OP_JUMP);
    loop_add_break(c->loop, off);
}
static void compile_continue(Comp *c, Node *n) {
    if (!c->loop) { cerror(c, n, "'continue' outside a loop"); return; }
    pop_tries_to(c, c->loop->try_depth);
    emit_pops_above(c, c->loop->base_depth);
    emit_loop(c, c->loop->continue_target);
}

static void compile_stmt(Comp *c, Node *n) {
    if (!n) return;
    c->line = n->line;
    switch (n->type) {
    case N_EXPR_STMT:
        compile_expr(c, n->as.expr.expr);
        emit_op(c, OP_POP);
        break;
    case N_VAR:   compile_var(c, n); break;
    case N_BLOCK: compile_block(c, n); break;
    case N_IF:    compile_if(c, n); break;
    case N_WHILE: compile_while(c, n); break;
    case N_DOWHILE: compile_dowhile(c, n); break;
    case N_FOR:   compile_for(c, n); break;
    case N_SWITCH: compile_switch(c, n); break;
    case N_TRY:    compile_try(c, n); break;
    case N_RETURN:
        if (c->current->in_initializer) { emit_op(c, OP_GET_LOCAL); emit16(c, 0); } /* return self */
        else if (n->as.expr.expr) compile_expr(c, n->as.expr.expr);
        else emit_op(c, OP_NULL);
        pop_tries_to(c, 0); /* leave any enclosing try handlers before returning */
        emit_op(c, OP_RETURN);
        break;
    case N_BREAK:    compile_break(c, n); break;
    case N_CONTINUE: compile_continue(c, n); break;
    case N_THROW:
        compile_expr(c, n->as.expr.expr);
        emit_op(c, OP_THROW);
        break;
    default:
        /* declarations appearing where a statement is expected */
        compile_decl(c, n);
        break;
    }
}

/* ---------------------------------------------------------- declarations --- */

static void init_compiler(Comp *c, Compiler *comp, Compiler *enclosing,
                          ObjString *name, uint8_t arity) {
    comp->enclosing = enclosing;
    comp->function = function_new(name, arity);
    comp->locals = (Local *)vm_alloc(8 * sizeof(Local));
    comp->local_cap = 8;
    comp->local_count = 0;
    comp->local_max = 0;
    comp->scope_depth = 0;
    comp->try_depth = 0;
    comp->has_try = false;
    comp->in_initializer = false;
    c->current = comp;
    add_local(c, "", 0); /* reserve slot 0 for the callee/receiver */
}

static ObjFunction *end_compiler(Comp *c) {
    Compiler *cc = c->current;
    ObjFunction *fn = cc->function;
    /* Frame needs: slot 0 + params (arity+1) plus the body's peak temp/local depth.
     * Exception handlers push the exception value at a depth the linear stack-effect
     * walk can't see (the catch code is jumped over on the normal path), so add a
     * safe margin for try-containing functions. The VM reserves this at call time so
     * the value stack can never overflow. */
    uint32_t base = (uint32_t)fn->arity + 1u + (uint32_t)chunk_max_stack(fn->chunk);
    if (cc->has_try) base += (uint32_t)cc->local_max + 8u;
    fn->frame_size = base;
    vm_free(cc->locals);
    c->current = cc->enclosing;
    return fn;
}

static void compile_function(Comp *c, Node *n) {
    if (n->as.func.param_count > 255) {
        cerror(c, n, "too many function parameters (max 255)");
        return;
    }
    ObjString *name = string_new(n->as.func.name, n->as.func.len);
    Compiler comp;
    init_compiler(c, &comp, c->current, name, (uint8_t)n->as.func.param_count);
    release(mk_obj((Obj *)name)); /* function_new retained it */

    for (int i = 0; i < n->as.func.param_count; i++)
        add_local(c, n->as.func.params[i], n->as.func.param_lens[i]);

    /* Compile the body's statements directly at function scope (rather than as a
     * nested block), so body locals share the params' depth and are discarded on
     * return without emitting dead POPs after RETURN. */
    if (n->as.func.body) {
        Node *body = n->as.func.body;
        if (body->type == N_BLOCK)
            for (int i = 0; i < body->as.list.count; i++) compile_stmt(c, body->as.list.items[i]);
        else
            compile_stmt(c, body);
    }
    emit_op(c, OP_NULL);
    emit_op(c, OP_RETURN);

    ObjFunction *fn = end_compiler(c);
    /* Bind the function as a global by name. */
    emit_op(c, OP_CONSTANT); emit16(c, (uint16_t)make_const(c, mk_obj((Obj *)fn)));
    release(mk_obj((Obj *)fn));
    emit_op(c, OP_DEFINE_GLOBAL);
    emit16(c, (uint16_t)const_string(c, n->as.func.name, n->as.func.len));
}

static void compile_const(Comp *c, Node *n) {
    compile_expr(c, n->as.var.init);
    if (in_global_scope(c)) {
        emit_op(c, OP_DEFINE_GLOBAL);
        emit16(c, (uint16_t)const_string(c, n->as.var.name, n->as.var.len));
    } else {
        add_local(c, n->as.var.name, n->as.var.len);
    }
}

static void compile_enum(Comp *c, Node *n) {
    int32_t next = 0;
    for (int i = 0; i < n->as.enm.count; i++) {
        Node *val = n->as.enm.values[i];
        int32_t v;
        if (val) {
            if (val->type == N_LITERAL && val->as.lit.kind == TOK_NUMBER) v = (int32_t)val->as.lit.i;
            else { cerror(c, val, "enum value must be an integer literal"); return; }
        } else {
            v = next;
        }
        next = v + 1;
        emit_const(c, mk_number(v));
        emit_op(c, OP_DEFINE_GLOBAL);
        emit16(c, (uint16_t)const_string(c, n->as.enm.names[i], n->as.enm.name_lens[i]));
    }
}

static void compile_module(Comp *c, Node *n, bool as_member);

static bool method_is_initialize(Node *m) {
    return m->as.func.len == 10 && memcmp(m->as.func.name, "initialize", 10) == 0;
}

/* Compile one class method (or, if fn_node is NULL, a synthesized initialize with
 * only the field-init prologue). Returns the ObjFunction (refcount 1). */
static ObjFunction *compile_method(Comp *c, Node *class_node, Node *fn_node,
                                   ClassCtx *member_ctx, bool is_init, bool is_static) {
    const char *mname = fn_node ? fn_node->as.func.name : "initialize";
    int mlen = fn_node ? fn_node->as.func.len : 10;
    uint8_t arity = fn_node ? (uint8_t)fn_node->as.func.param_count : 0;

    ObjString *name = string_new(mname, mlen);
    Compiler comp;
    init_compiler(c, &comp, c->current, name, arity);
    release(mk_obj((Obj *)name));
    comp.in_initializer = is_init;

    ClassCtx *saved = c->klass;
    c->klass = is_static ? NULL : member_ctx; /* static methods have no `self` */

    (void)class_node;
    if (fn_node)
        for (int i = 0; i < fn_node->as.func.param_count; i++)
            add_local(c, fn_node->as.func.params[i], fn_node->as.func.param_lens[i]);

    if (fn_node && fn_node->as.func.body) {
        Node *body = fn_node->as.func.body;
        if (body->type == N_BLOCK)
            for (int i = 0; i < body->as.list.count; i++) compile_stmt(c, body->as.list.items[i]);
        else compile_stmt(c, body);
    }
    if (is_init) { emit_op(c, OP_GET_LOCAL); emit16(c, 0); } /* return self */
    else emit_op(c, OP_NULL);
    emit_op(c, OP_RETURN);

    c->klass = saved;
    return end_compiler(c);
}

/* Compile the hidden per-class $fieldinit method: sets this class's own instance
 * fields to their initializers (self.<field> = <init>), then returns self. Run
 * base-first for the whole chain by OP_NEW at construction. */
static ObjFunction *compile_fieldinit(Comp *c, Node *class_node, ClassCtx *ctx) {
    ObjString *name = string_new("$fieldinit", 10);
    Compiler comp;
    init_compiler(c, &comp, c->current, name, 0);
    release(mk_obj((Obj *)name));
    ClassCtx *saved = c->klass;
    c->klass = ctx;
    for (int i = 0; i < class_node->as.cls.count; i++) {
        Node *m = class_node->as.cls.members[i];
        if (m->type != N_VAR || (m->as.var.modifiers & MOD_STATIC) || !m->as.var.init) continue;
        emit_op(c, OP_GET_LOCAL); emit16(c, 0);   /* self */
        compile_expr(c, m->as.var.init);
        emit_op(c, OP_SET_PROPERTY);
        emit16(c, (uint16_t)const_symbol(c, m->as.var.name, m->as.var.len));
    }
    emit_op(c, OP_GET_LOCAL); emit16(c, 0);       /* return self */
    emit_op(c, OP_RETURN);
    c->klass = saved;
    return end_compiler(c);
}

/* Find a class declaration by simple name anywhere in the program (top level or
 * inside a module), so inherited members can be resolved at compile time. */
static Node *find_class_in(Node *scope, const char *name, int len) {
    Node **items; int count;
    if (scope->type == N_PROGRAM) { items = scope->as.list.items; count = scope->as.list.count; }
    else if (scope->type == N_MODULE) { items = scope->as.mod.members; count = scope->as.mod.count; }
    else return NULL;
    for (int i = 0; i < count; i++) {
        Node *m = items[i];
        if (m->type == N_CLASS && m->as.cls.len == len && memcmp(m->as.cls.name, name, (size_t)len) == 0)
            return m;
        if (m->type == N_MODULE) { Node *r = find_class_in(m, name, len); if (r) return r; }
    }
    return NULL;
}

/* Growable member-name set (borrowed names from the AST). */
typedef struct { const char **names; int *lens; int count, cap; } MemberSet;
static void ms_add(MemberSet *ms, const char *name, int len) {
    if (ms->count + 1 > ms->cap) {
        int nc = ms->cap < 8 ? 8 : ms->cap * 2;
        ms->names = (const char **)vm_realloc(ms->names, (size_t)nc * sizeof(char *));
        ms->lens = (int *)vm_realloc(ms->lens, (size_t)nc * sizeof(int));
        ms->cap = nc;
    }
    ms->names[ms->count] = name;
    ms->lens[ms->count] = len;
    ms->count++;
}

/* Collect the instance fields + methods of a class and all its ancestors so that
 * unqualified references in a method resolve to self.<member> (incl. inherited). */
static void collect_members(Comp *c, Node *cn, MemberSet *ms, int depth) {
    if (depth > 64) return; /* guard against a malformed inheritance cycle */
    for (int i = 0; i < cn->as.cls.count; i++) {
        Node *m = cn->as.cls.members[i];
        if (m->type == N_VAR && !(m->as.var.modifiers & MOD_STATIC))
            ms_add(ms, m->as.var.name, m->as.var.len);
        else if (m->type == N_FUNCTION && !(m->as.func.modifiers & MOD_STATIC))
            ms_add(ms, m->as.func.name, m->as.func.len);
    }
    if (!cn->as.cls.super) return;
    Node *sup = cn->as.cls.super;
    const char *sname; int slen;
    if (sup->type == N_IDENT)       { sname = sup->as.name.name; slen = sup->as.name.len; }
    else if (sup->type == N_MEMBER) { sname = sup->as.member.name; slen = sup->as.member.len; }
    else return;
    Node *super_node = c->program ? find_class_in(c->program, sname, slen) : NULL;
    if (super_node) collect_members(c, super_node, ms, depth + 1);
    else if (slen >= 9 && memcmp(sname + slen - 9, "Exception", 9) == 0)
        ms_add(ms, "mMessage", 8); /* built-in Exception base field */
}

static void compile_class(Comp *c, Node *n, bool as_member) {
    /* Member set for self-qualified resolution: own + inherited fields/methods. */
    MemberSet ms = { NULL, NULL, 0, 0 };
    collect_members(c, n, &ms, 0);
    bool has_field_init = false;
    for (int i = 0; i < n->as.cls.count; i++) {
        Node *m = n->as.cls.members[i];
        if (m->type == N_VAR && !(m->as.var.modifiers & MOD_STATIC) && m->as.var.init) has_field_init = true;
    }
    ClassCtx ctx = { ms.names, ms.lens, ms.count };

    emit_op(c, OP_CLASS);
    emit16(c, (uint16_t)const_string(c, n->as.cls.name, n->as.cls.len));
    if (n->as.cls.super) { compile_expr(c, n->as.cls.super); emit_op(c, OP_INHERIT); }

    for (int i = 0; i < n->as.cls.count; i++) {
        Node *m = n->as.cls.members[i];
        switch (m->type) {
        case N_FUNCTION: {
            bool st = (m->as.func.modifiers & MOD_STATIC) != 0;
            bool is_init = !st && method_is_initialize(m);
            ObjFunction *fn = compile_method(c, n, m, &ctx, is_init, st);
            emit_op(c, OP_CONSTANT); emit16(c, (uint16_t)make_const(c, mk_obj((Obj *)fn)));
            release(mk_obj((Obj *)fn));
            emit_op(c, st ? OP_MEMBER : OP_METHOD);
            emit16(c, (uint16_t)const_symbol(c, m->as.func.name, m->as.func.len));
            break;
        }
        case N_VAR:
            if (m->as.var.modifiers & MOD_STATIC) { /* static field */
                if (m->as.var.init) compile_expr(c, m->as.var.init); else emit_op(c, OP_NULL);
                emit_op(c, OP_MEMBER); emit16(c, (uint16_t)const_symbol(c, m->as.var.name, m->as.var.len));
            }
            break; /* instance field handled in the initialize prologue */
        case N_CONST:
            compile_expr(c, m->as.var.init);
            emit_op(c, OP_MEMBER); emit16(c, (uint16_t)const_symbol(c, m->as.var.name, m->as.var.len));
            break;
        case N_ENUM: {
            int32_t next = 0;
            for (int j = 0; j < m->as.enm.count; j++) {
                Node *val = m->as.enm.values[j];
                int32_t v = val && val->type == N_LITERAL && val->as.lit.kind == TOK_NUMBER
                            ? (int32_t)val->as.lit.i : next;
                next = v + 1;
                emit_const(c, mk_number(v));
                emit_op(c, OP_MEMBER); emit16(c, (uint16_t)const_symbol(c, m->as.enm.names[j], m->as.enm.name_lens[j]));
            }
            break;
        }
        case N_CLASS:  compile_class(c, m, true); break;
        case N_MODULE: compile_module(c, m, true); break;
        default: break;
        }
    }

    /* If the class has instance-field initializers, emit its $fieldinit method
     * (run for the whole chain at construction, independent of initialize). */
    if (has_field_init) {
        ObjFunction *fn = compile_fieldinit(c, n, &ctx);
        emit_op(c, OP_CONSTANT); emit16(c, (uint16_t)make_const(c, mk_obj((Obj *)fn)));
        release(mk_obj((Obj *)fn));
        emit_op(c, OP_METHOD); emit16(c, (uint16_t)const_symbol(c, "$fieldinit", 10));
    }

    if (as_member) { emit_op(c, OP_MEMBER); emit16(c, (uint16_t)const_symbol(c, n->as.cls.name, n->as.cls.len)); }
    else { emit_op(c, OP_DEFINE_GLOBAL); emit16(c, (uint16_t)const_string(c, n->as.cls.name, n->as.cls.len)); }

    if (ms.names) vm_free(ms.names);
    if (ms.lens) vm_free(ms.lens);
}

static void compile_module(Comp *c, Node *n, bool as_member) {
    emit_op(c, OP_MODULE);
    emit16(c, (uint16_t)const_string(c, n->as.mod.name, n->as.mod.len));
    for (int i = 0; i < n->as.mod.count; i++) {
        Node *m = n->as.mod.members[i];
        switch (m->type) {
        case N_FUNCTION: {
            ObjFunction *fn = compile_method(c, m, m, NULL, false, true); /* no self */
            emit_op(c, OP_CONSTANT); emit16(c, (uint16_t)make_const(c, mk_obj((Obj *)fn)));
            release(mk_obj((Obj *)fn));
            emit_op(c, OP_MEMBER); emit16(c, (uint16_t)const_symbol(c, m->as.func.name, m->as.func.len));
            break;
        }
        case N_VAR:
            if (m->as.var.init) compile_expr(c, m->as.var.init); else emit_op(c, OP_NULL);
            emit_op(c, OP_MEMBER); emit16(c, (uint16_t)const_symbol(c, m->as.var.name, m->as.var.len));
            break;
        case N_CONST:
            compile_expr(c, m->as.var.init);
            emit_op(c, OP_MEMBER); emit16(c, (uint16_t)const_symbol(c, m->as.var.name, m->as.var.len));
            break;
        case N_ENUM: {
            int32_t next = 0;
            for (int j = 0; j < m->as.enm.count; j++) {
                Node *val = m->as.enm.values[j];
                int32_t v = val && val->type == N_LITERAL && val->as.lit.kind == TOK_NUMBER
                            ? (int32_t)val->as.lit.i : next;
                next = v + 1;
                emit_const(c, mk_number(v));
                emit_op(c, OP_MEMBER); emit16(c, (uint16_t)const_symbol(c, m->as.enm.names[j], m->as.enm.name_lens[j]));
            }
            break;
        }
        case N_CLASS:  compile_class(c, m, true); break;
        case N_MODULE: compile_module(c, m, true); break;
        default: break;
        }
    }
    if (as_member) { emit_op(c, OP_MEMBER); emit16(c, (uint16_t)const_symbol(c, n->as.mod.name, n->as.mod.len)); }
    else { emit_op(c, OP_DEFINE_GLOBAL); emit16(c, (uint16_t)const_string(c, n->as.mod.name, n->as.mod.len)); }
}

static void compile_decl(Comp *c, Node *n) {
    c->line = n->line;
    switch (n->type) {
    case N_FUNCTION: compile_function(c, n); break;
    case N_VAR:      compile_var(c, n); break;
    case N_CONST:    compile_const(c, n); break;
    case N_ENUM:     compile_enum(c, n); break;
    case N_USING:    break; /* name resolution is dynamic; no code needed */
    case N_CLASS:    compile_class(c, n, false); break;
    case N_MODULE:   compile_module(c, n, false); break;
    default:
        compile_stmt(c, n);
        break;
    }
}

/* ------------------------------------------------------------- entry point - */

ObjFunction *compile(Ast *ast, CompileResult *result) {
    Comp c;
    memset(&c, 0, sizeof(c));
    c.program = ast->root;

    Compiler top;
    init_compiler(&c, &top, NULL, NULL, 0);

    if (ast->root) {
        for (int i = 0; i < ast->root->as.list.count; i++)
            compile_decl(&c, ast->root->as.list.items[i]);
    }
    emit_op(&c, OP_NULL);
    emit_op(&c, OP_RETURN);

    ObjFunction *fn = end_compiler(&c);

    result->had_error = c.had_error;
    result->error_line = c.error_line;
    memcpy(result->error_msg, c.error_msg, sizeof(result->error_msg));

    if (c.had_error) {
        release(mk_obj((Obj *)fn));
        return NULL;
    }
    return fn;
}
