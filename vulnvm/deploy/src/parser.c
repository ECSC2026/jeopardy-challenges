/* parser.c — MonkeyC recursive-descent parser with Pratt-style expression
 * precedence. Produces an Ast (see ast.h). On the first error the parser records
 * it and stops; loops check had_error so parsing unwinds promptly. All node,
 * array and string allocations are tracked in the Ast for one-pass cleanup. */
#include "parser.h"
#include "alloc.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    Token *toks;
    int    count;
    int    pos;
    Ast   *ast;
} P;

/* --------------------------------------------------- allocation tracking --- */

static void record(Ast *a, void *p) {
    if (!p) return;
    if (a->alloc_count + 1 > a->alloc_cap) {
        int nc = a->alloc_cap < 16 ? 16 : a->alloc_cap * 2;
        a->allocs = (void **)vm_realloc(a->allocs, (size_t)nc * sizeof(void *));
        a->alloc_cap = nc;
    }
    a->allocs[a->alloc_count++] = p;
}

/* Keep a growable buffer registered in one alloc slot across reallocs. */
static void record_replace(Ast *a, int *slot, void *p) {
    if (*slot < 0) { record(a, p); *slot = a->alloc_count - 1; }
    else a->allocs[*slot] = p;
}

typedef struct { Node **data; int count, cap, slot; } NodeVec;
static NodeVec nv_new(void) { NodeVec v = { NULL, 0, 0, -1 }; return v; }
static void nv_push(P *p, NodeVec *v, Node *n) {
    if (v->count + 1 > v->cap) {
        int nc = v->cap < 4 ? 4 : v->cap * 2;
        v->data = (Node **)vm_realloc(v->data, (size_t)nc * sizeof(Node *));
        v->cap = nc;
        record_replace(p->ast, &v->slot, v->data);
    }
    v->data[v->count++] = n;
}

typedef struct { char **names; int *lens; int count, cap, slot_n, slot_l; } StrVec;
static StrVec sv_new(void) { StrVec v = { NULL, NULL, 0, 0, -1, -1 }; return v; }
static void sv_push(P *p, StrVec *v, char *name, int len) {
    if (v->count + 1 > v->cap) {
        int nc = v->cap < 4 ? 4 : v->cap * 2;
        v->names = (char **)vm_realloc(v->names, (size_t)nc * sizeof(char *));
        v->lens  = (int *)vm_realloc(v->lens, (size_t)nc * sizeof(int));
        v->cap = nc;
        record_replace(p->ast, &v->slot_n, v->names);
        record_replace(p->ast, &v->slot_l, v->lens);
    }
    v->names[v->count] = name;
    v->lens[v->count] = len;
    v->count++;
}

/* ------------------------------------------------------------ token helpers */

static Token *cur(P *p)  { return &p->toks[p->pos]; }
static Token *prevt(P *p) { return &p->toks[p->pos - 1]; }
static bool check(P *p, TokenType t) { return cur(p)->type == t; }
static bool at_end(P *p) { return cur(p)->type == TOK_EOF; }
static Token *advance(P *p) { if (!at_end(p)) p->pos++; return prevt(p); }
static bool match(P *p, TokenType t) { if (check(p, t)) { advance(p); return true; } return false; }

static void error_at(P *p, Token *t, const char *msg) {
    if (p->ast->had_error) return;
    p->ast->had_error = true;
    p->ast->error_line = t->line;
    p->ast->error_col = t->col;
    const char *lex = (t->type == TOK_EOF) ? "<eof>" : t->start;
    int lexlen = (t->type == TOK_EOF) ? 5 : (t->length > 0 ? t->length : 1);
    snprintf(p->ast->error_msg, sizeof(p->ast->error_msg),
             "%s (line %d:%d, near '%.*s')", msg, t->line, t->col, lexlen, lex);
}
static void perr(P *p, const char *msg) { error_at(p, cur(p), msg); }
static Token *consume(P *p, TokenType t, const char *msg) {
    if (check(p, t)) return advance(p);
    perr(p, msg);
    return cur(p);
}

/* ---------------------------------------------------------- node helpers --- */

static Node *mknode(P *p, NodeType t) {
    Node *n = (Node *)vm_alloc(sizeof(Node));
    memset(n, 0, sizeof(*n));
    n->type = t;
    n->line = cur(p)->line;
    n->col = cur(p)->col;
    record(p->ast, n);
    return n;
}

