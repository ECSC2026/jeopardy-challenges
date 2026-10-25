/* lexer.c — MonkeyC lexer. */
#include "lexer.h"
#include "alloc.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* ------------------------------------------------------- token type names -- */

static const char *const kTokNames[TOK__COUNT] = {
    "NUMBER", "LONG", "FLOAT", "DOUBLE", "STRING", "CHAR", "SYMBOL", "IDENT",
    "var", "const", "function", "class", "module", "enum",
    "using", "import", "as", "native", "static",
    "public", "private", "protected", "hidden",
    "if", "else", "while", "for", "do",
    "return", "break", "continue",
    "switch", "case", "default",
    "try", "catch", "finally", "throw",
    "new", "self", "me", "null", "true", "false",
    "and", "or", "not", "instanceof", "has", "extends",
    "+", "-", "*", "/", "%",
    "==", "!=", "<", ">", "<=", ">=",
    "&&", "||", "!",
    "&", "|", "^", "~", "<<", ">>",
    "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
    "++", "--",
    "?", ":",
    ".", ",", ";",
    "(", ")", "[", "]", "{", "}",
    "=>",
    "ERROR", "EOF",
};

const char *token_type_name(TokenType t) {
    if ((unsigned)t >= TOK__COUNT) return "?";
    return kTokNames[t];
}

/* -------------------------------------------------------- keyword table ---- */

typedef struct { const char *word; TokenType type; } Keyword;
static const Keyword kKeywords[] = {
    {"var", TOK_VAR}, {"const", TOK_CONST}, {"function", TOK_FUNCTION},
    {"class", TOK_CLASS}, {"module", TOK_MODULE}, {"enum", TOK_ENUM},
    {"using", TOK_USING}, {"import", TOK_IMPORT}, {"as", TOK_AS},
    {"native", TOK_NATIVE}, {"static", TOK_STATIC},
    {"public", TOK_PUBLIC}, {"private", TOK_PRIVATE},
    {"protected", TOK_PROTECTED}, {"hidden", TOK_HIDDEN},
    {"if", TOK_IF}, {"else", TOK_ELSE}, {"while", TOK_WHILE},
    {"for", TOK_FOR}, {"do", TOK_DO},
    {"return", TOK_RETURN}, {"break", TOK_BREAK}, {"continue", TOK_CONTINUE},
    {"switch", TOK_SWITCH}, {"case", TOK_CASE}, {"default", TOK_DEFAULT},
    {"try", TOK_TRY}, {"catch", TOK_CATCH}, {"finally", TOK_FINALLY},
    {"throw", TOK_THROW},
    {"new", TOK_NEW}, {"self", TOK_SELF}, {"me", TOK_ME}, {"null", TOK_NULL},
    {"true", TOK_TRUE}, {"false", TOK_FALSE},
    {"and", TOK_AND}, {"or", TOK_OR}, {"not", TOK_NOT},
    {"instanceof", TOK_INSTANCEOF}, {"has", TOK_HAS}, {"extends", TOK_EXTENDS},
    {NULL, TOK_EOF},
};

static TokenType keyword_lookup(const char *s, int len) {
    for (const Keyword *k = kKeywords; k->word; k++) {
        if ((int)strlen(k->word) == len && memcmp(k->word, s, (size_t)len) == 0)
            return k->type;
    }
    return TOK_IDENT;
}

/* ------------------------------------------------------------- scanner ----- */

typedef struct {
    const char *src;    /* start of source           */
    const char *cur;    /* current position          */
    const char *tok;    /* start of current token    */
    const char *line_start;
    int         line;
    TokenList  *out;
} Lexer;

