/* lexer.h — MonkeyC lexer: turns source text into a Token list.
 *
 * The lexer scans the whole source into a growable array terminated by a single
 * TOK_EOF. On a lexing error it appends a TOK_ERROR token (whose `str` holds the
 * message) and continues, so multiple errors can be reported; `had_error` is set.
 * All storage (the token array and decoded string/error buffers) comes from the
 * custom arena and is released by lex_free.
 */
#ifndef VULNVM_LEXER_H
#define VULNVM_LEXER_H

#include "token.h"

typedef struct {
    Token *tokens;
    int    count;
    int    capacity;
    bool   had_error;

    /* owned decoded-literal / error-message buffers, for cleanup */
    char **owned;
    int    owned_count;
    int    owned_cap;
} TokenList;

/* Tokenize a NUL-terminated source buffer. Never returns NULL; check had_error.
 * The returned list borrows `source` (tokens point into it), so `source` must
 * outlive the TokenList. */
TokenList *lex(const char *source);

/* Free a TokenList and all buffers it owns. */
void lex_free(TokenList *tl);

/* Print the token stream to stdout (used by the `--tokens` CLI flag and tests). */
void lex_dump(const TokenList *tl);

#endif /* VULNVM_LEXER_H */