static char *pstrdup(P *p, const char *s, int len) {
    char *c = (char *)vm_alloc((size_t)len + 1);
    memcpy(c, s, (size_t)len);
    c[len] = '\0';
    record(p->ast, c);
    return c;
}

/* --------------------------------------------------- forward declarations -- */

static Node *parse_expression(P *p);
static Node *parse_assignment(P *p);
static Node *parse_statement(P *p);
static Node *parse_declaration(P *p);
static Node *parse_block(P *p);
static Node *parse_type_ref(P *p);
static void  skip_type(P *p);

/* -------------------------------------------------------------- literals --- */

static Node *lit_node(P *p, TokenType kind) {
    Token *t = cur(p);
    Node *n = mknode(p, N_LITERAL);
    n->as.lit.kind = kind;
    switch (kind) {
    case TOK_NUMBER: n->as.lit.i = t->num.i32; break;
    case TOK_LONG:   n->as.lit.i = t->num.i64; break;
    case TOK_FLOAT:  n->as.lit.f = t->num.f32; break;
    case TOK_DOUBLE: n->as.lit.d = t->num.f64; break;
    case TOK_CHAR:   n->as.lit.ch = t->num.ch; break;
    case TOK_STRING: n->as.lit.str = pstrdup(p, t->str, t->str_len); n->as.lit.slen = t->str_len; break;
    default: break; /* true/false/null carry no payload */
    }
    advance(p);
    return n;
}

static Node *parse_array_literal(P *p) {
    Node *n = mknode(p, N_ARRAY);
    consume(p, TOK_LBRACKET, "expected '['");
    NodeVec elems = nv_new();
    if (!check(p, TOK_RBRACKET)) {
        do {
            if (check(p, TOK_RBRACKET)) break; /* trailing comma */
            nv_push(p, &elems, parse_expression(p));
        } while (match(p, TOK_COMMA) && !p->ast->had_error);
    }
    Token *rb = consume(p, TOK_RBRACKET, "expected ']' to close array");
    /* `[...]b` is a ByteArray literal — but only when the `b` is immediately
     * adjacent to the `]` (no intervening whitespace), so a following identifier
     * named `b` on the next line is not silently absorbed. */
    if (check(p, TOK_IDENT) && cur(p)->length == 1 && cur(p)->start[0] == 'b' &&
        cur(p)->start == rb->start + rb->length) {
        advance(p);
        n->type = N_BYTEARRAY;
    }
    n->as.list.items = elems.data;
    n->as.list.count = elems.count;
    return n;
}

static Node *parse_dict_literal(P *p) {
    Node *n = mknode(p, N_DICT);
    consume(p, TOK_LBRACE, "expected '{'");
    NodeVec keys = nv_new(), vals = nv_new();
    if (!check(p, TOK_RBRACE)) {
        do {
            if (check(p, TOK_RBRACE)) break;
            Node *k = parse_expression(p);
            consume(p, TOK_FAT_ARROW, "expected '=>' between dictionary key and value");
            Node *v = parse_expression(p);
            nv_push(p, &keys, k);
            nv_push(p, &vals, v);
        } while (match(p, TOK_COMMA) && !p->ast->had_error);
    }
    consume(p, TOK_RBRACE, "expected '}' to close dictionary");
    n->as.dict.keys = keys.data;
    n->as.dict.vals = vals.data;
    n->as.dict.count = keys.count;
    return n;
}

static Node *parse_new(P *p) {
    Node *n = mknode(p, N_NEW);
    consume(p, TOK_NEW, "expected 'new'");
    if (check(p, TOK_LBRACKET)) {
        /* new [size] — sized array creation (cls == NULL). */
        advance(p);
        NodeVec args = nv_new();
        nv_push(p, &args, parse_expression(p));
        consume(p, TOK_RBRACKET, "expected ']' after array size");
        n->as.newexpr.cls = NULL;
        n->as.newexpr.args = args.data;
        n->as.newexpr.argc = args.count;
        return n;
    }
    n->as.newexpr.cls = parse_type_ref(p);
    consume(p, TOK_LPAREN, "expected '(' after class name in 'new'");
    NodeVec args = nv_new();
    if (!check(p, TOK_RPAREN)) {
        do { nv_push(p, &args, parse_expression(p)); } while (match(p, TOK_COMMA) && !p->ast->had_error);
    }
    consume(p, TOK_RPAREN, "expected ')' after constructor arguments");
    n->as.newexpr.args = args.data;
    n->as.newexpr.argc = args.count;
    return n;
}

