/* parser.h — recursive-descent parser: TokenList -> Ast. */
#ifndef VULNVM_PARSER_H
#define VULNVM_PARSER_H

#include "ast.h"
#include "lexer.h"

/* Parse a token list into an Ast. Never returns NULL; check ast->had_error and
 * ast->error_msg. The Ast copies all strings it needs, so the TokenList and the
 * source buffer may be freed afterwards. */
Ast *parse(TokenList *tokens);

/* Convenience: lex then parse. Lexing errors are reported through the returned
 * Ast's had_error/error_msg as well. */
Ast *parse_source(const char *source);

#endif /* VULNVM_PARSER_H */
