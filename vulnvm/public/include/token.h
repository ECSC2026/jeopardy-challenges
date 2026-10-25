/* token.h — MonkeyC token kinds and the Token structure produced by the lexer. */
#ifndef VULNVM_TOKEN_H
#define VULNVM_TOKEN_H

#include "common.h"

typedef enum {
    /* literals */
    TOK_NUMBER,   /* 32-bit int literal            */
    TOK_LONG,     /* 64-bit int literal (l/L)      */
    TOK_FLOAT,    /* 32-bit float literal          */
    TOK_DOUBLE,   /* 64-bit double literal (d/D)   */
    TOK_STRING,   /* "..." (decoded into str)      */
    TOK_CHAR,     /* '...' code point (in num.ch)  */
    TOK_SYMBOL,   /* :name (start/length = name)   */
    TOK_IDENT,    /* identifier                    */

    /* keywords */
    TOK_VAR, TOK_CONST, TOK_FUNCTION, TOK_CLASS, TOK_MODULE, TOK_ENUM,
    TOK_USING, TOK_IMPORT, TOK_AS, TOK_NATIVE, TOK_STATIC,
    TOK_PUBLIC, TOK_PRIVATE, TOK_PROTECTED, TOK_HIDDEN,
    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_DO,
    TOK_RETURN, TOK_BREAK, TOK_CONTINUE,
    TOK_SWITCH, TOK_CASE, TOK_DEFAULT,
    TOK_TRY, TOK_CATCH, TOK_FINALLY, TOK_THROW,
    TOK_NEW, TOK_SELF, TOK_ME, TOK_NULL, TOK_TRUE, TOK_FALSE,
    TOK_AND, TOK_OR, TOK_NOT, TOK_INSTANCEOF, TOK_HAS, TOK_EXTENDS,

    /* operators & punctuation */
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_EQ, TOK_NEQ, TOK_LT, TOK_GT, TOK_LE, TOK_GE,
    TOK_AMP_AMP, TOK_PIPE_PIPE, TOK_BANG,
    TOK_AMP, TOK_PIPE, TOK_CARET, TOK_TILDE, TOK_SHL, TOK_SHR,
    TOK_ASSIGN, TOK_PLUS_EQ, TOK_MINUS_EQ, TOK_STAR_EQ, TOK_SLASH_EQ,
    TOK_PERCENT_EQ, TOK_AMP_EQ, TOK_PIPE_EQ, TOK_CARET_EQ, TOK_SHL_EQ, TOK_SHR_EQ,
    TOK_PLUS_PLUS, TOK_MINUS_MINUS,
    TOK_QUESTION, TOK_COLON,
    TOK_DOT, TOK_COMMA, TOK_SEMICOLON,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACKET, TOK_RBRACKET, TOK_LBRACE, TOK_RBRACE,
    TOK_FAT_ARROW, /* => (dictionary) */

    /* special */
    TOK_ERROR,     /* lexing error; str holds the message */
    TOK_EOF,
    TOK__COUNT
} TokenType;

typedef struct {
    TokenType   type;
    const char *start;  /* pointer into the source buffer                    */
    int         length; /* length of the raw lexeme (name only for symbols)  */
    int         line;   /* 1-based line                                      */
    int         col;    /* 1-based column of `start`                         */
    union {
        int32_t  i32;
        int64_t  i64;
        float    f32;
        double   f64;
        uint32_t ch;    /* TOK_CHAR code point */
    } num;
    char *str;          /* TOK_STRING decoded bytes / TOK_ERROR message      */
    int   str_len;      /* decoded byte length (excludes trailing NUL)       */
} Token;

/* Human-readable name of a token type (for --tokens dumps and tests). */
const char *token_type_name(TokenType t);

#endif /* VULNVM_TOKEN_H */