static Node *parse_primary(P *p) {
    Token *t = cur(p);
    switch (t->type) {
    case TOK_NUMBER: return lit_node(p, TOK_NUMBER);
    case TOK_LONG:   return lit_node(p, TOK_LONG);
    case TOK_FLOAT:  return lit_node(p, TOK_FLOAT);
    case TOK_DOUBLE: return lit_node(p, TOK_DOUBLE);
    case TOK_CHAR:   return lit_node(p, TOK_CHAR);
    case TOK_STRING: return lit_node(p, TOK_STRING);
    case TOK_TRUE:   return lit_node(p, TOK_TRUE);
    case TOK_FALSE:  return lit_node(p, TOK_FALSE);
    case TOK_NULL:   return lit_node(p, TOK_NULL);
    case TOK_SYMBOL: {
        Node *n = mknode(p, N_SYMBOL);
        n->as.name.name = pstrdup(p, t->start, t->length);
        n->as.name.len = t->length;
        advance(p);
        return n;
    }
    case TOK_IDENT: {
        Node *n = mknode(p, N_IDENT);
        n->as.name.name = pstrdup(p, t->start, t->length);
        n->as.name.len = t->length;
        advance(p);
        return n;
    }
    case TOK_SELF: case TOK_ME: { Node *n = mknode(p, N_SELF); advance(p); return n; }
    case TOK_LPAREN: {
        advance(p);
        Node *e = parse_expression(p);
        consume(p, TOK_RPAREN, "expected ')' after expression");
        return e;
    }
    case TOK_LBRACKET: return parse_array_literal(p);
    case TOK_LBRACE:   return parse_dict_literal(p);
    case TOK_NEW:      return parse_new(p);
    default: {
        perr(p, "expected an expression");
        Node *n = mknode(p, N_LITERAL);
        n->as.lit.kind = TOK_NULL;
        if (!at_end(p)) advance(p); /* ensure progress */
        return n;
    }
    }
}

static Node *parse_postfix(P *p) {
    Node *e = parse_primary(p);
    for (;;) {
        if (match(p, TOK_LPAREN)) {
            NodeVec args = nv_new();
            if (!check(p, TOK_RPAREN)) {
                do { nv_push(p, &args, parse_expression(p)); } while (match(p, TOK_COMMA) && !p->ast->had_error);
            }
            consume(p, TOK_RPAREN, "expected ')' after arguments");
            Node *n = mknode(p, N_CALL);
            n->as.call.callee = e; n->as.call.args = args.data; n->as.call.argc = args.count;
            e = n;
        } else if (match(p, TOK_LBRACKET)) {
            Node *idx = parse_expression(p);
            consume(p, TOK_RBRACKET, "expected ']' after index");
            Node *n = mknode(p, N_INDEX);
            n->as.index.object = e; n->as.index.index = idx;
            e = n;
        } else if (match(p, TOK_DOT)) {
            Token *nm = consume(p, TOK_IDENT, "expected member name after '.'");
            Node *n = mknode(p, N_MEMBER);
            n->as.member.object = e;
            n->as.member.name = pstrdup(p, nm->start, nm->length);
            n->as.member.len = nm->length;
            e = n;
        } else if (check(p, TOK_PLUS_PLUS) || check(p, TOK_MINUS_MINUS)) {
            TokenType op = cur(p)->type;
            Node *n = mknode(p, N_UNARY);
            advance(p);
            n->as.unary.op = op; n->as.unary.operand = e; n->as.unary.postfix = true;
            e = n;
        } else break;
    }
    return e;
}

static Node *parse_unary(P *p) {
    TokenType t = cur(p)->type;
    if (t == TOK_MINUS || t == TOK_BANG || t == TOK_TILDE || t == TOK_NOT ||
        t == TOK_PLUS_PLUS || t == TOK_MINUS_MINUS) {
        Node *n = mknode(p, N_UNARY);
        advance(p);
        n->as.unary.op = t;
        n->as.unary.operand = parse_unary(p);
        n->as.unary.postfix = false;
        return n;
    }
    return parse_postfix(p);
}