static bool is_digit(int c)  { return c >= '0' && c <= '9'; }
static bool is_hex(int c)    { return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
static bool is_alpha(int c)  { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static bool is_alnum(int c)  { return is_alpha(c) || is_digit(c); }

static char peek(Lexer *L)      { return *L->cur; }
static char peek_next(Lexer *L) { return L->cur[0] ? L->cur[1] : '\0'; }
static bool at_end(Lexer *L)    { return *L->cur == '\0'; }
static char advance(Lexer *L)   { return *L->cur++; }
static bool match(Lexer *L, char c) {
    if (*L->cur != c) return false;
    L->cur++;
    return true;
}

static int cur_col(Lexer *L)  { return (int)(L->tok - L->line_start) + 1; }

/* Track a decoded/allocated buffer for cleanup at lex_free. */
static void own_buffer(TokenList *tl, char *buf) {
    if (tl->owned_count + 1 > tl->owned_cap) {
        int ncap = tl->owned_cap < 8 ? 8 : tl->owned_cap * 2;
        tl->owned = (char **)vm_realloc(tl->owned, (size_t)ncap * sizeof(char *));
        tl->owned_cap = ncap;
    }
    tl->owned[tl->owned_count++] = buf;
}

static Token *emit(Lexer *L, TokenType type) {
    TokenList *tl = L->out;
    if (tl->count + 1 > tl->capacity) {
        int ncap = tl->capacity < 16 ? 16 : tl->capacity * 2;
        tl->tokens = (Token *)vm_realloc(tl->tokens, (size_t)ncap * sizeof(Token));
        tl->capacity = ncap;
    }
    Token *t = &tl->tokens[tl->count++];
    memset(t, 0, sizeof(*t));
    t->type = type;
    t->start = L->tok;
    t->length = (int)(L->cur - L->tok);
    t->line = L->line;
    t->col = cur_col(L);
    return t;
}

static void emit_error(Lexer *L, const char *msg) {
    Token *t = emit(L, TOK_ERROR);
    size_t n = strlen(msg);
    char *buf = (char *)vm_alloc(n + 1);
    memcpy(buf, msg, n + 1);
    own_buffer(L->out, buf);
    t->str = buf;
    t->str_len = (int)n;
    L->out->had_error = true;
}

/* ------------------------------------------------ whitespace & comments ---- */

/* Returns false if an unterminated block comment was hit. */
static bool skip_trivia(Lexer *L) {
    for (;;) {
        char c = peek(L);
        switch (c) {
        case ' ': case '\t': case '\r':
            advance(L);
            break;
        case '\n':
            advance(L);
            L->line++;
            L->line_start = L->cur;
            break;
        case '/':
            if (peek_next(L) == '/') {
                while (!at_end(L) && peek(L) != '\n') advance(L);
            } else if (peek_next(L) == '*') {
                advance(L); advance(L); /* consume the opener */
                while (!at_end(L) && !(peek(L) == '*' && peek_next(L) == '/')) {
                    if (peek(L) == '\n') { L->line++; advance(L); L->line_start = L->cur; }
                    else advance(L);
                }
                if (at_end(L)) return false; /* unterminated */
                advance(L); advance(L); /* consume */
            } else {
                return true;
            }
            break;
        default:
            return true;
        }
    }
}

/* ------------------------------------------------------------ literals ----- */

/* Decode a backslash escape (cur points just past the backslash). Writes the
 * decoded byte(s) via `emit_byte`. Returns false on an invalid escape. */
static bool decode_escape(Lexer *L, char *out, int *out_len, char *errbuf) {
    if (at_end(L)) { strcpy(errbuf, "unterminated escape sequence"); return false; }
    char e = advance(L);
    switch (e) {
    case 'n':  out[0] = '\n'; *out_len = 1; return true;
    case 't':  out[0] = '\t'; *out_len = 1; return true;
    case 'r':  out[0] = '\r'; *out_len = 1; return true;
    case 'b':  out[0] = '\b'; *out_len = 1; return true;
    case 'f':  out[0] = '\f'; *out_len = 1; return true;
    case '0':  out[0] = '\0'; *out_len = 1; return true;
    case '\\': out[0] = '\\'; *out_len = 1; return true;
    case '\'': out[0] = '\''; *out_len = 1; return true;
    case '"':  out[0] = '"';  *out_len = 1; return true;
    case 'x': case 'X': {
        int v = 0, n = 0;
        while (n < 2 && is_hex(peek(L))) {
            char h = advance(L);
            int d = is_digit(h) ? h - '0' : (h | 0x20) - 'a' + 10;
            v = v * 16 + d;
            n++;
        }
        if (n == 0) { strcpy(errbuf, "invalid \\x escape: expected hex digit"); return false; }
        out[0] = (char)v; *out_len = 1; return true;
    }
    default:
        snprintf(errbuf, 64, "invalid escape '\\%c'", e);
        return false;
    }
}

static void scan_string(Lexer *L) {
    /* opening quote already consumed */
    size_t cap = 16, len = 0;
    char *buf = (char *)vm_alloc(cap);
    char errbuf[80];

    while (!at_end(L) && peek(L) != '"') {
        char c = peek(L);
        char decoded[4]; int dlen = 1;
        if (c == '\n') {
            /* allow multi-line? MonkeyC strings are single line; error. */
            emit_error(L, "unterminated string literal");
            vm_free(buf);
            return;
        }
        if (c == '\\') {
            advance(L);
            if (!decode_escape(L, decoded, &dlen, errbuf)) {
                emit_error(L, errbuf);
                vm_free(buf);
                return;
            }
        } else {
            decoded[0] = advance(L);
            dlen = 1;
        }
        if (len + (size_t)dlen + 1 > cap) {
            while (len + (size_t)dlen + 1 > cap) cap *= 2;
            buf = (char *)vm_realloc(buf, cap);
        }
        memcpy(buf + len, decoded, (size_t)dlen);
        len += (size_t)dlen;
    }

    if (at_end(L)) {
        emit_error(L, "unterminated string literal");
        vm_free(buf);
        return;
    }
    advance(L); /* closing quote */
    buf[len] = '\0';

    Token *t = emit(L, TOK_STRING);
    own_buffer(L->out, buf);
    t->str = buf;
    t->str_len = (int)len;
}

static void scan_char(Lexer *L) {
    /* opening quote already consumed */
    char errbuf[80];
    if (at_end(L) || peek(L) == '\'') { emit_error(L, "empty char literal"); return; }

    uint32_t cp;
    if (peek(L) == '\\') {
        advance(L);
        char decoded[4]; int dlen = 1;
        if (!decode_escape(L, decoded, &dlen, errbuf)) { emit_error(L, errbuf); return; }
        cp = (unsigned char)decoded[0];
    } else {
        /* decode one UTF-8 code point */
        unsigned char c0 = (unsigned char)advance(L);
        int extra;
        if (c0 < 0x80)             { cp = c0;        extra = 0; }
        else if ((c0 & 0xE0) == 0xC0) { cp = c0 & 0x1F; extra = 1; }
        else if ((c0 & 0xF0) == 0xE0) { cp = c0 & 0x0F; extra = 2; }
        else if ((c0 & 0xF8) == 0xF0) { cp = c0 & 0x07; extra = 3; }
        else { emit_error(L, "invalid character in char literal"); return; }
        for (int i = 0; i < extra; i++) {
            if (at_end(L) || (peek(L) & 0xC0) != 0x80) {
                emit_error(L, "truncated UTF-8 in char literal");
                return;
            }
            cp = (cp << 6) | (advance(L) & 0x3F);
        }
    }

    if (peek(L) != '\'') { emit_error(L, "unterminated char literal (expected ')"); return; }
    advance(L);
    Token *t = emit(L, TOK_CHAR);
    t->num.ch = cp;
}

static void scan_number(Lexer *L) {
    /* L->tok points at first digit. Determine form. */
    bool is_hex_lit = false, is_float = false;
    if (L->cur[-1] == '0' && (peek(L) == 'x' || peek(L) == 'X')) {
        is_hex_lit = true;
        advance(L); /* x */
        int hexdigits = 0;
        while (is_hex(peek(L))) { advance(L); hexdigits++; }
        if (hexdigits == 0) { emit_error(L, "hex literal '0x' has no digits"); return; }
    } else {
        while (is_digit(peek(L))) advance(L);
        if (peek(L) == '.' && is_digit(peek_next(L))) {
            is_float = true;
            advance(L);
            while (is_digit(peek(L))) advance(L);
        }
        if (peek(L) == 'e' || peek(L) == 'E') {
            char n1 = peek_next(L);
            if (is_digit(n1) || ((n1 == '+' || n1 == '-') && is_digit(L->cur[2]))) {
                is_float = true;
                advance(L);                       /* e */
                if (peek(L) == '+' || peek(L) == '-') advance(L);
                while (is_digit(peek(L))) advance(L);
            }
        }
    }

    /* optional type suffix */
    TokenType type;
    bool is_long = false, is_double = false;
    char suf = peek(L);
    if (suf == 'l' || suf == 'L') { is_long = true; advance(L); }
    else if (suf == 'd' || suf == 'D') { is_double = true; advance(L); }

    /* copy digits (without suffix) into a NUL-terminated scratch for parsing */
    int rawlen = (int)(L->cur - L->tok);
    int vallen = rawlen - ((is_long || is_double) ? 1 : 0);
    char scratch[64];
    if (vallen >= (int)sizeof(scratch)) { emit_error(L, "numeric literal too long"); return; }
    memcpy(scratch, L->tok, (size_t)vallen);
    scratch[vallen] = '\0';

    if (is_float || (is_double && !is_hex_lit)) {
        double d = strtod(scratch, NULL);
        if (is_double) { type = TOK_DOUBLE; Token *t = emit(L, type); t->num.f64 = d; }
        else           { type = TOK_FLOAT;  Token *t = emit(L, type); t->num.f32 = (float)d; }
    } else {
        /* integer (decimal or hex) */
        errno = 0;
        long long v;
        if (is_hex_lit) {
            /* Parse hex as unsigned so a full 64-bit pattern (e.g.
             * 0xFFFFFFFFFFFFFFFF) is preserved instead of being clamped by signed
             * overflow. A hex constant that needs more than 32 bits is promoted to
             * Long automatically, so large literals aren't silently truncated to a
             * 32-bit Number (a value like 0x640000006400 keeps all 8 bytes). */
            unsigned long long uv = strtoull(scratch, NULL, 16);
            if (!is_long && uv > 0xFFFFFFFFULL) is_long = true;
            v = (long long)uv;
        } else {
            v = strtoll(scratch, NULL, 10);
        }
        if (is_long) { type = TOK_LONG; Token *t = emit(L, type); t->num.i64 = (int64_t)v; }
        else { type = TOK_NUMBER; Token *t = emit(L, type); t->num.i32 = (int32_t)v; }
    }
}

static void scan_identifier(Lexer *L) {
    while (is_alnum(peek(L))) advance(L);
    int len = (int)(L->cur - L->tok);
    TokenType kw = keyword_lookup(L->tok, len);
    emit(L, kw);
}

static void scan_symbol(Lexer *L) {
    /* colon already consumed; must be followed by an identifier */
    if (!is_alpha(peek(L))) { emit_error(L, "expected identifier after ':' in symbol"); return; }
    const char *name = L->cur;
    while (is_alnum(peek(L))) advance(L);
    Token *t = emit(L, TOK_SYMBOL);
    /* start/length point at the name (without the colon) */
    t->start = name;
    t->length = (int)(L->cur - name);
}

/* ---------------------------------------------------------- main loop ------ */

static void scan_token(Lexer *L) {
    char c = advance(L);
    switch (c) {
    case '(': emit(L, TOK_LPAREN); return;
    case ')': emit(L, TOK_RPAREN); return;
    case '[': emit(L, TOK_LBRACKET); return;
    case ']': emit(L, TOK_RBRACKET); return;
    case '{': emit(L, TOK_LBRACE); return;
    case '}': emit(L, TOK_RBRACE); return;
    case ',': emit(L, TOK_COMMA); return;
    case ';': emit(L, TOK_SEMICOLON); return;
    case '.': emit(L, TOK_DOT); return;
    case '~': emit(L, TOK_TILDE); return;
    case '?': emit(L, TOK_QUESTION); return;
    case '+': emit(L, match(L, '+') ? TOK_PLUS_PLUS : match(L, '=') ? TOK_PLUS_EQ : TOK_PLUS); return;
    case '-': emit(L, match(L, '-') ? TOK_MINUS_MINUS : match(L, '=') ? TOK_MINUS_EQ : TOK_MINUS); return;
    case '*': emit(L, match(L, '=') ? TOK_STAR_EQ : TOK_STAR); return;
    case '/': emit(L, match(L, '=') ? TOK_SLASH_EQ : TOK_SLASH); return;
    case '%': emit(L, match(L, '=') ? TOK_PERCENT_EQ : TOK_PERCENT); return;
    case '^': emit(L, match(L, '=') ? TOK_CARET_EQ : TOK_CARET); return;
    case '!': emit(L, match(L, '=') ? TOK_NEQ : TOK_BANG); return;
    case '=': emit(L, match(L, '=') ? TOK_EQ : match(L, '>') ? TOK_FAT_ARROW : TOK_ASSIGN); return;
    case '&': emit(L, match(L, '&') ? TOK_AMP_AMP : match(L, '=') ? TOK_AMP_EQ : TOK_AMP); return;
    case '|': emit(L, match(L, '|') ? TOK_PIPE_PIPE : match(L, '=') ? TOK_PIPE_EQ : TOK_PIPE); return;
    case '<':
        if (match(L, '<')) { emit(L, match(L, '=') ? TOK_SHL_EQ : TOK_SHL); return; }
        emit(L, match(L, '=') ? TOK_LE : TOK_LT); return;
    case '>':
        if (match(L, '>')) { emit(L, match(L, '=') ? TOK_SHR_EQ : TOK_SHR); return; }
        emit(L, match(L, '=') ? TOK_GE : TOK_GT); return;
    case ':':
        /* `:name` is a symbol; a bare `:` is the ternary/standalone colon. */
        if (is_alpha(peek(L))) scan_symbol(L);
        else emit(L, TOK_COLON);
        return;
    case '"': scan_string(L); return;
    case '\'': scan_char(L); return;
    default:
        if (is_digit(c)) { scan_number(L); return; }
        if (is_alpha(c)) { scan_identifier(L); return; }
        { char m[48]; snprintf(m, sizeof(m), "unexpected character '%c' (0x%02x)",
                               (c >= 32 && c < 127) ? c : '?', (unsigned char)c);
          emit_error(L, m); return; }
    }
}

/* ---------------------------------------------------------- public API ----- */

TokenList *lex(const char *source) {
    TokenList *tl = (TokenList *)vm_alloc(sizeof(TokenList));
    memset(tl, 0, sizeof(*tl));

    Lexer L;
    L.src = L.cur = L.line_start = source;
    L.tok = source;
    L.line = 1;
    L.out = tl;

    for (;;) {
        if (!skip_trivia(&L)) {
            L.tok = L.cur;
            emit_error(&L, "unterminated block comment");
            break;
        }
        L.tok = L.cur;
        if (at_end(&L)) break;
        scan_token(&L);
    }
    L.tok = L.cur;
    emit(&L, TOK_EOF);
    return tl;
}

void lex_free(TokenList *tl) {
    if (!tl) return;
    for (int i = 0; i < tl->owned_count; i++) vm_free(tl->owned[i]);
    if (tl->owned)  vm_free(tl->owned);
    if (tl->tokens) vm_free(tl->tokens);
    vm_free(tl);
}

void lex_dump(const TokenList *tl) {
    for (int i = 0; i < tl->count; i++) {
        const Token *t = &tl->tokens[i];
        printf("%4d:%-3d  %-10s", t->line, t->col, token_type_name(t->type));
        switch (t->type) {
        case TOK_NUMBER: printf("  %d", t->num.i32); break;
        case TOK_LONG:   printf("  %lldL", (long long)t->num.i64); break;
        case TOK_FLOAT:  printf("  %g", (double)t->num.f32); break;
        case TOK_DOUBLE: printf("  %gD", t->num.f64); break;
        case TOK_CHAR:   printf("  U+%04X", t->num.ch); break;
        case TOK_STRING: printf("  \"%.*s\"", t->str_len, t->str ? t->str : ""); break;
        case TOK_SYMBOL: printf("  :%.*s", t->length, t->start); break;
        case TOK_IDENT:  printf("  %.*s", t->length, t->start); break;
        case TOK_ERROR:  printf("  <%s>", t->str ? t->str : "?"); break;
        default: break;
        }
        printf("\n");
    }
}
