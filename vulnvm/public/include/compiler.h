/* compiler.h — AST -> bytecode compiler.
 *
 * Produces the top-level script as an ObjFunction (arity 0). Requires the object
 * system to be initialised (it creates string/function objects and symbol ids).
 * Top-level var/const/enum/function declarations become globals; variables inside
 * function bodies become locals (stack slots).
 */
#ifndef VULNVM_COMPILER_H
#define VULNVM_COMPILER_H

#include "ast.h"
#include "object.h"

typedef struct {
    bool had_error;
    char error_msg[256];
    int  error_line;
} CompileResult;

/* Compile a parsed program. On success returns the script ObjFunction (refcount
 * 1, owned by the caller) and sets result->had_error = false. On failure returns
 * NULL with result describing the first error. */
ObjFunction *compile(Ast *ast, CompileResult *result);

#endif /* VULNVM_COMPILER_H */