static int binprec(TokenType t) {
    switch (t) {
    case TOK_PIPE_PIPE: case TOK_OR:  return 1;
    case TOK_AMP_AMP:   case TOK_AND: return 2;
    case TOK_PIPE:  return 3;
    case TOK_CARET: return 4;
    case TOK_AMP:   return 5;
    case TOK_EQ: case TOK_NEQ: return 6;
    case TOK_LT: case TOK_GT: case TOK_LE: case TOK_GE:
    case TOK_INSTANCEOF: case TOK_HAS: return 7;
    case TOK_SHL: case TOK_SHR: return 8;
    case TOK_PLUS: case TOK_MINUS: return 9;
    case TOK_STAR: case TOK_SLASH: case TOK_PERCENT: return 10;
    default: return 0;
    }
}
static bool is_logical(TokenType t) {
    return t == TOK_AMP_AMP || t == TOK_PIPE_PIPE || t == TOK_AND || t == TOK_OR;
}

static Node *parse_binary(P *p, int minprec) {
    Node *left = parse_unary(p);
    for (;;) {
        TokenType op = cur(p)->type;
        int prec = binprec(op);
        if (prec == 0 || prec < minprec || p->ast->had_error) break;
        advance(p);
        if (op == TOK_INSTANCEOF) {
            Node *type = parse_unary(p);
            Node *n = mknode(p, N_INSTANCEOF);
            n->as.instof.expr = left; n->as.instof.type = type;
            left = n;
        } else if (op == TOK_HAS) {
            Node *sym = parse_unary(p);
            Node *n = mknode(p, N_HAS);
            n->as.has.expr = left; n->as.has.sym = sym;
            left = n;
        } else {
            Node *right = parse_binary(p, prec + 1);
            Node *n = mknode(p, is_logical(op) ? N_LOGICAL : N_BINARY);
            n->as.binary.op = op; n->as.binary.left = left; n->as.binary.right = right;
            left = n;
        }
    }
    return left;
}

static Node *parse_ternary(P *p) {
    Node *cond = parse_binary(p, 1);
    if (match(p, TOK_QUESTION)) {
        Node *then_e = parse_assignment(p);
        consume(p, TOK_COLON, "expected ':' in ternary expression");
        Node *else_e = parse_assignment(p);
        Node *n = mknode(p, N_TERNARY);
        n->as.ternary.cond = cond; n->as.ternary.then_e = then_e; n->as.ternary.else_e = else_e;
        return n;
    }
    return cond;
}

static bool is_assign_op(TokenType t) {
    switch (t) {
    case TOK_ASSIGN: case TOK_PLUS_EQ: case TOK_MINUS_EQ: case TOK_STAR_EQ:
    case TOK_SLASH_EQ: case TOK_PERCENT_EQ: case TOK_AMP_EQ: case TOK_PIPE_EQ:
    case TOK_CARET_EQ: case TOK_SHL_EQ: case TOK_SHR_EQ: return true;
    default: return false;
    }
}

static Node *parse_assignment(P *p) {
    Node *left = parse_ternary(p);
    if (is_assign_op(cur(p)->type)) {
        Token *optok = cur(p);
        TokenType op = optok->type;
        advance(p);
        Node *value = parse_assignment(p); /* right associative */
        if (!(left->type == N_IDENT || left->type == N_INDEX || left->type == N_MEMBER))
            error_at(p, optok, "invalid assignment target");
        Node *n = mknode(p, N_ASSIGN);
        n->as.assign.op = op; n->as.assign.target = left; n->as.assign.value = value;
        return n;
    }
    return left;
}

static Node *parse_expression(P *p) { return parse_assignment(p); }

/* ----------------------------------------------------------- type refs ----- */

static Node *parse_type_ref(P *p) {
    Token *t = consume(p, TOK_IDENT, "expected type name");
    Node *e = mknode(p, N_IDENT);
    e->as.name.name = pstrdup(p, t->start, t->length);
    e->as.name.len = t->length;
    while (match(p, TOK_DOT)) {
        Token *m = consume(p, TOK_IDENT, "expected name after '.'");
        Node *mm = mknode(p, N_MEMBER);
        mm->as.member.object = e;
        mm->as.member.name = pstrdup(p, m->start, m->length);
        mm->as.member.len = m->length;
        e = mm;
    }
    return e;
}

/* Skip an `as Type` annotation (ignored at runtime): dotted names, generic
 * `<...>`, and `or`/`Null` unions. */
