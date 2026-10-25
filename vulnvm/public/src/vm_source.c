/* vm_source.c — the source front-end for the VM.
 *
 * vm_run_source is the only VM entry point that needs the lexer/parser/compiler.
 * It lives here, apart from vm.c, so the interpreter binary can link the VM core
 * (which runs precompiled .vmb programs) without pulling in the compiler. The
 * library and test builds link this unit too, so vm_run_source keeps working for
 * embedders and tests.
 */
#include "vm.h"
#include "vm_internal.h"
#include "parser.h"
#include "compiler.h"
#include "value.h"

VMResult vm_run_source(VM *vm, const char *source) {
    Ast *ast = parse_source(source);
    if (ast->had_error) {
        vm_set_compile_error(vm, ast->error_msg);
        ast_free(ast);
        return VM_COMPILE_ERROR;
    }
    CompileResult cres;
    ObjFunction *script = compile(ast, &cres);
    ast_free(ast);
    if (!script) {
        vm_set_compile_error(vm, cres.error_msg);
        return VM_COMPILE_ERROR;
    }
    VMResult r = vm_interpret(vm, script);
    release(mk_obj((Obj *)script));
    return r;
}