static void skip_type(P *p) {
    for (;;) {
        if (check(p, TOK_IDENT) || check(p, TOK_NULL)) {
            advance(p);
            while (match(p, TOK_DOT)) { if (check(p, TOK_IDENT)) advance(p); else break; }
            if (check(p, TOK_LT)) {
                int depth = 0;
                do {
                    if (check(p, TOK_LT)) { depth++; advance(p); }
                    else if (check(p, TOK_SHL)) { depth += 2; advance(p); }
                    else if (check(p, TOK_GT)) { depth--; advance(p); }
                    else if (check(p, TOK_SHR)) { depth -= 2; advance(p); }
                    else if (at_end(p)) break;
                    else advance(p);
                } while (depth > 0);
            }
        } else break;
        if (match(p, TOK_OR)) continue; /* union: `A or B`, `X or Null` */
        break;
    }
}

/* ------------------------------------------------------------ statements --- */

static Node *parse_block(P *p) {
    Node *n = mknode(p, N_BLOCK);
    consume(p, TOK_LBRACE, "expected '{'");
    NodeVec v = nv_new();
    while (!check(p, TOK_RBRACE) && !at_end(p) && !p->ast->had_error)
        nv_push(p, &v, parse_statement(p));
    consume(p, TOK_RBRACE, "expected '}'");
    n->as.list.items = v.data;
    n->as.list.count = v.count;
    return n;
}

static Node *parse_var(P *p, unsigned mods, bool semi) {
    consume(p, TOK_VAR, "expected 'var'");
    NodeVec vars = nv_new();
    do {
        Node *v = mknode(p, N_VAR);
        Token *nm = consume(p, TOK_IDENT, "expected variable name");
        v->as.var.name = pstrdup(p, nm->start, nm->length);
        v->as.var.len = nm->length;
        v->as.var.modifiers = mods;
        if (match(p, TOK_AS)) skip_type(p);
        if (match(p, TOK_ASSIGN)) v->as.var.init = parse_expression(p);
        nv_push(p, &vars, v);
    } while (match(p, TOK_COMMA) && !p->ast->had_error);
    if (semi) consume(p, TOK_SEMICOLON, "expected ';' after variable declaration");
    if (vars.count == 1) return vars.data[0];
    Node *blk = mknode(p, N_BLOCK);
    blk->as.list.items = vars.data;
    blk->as.list.count = vars.count;
    return blk;
}

static Node *parse_if(P *p) {
    Node *n = mknode(p, N_IF);
    consume(p, TOK_IF, "expected 'if'");
    consume(p, TOK_LPAREN, "expected '(' after 'if'");
    n->as.ifs.cond = parse_expression(p);
    consume(p, TOK_RPAREN, "expected ')' after condition");
    n->as.ifs.then_s = parse_statement(p);
    if (match(p, TOK_ELSE)) n->as.ifs.else_s = parse_statement(p);
    return n;
}

static Node *parse_while(P *p) {
    Node *n = mknode(p, N_WHILE);
    consume(p, TOK_WHILE, "expected 'while'");
    consume(p, TOK_LPAREN, "expected '(' after 'while'");
    n->as.loop.cond = parse_expression(p);
    consume(p, TOK_RPAREN, "expected ')' after condition");
    n->as.loop.body = parse_statement(p);
    return n;
}

static Node *parse_dowhile(P *p) {
    Node *n = mknode(p, N_DOWHILE);
    consume(p, TOK_DO, "expected 'do'");
    n->as.loop.body = parse_statement(p);
    consume(p, TOK_WHILE, "expected 'while' after do-body");
    consume(p, TOK_LPAREN, "expected '(' after 'while'");
    n->as.loop.cond = parse_expression(p);
    consume(p, TOK_RPAREN, "expected ')' after condition");
    consume(p, TOK_SEMICOLON, "expected ';' after do-while");
    return n;
}

static Node *parse_for(P *p) {
    Node *n = mknode(p, N_FOR);
    consume(p, TOK_FOR, "expected 'for'");
    consume(p, TOK_LPAREN, "expected '(' after 'for'");
    /* init */
    if (check(p, TOK_SEMICOLON)) { advance(p); }
    else if (check(p, TOK_VAR)) { n->as.forl.init = parse_var(p, 0, true); }
    else {
        n->as.forl.init = parse_expression(p);
        consume(p, TOK_SEMICOLON, "expected ';' after for-initializer");
    }
    /* cond */
    if (!check(p, TOK_SEMICOLON)) n->as.forl.cond = parse_expression(p);
    consume(p, TOK_SEMICOLON, "expected ';' after for-condition");
    /* update */
    if (!check(p, TOK_RPAREN)) n->as.forl.update = parse_expression(p);
    consume(p, TOK_RPAREN, "expected ')' after for-clauses");
    n->as.forl.body = parse_statement(p);
    return n;
}

static Node *parse_switch(P *p) {
    Node *n = mknode(p, N_SWITCH);
    consume(p, TOK_SWITCH, "expected 'switch'");
    consume(p, TOK_LPAREN, "expected '(' after 'switch'");
    n->as.sw.expr = parse_expression(p);
    consume(p, TOK_RPAREN, "expected ')' after switch value");
    consume(p, TOK_LBRACE, "expected '{' to open switch body");
    NodeVec vals = nv_new(), bodies = nv_new();
    while (!check(p, TOK_RBRACE) && !at_end(p) && !p->ast->had_error) {
        Node *val = NULL;
        if (match(p, TOK_CASE)) {
            val = parse_expression(p);
            consume(p, TOK_COLON, "expected ':' after case value");
        } else if (match(p, TOK_DEFAULT)) {
            consume(p, TOK_COLON, "expected ':' after default");
        } else {
            perr(p, "expected 'case' or 'default' in switch");
            break;
        }
        Node *body = mknode(p, N_BLOCK);
        NodeVec stmts = nv_new();
        while (!check(p, TOK_CASE) && !check(p, TOK_DEFAULT) &&
               !check(p, TOK_RBRACE) && !at_end(p) && !p->ast->had_error)
            nv_push(p, &stmts, parse_statement(p));
        body->as.list.items = stmts.data;
        body->as.list.count = stmts.count;
        nv_push(p, &vals, val);
        nv_push(p, &bodies, body);
    }
    consume(p, TOK_RBRACE, "expected '}' to close switch");
    n->as.sw.case_vals = vals.data;
    n->as.sw.case_bodies = bodies.data;
    n->as.sw.count = vals.count;
    return n;
}

static Node *parse_try(P *p) {
    Node *n = mknode(p, N_TRY);
    consume(p, TOK_TRY, "expected 'try'");
    n->as.tryc.block = parse_block(p);
    StrVec names = sv_new();
    NodeVec types = nv_new(), bodies = nv_new();
    while (check(p, TOK_CATCH)) {
        advance(p);
        consume(p, TOK_LPAREN, "expected '(' after 'catch'");
        char *cname = NULL; int clen = 0;
        Node *ctype = NULL;
        if (check(p, TOK_IDENT)) {
            Token *t = advance(p);
            cname = pstrdup(p, t->start, t->length); clen = t->length;
            if (match(p, TOK_INSTANCEOF)) ctype = parse_type_ref(p);
        }
        consume(p, TOK_RPAREN, "expected ')' after catch parameter");
        Node *cbody = parse_block(p);
        sv_push(p, &names, cname, clen);
        nv_push(p, &types, ctype);
        nv_push(p, &bodies, cbody);
    }
    if (match(p, TOK_FINALLY)) n->as.tryc.finally_block = parse_block(p);
    n->as.tryc.catch_names = names.names;
    n->as.tryc.catch_name_lens = names.lens;
    n->as.tryc.catch_types = types.data;
    n->as.tryc.catch_bodies = bodies.data;
    n->as.tryc.catch_count = types.count;
    return n;
}

static Node *parse_statement(P *p) {
    switch (cur(p)->type) {
    case TOK_LBRACE:  return parse_block(p);
    case TOK_IF:      return parse_if(p);
    case TOK_WHILE:   return parse_while(p);
    case TOK_DO:      return parse_dowhile(p);
    case TOK_FOR:     return parse_for(p);
    case TOK_SWITCH:  return parse_switch(p);
    case TOK_TRY:     return parse_try(p);
    case TOK_VAR:     return parse_var(p, 0, true);
    case TOK_RETURN: {
        Node *n = mknode(p, N_RETURN);
        advance(p);
        if (!check(p, TOK_SEMICOLON)) n->as.expr.expr = parse_expression(p);
        consume(p, TOK_SEMICOLON, "expected ';' after return");
        return n;
    }
    case TOK_BREAK: {
        Node *n = mknode(p, N_BREAK); advance(p);
        consume(p, TOK_SEMICOLON, "expected ';' after break");
        return n;
    }
    case TOK_CONTINUE: {
        Node *n = mknode(p, N_CONTINUE); advance(p);
        consume(p, TOK_SEMICOLON, "expected ';' after continue");
        return n;
    }
    case TOK_THROW: {
        Node *n = mknode(p, N_THROW); advance(p);
        n->as.expr.expr = parse_expression(p);
        consume(p, TOK_SEMICOLON, "expected ';' after throw");
        return n;
    }
    default: {
        Node *n = mknode(p, N_EXPR_STMT);
        n->as.expr.expr = parse_expression(p);
        consume(p, TOK_SEMICOLON, "expected ';' after expression");
        return n;
    }
    }
}

/* ---------------------------------------------------------- declarations --- */

static unsigned parse_modifiers(P *p) {
    unsigned m = 0;
    for (;;) {
        switch (cur(p)->type) {
        case TOK_PUBLIC:    m |= MOD_PUBLIC; break;
        case TOK_PRIVATE:   m |= MOD_PRIVATE; break;
        case TOK_PROTECTED: m |= MOD_PROTECTED; break;
        case TOK_HIDDEN:    m |= MOD_HIDDEN; break;
        case TOK_STATIC:    m |= MOD_STATIC; break;
        case TOK_NATIVE:    m |= MOD_NATIVE; break;
        default: return m;
        }
        advance(p);
    }
}

static Node *parse_function(P *p, unsigned mods) {
    Node *n = mknode(p, N_FUNCTION);
    consume(p, TOK_FUNCTION, "expected 'function'");
    Token *name = consume(p, TOK_IDENT, "expected function name");
    n->as.func.name = pstrdup(p, name->start, name->length);
    n->as.func.len = name->length;
    n->as.func.modifiers = mods;
    consume(p, TOK_LPAREN, "expected '(' after function name");
    StrVec params = sv_new();
    if (!check(p, TOK_RPAREN)) {
        do {
            Token *pn = consume(p, TOK_IDENT, "expected parameter name");
            sv_push(p, &params, pstrdup(p, pn->start, pn->length), pn->length);
            if (match(p, TOK_AS)) skip_type(p);
        } while (match(p, TOK_COMMA) && !p->ast->had_error);
    }
    consume(p, TOK_RPAREN, "expected ')' after parameters");
    if (match(p, TOK_AS)) skip_type(p); /* return type */
    n->as.func.params = params.names;
    n->as.func.param_lens = params.lens;
    n->as.func.param_count = params.count;
    if (check(p, TOK_LBRACE)) n->as.func.body = parse_block(p);
    else consume(p, TOK_SEMICOLON, "expected '{' or ';' for function body");
    return n;
}

static Node *parse_class(P *p, unsigned mods) {
    Node *n = mknode(p, N_CLASS);
    consume(p, TOK_CLASS, "expected 'class'");
    Token *name = consume(p, TOK_IDENT, "expected class name");
    n->as.cls.name = pstrdup(p, name->start, name->length);
    n->as.cls.len = name->length;
    n->as.cls.modifiers = mods;
    if (match(p, TOK_EXTENDS)) n->as.cls.super = parse_type_ref(p);
    consume(p, TOK_LBRACE, "expected '{' to open class body");
    NodeVec members = nv_new();
    while (!check(p, TOK_RBRACE) && !at_end(p) && !p->ast->had_error)
        nv_push(p, &members, parse_declaration(p));
    consume(p, TOK_RBRACE, "expected '}' to close class body");
    n->as.cls.members = members.data;
    n->as.cls.count = members.count;
    return n;
}

static Node *parse_module(P *p, unsigned mods) {
    (void)mods;
    Node *n = mknode(p, N_MODULE);
    consume(p, TOK_MODULE, "expected 'module'");
    Token *name = consume(p, TOK_IDENT, "expected module name");
    n->as.mod.name = pstrdup(p, name->start, name->length);
    n->as.mod.len = name->length;
    consume(p, TOK_LBRACE, "expected '{' to open module body");
    NodeVec members = nv_new();
    while (!check(p, TOK_RBRACE) && !at_end(p) && !p->ast->had_error)
        nv_push(p, &members, parse_declaration(p));
    consume(p, TOK_RBRACE, "expected '}' to close module body");
    n->as.mod.members = members.data;
    n->as.mod.count = members.count;
    return n;
}

static Node *parse_const(P *p, unsigned mods) {
    Node *n = mknode(p, N_CONST);
    consume(p, TOK_CONST, "expected 'const'");
    Token *name = consume(p, TOK_IDENT, "expected constant name");
    n->as.var.name = pstrdup(p, name->start, name->length);
    n->as.var.len = name->length;
    n->as.var.modifiers = mods;
    if (match(p, TOK_AS)) skip_type(p);
    consume(p, TOK_ASSIGN, "expected '=' in const declaration");
    n->as.var.init = parse_expression(p);
    consume(p, TOK_SEMICOLON, "expected ';' after const declaration");
    return n;
}

static Node *parse_enum(P *p, unsigned mods) {
    (void)mods;
    Node *n = mknode(p, N_ENUM);
    consume(p, TOK_ENUM, "expected 'enum'");
    if (check(p, TOK_IDENT)) advance(p); /* optional enum name (ignored) */
    consume(p, TOK_LBRACE, "expected '{' to open enum");
    StrVec names = sv_new();
    NodeVec values = nv_new();
    while (!check(p, TOK_RBRACE) && !at_end(p) && !p->ast->had_error) {
        Token *nm = consume(p, TOK_IDENT, "expected enum member name");
        Node *val = NULL;
        if (match(p, TOK_ASSIGN)) val = parse_expression(p);
        sv_push(p, &names, pstrdup(p, nm->start, nm->length), nm->length);
        nv_push(p, &values, val);
        if (!match(p, TOK_COMMA)) break;
    }
    consume(p, TOK_RBRACE, "expected '}' to close enum");
    n->as.enm.names = names.names;
    n->as.enm.name_lens = names.lens;
    n->as.enm.values = values.data;
    n->as.enm.count = names.count;
    return n;
}

static Node *parse_using(P *p) {
    Node *n = mknode(p, N_USING);
    advance(p); /* 'using' or 'import' */
    StrVec path = sv_new();
    Token *t = consume(p, TOK_IDENT, "expected module path");
    sv_push(p, &path, pstrdup(p, t->start, t->length), t->length);
    while (match(p, TOK_DOT)) {
        t = consume(p, TOK_IDENT, "expected name after '.'");
        sv_push(p, &path, pstrdup(p, t->start, t->length), t->length);
    }
    if (match(p, TOK_AS)) {
        Token *a = consume(p, TOK_IDENT, "expected alias after 'as'");
        n->as.use.alias = pstrdup(p, a->start, a->length);
        n->as.use.alias_len = a->length;
    }
    consume(p, TOK_SEMICOLON, "expected ';' after using declaration");
    n->as.use.path = path.names;
    n->as.use.path_lens = path.lens;
    n->as.use.path_count = path.count;
    return n;
}

static Node *parse_declaration(P *p) {
    unsigned mods = parse_modifiers(p);
    switch (cur(p)->type) {
    case TOK_FUNCTION: return parse_function(p, mods);
    case TOK_CLASS:    return parse_class(p, mods);
    case TOK_MODULE:   return parse_module(p, mods);
    case TOK_VAR:      return parse_var(p, mods, true);
    case TOK_CONST:    return parse_const(p, mods);
    case TOK_ENUM:     return parse_enum(p, mods);
    case TOK_USING:
    case TOK_IMPORT:   return parse_using(p);
    default:
        if (mods) {
            perr(p, "expected a declaration after modifiers");
            Node *n = mknode(p, N_LITERAL);
            n->as.lit.kind = TOK_NULL;
            if (!at_end(p)) advance(p);
            return n;
        }
        /* Script-style top level: allow statements. */
        return parse_statement(p);
    }
}

/* ------------------------------------------------------------ entry points */

Ast *parse(TokenList *tokens) {
    Ast *ast = (Ast *)vm_alloc(sizeof(Ast));
    memset(ast, 0, sizeof(*ast));

    P p = { tokens->tokens, tokens->count, 0, ast };
    Node *prog = mknode(&p, N_PROGRAM);
    NodeVec items = nv_new();
    while (!at_end(&p) && !ast->had_error)
        nv_push(&p, &items, parse_declaration(&p));
    prog->as.list.items = items.data;
    prog->as.list.count = items.count;
    ast->root = prog;
    return ast;
}

Ast *parse_source(const char *source) {
    TokenList *tl = lex(source);
    if (tl->had_error) {
        Ast *ast = (Ast *)vm_alloc(sizeof(Ast));
        memset(ast, 0, sizeof(*ast));
        ast->had_error = true;
        for (int i = 0; i < tl->count; i++) {
            if (tl->tokens[i].type == TOK_ERROR) {
                snprintf(ast->error_msg, sizeof(ast->error_msg), "lex error: %s",
                         tl->tokens[i].str ? tl->tokens[i].str : "?");
                ast->error_line = tl->tokens[i].line;
                ast->error_col = tl->tokens[i].col;
                break;
            }
        }
        lex_free(tl);
        return ast;
    }
    Ast *ast = parse(tl);
    lex_free(tl);
    return ast;
}
