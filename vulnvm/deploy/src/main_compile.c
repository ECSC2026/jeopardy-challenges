/* main_compile.c — the `vulnvmc` compiler driver.
 *
 * Lexes, parses and compiles a MonkeyC source file into a self-contained .vmb
 * program (bytecode + constant pool) that the `vulnvm` interpreter runs. No VM
 * is linked in — this binary only produces bytecode.
 *
 * Usage: vulnvmc [flags] program.mc
 *   -o FILE      write the program to FILE (default: program.mc -> program.vmb;
 *                "-" writes to stdout)
 *   --tokens     dump the token stream and exit
 *   --ast        dump the AST and exit
 *   --bytecode   disassemble the compiled program and exit (no .vmb written)
 *   --arena N    allocator arena size in MiB (default 64)
 */
#include "alloc.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "serialize.h"
#include "chunk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "vulnvmc: cannot open '%s'\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return NULL; }
    char *buf = (char *)vm_alloc((size_t)size + 1);
    size_t got = fread(buf, 1, (size_t)size, f);
    buf[got] = '\0';
    fclose(f);
    return buf;
}

/* Derive "foo.mc" -> "foo.vmb" (or append ".vmb" if there is no ".mc"). */
static char *default_output(const char *src) {
    size_t n = strlen(src);
    const char *dot = (n >= 3 && strcmp(src + n - 3, ".mc") == 0) ? src + n - 3 : src + n;
    size_t base = (size_t)(dot - src);
    char *out = (char *)malloc(base + 5);
    memcpy(out, src, base);
    memcpy(out + base, ".vmb", 5);
    return out;
}

int main(int argc, char **argv) {
    const char *path = NULL, *out_path = NULL;
    bool opt_tokens = false, opt_ast = false, opt_bytecode = false;
    size_t arena_mib = 64;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) out_path = argv[++i];
        else if (strcmp(argv[i], "--tokens") == 0) opt_tokens = true;
        else if (strcmp(argv[i], "--ast") == 0) opt_ast = true;
        else if (strcmp(argv[i], "--bytecode") == 0) opt_bytecode = true;
        else if (strcmp(argv[i], "--arena") == 0 && i + 1 < argc) arena_mib = (size_t)strtoul(argv[++i], NULL, 10);
        else if (argv[i][0] == '-' && argv[i][1] != '\0' && strcmp(argv[i], "-") != 0) {
            fprintf(stderr, "vulnvmc: unknown flag '%s'\n", argv[i]); return 2;
        } else path = argv[i];
    }
    if (!path) { fprintf(stderr, "usage: vulnvmc [-o out.vmb] [--tokens|--ast|--bytecode] program.mc\n"); return 2; }

    arena_init(arena_mib * 1024 * 1024);
    object_system_init();

    char *source = read_file(path);
    if (!source) { object_system_shutdown(); arena_destroy(); return 1; }

    int rc = 0;
    if (opt_tokens) {
        TokenList *tl = lex(source);
        lex_dump(tl);
        lex_free(tl);
        goto done;
    }

    Ast *ast = parse_source(source);
    if (ast->had_error) { fprintf(stderr, "parse error: %s\n", ast->error_msg); ast_free(ast); rc = 1; goto done; }
    if (opt_ast) { ast_dump(ast->root, stdout); ast_free(ast); goto done; }

    CompileResult cres;
    ObjFunction *fn = compile(ast, &cres);
    ast_free(ast);
    if (!fn) { fprintf(stderr, "compile error: %s\n", cres.error_msg); rc = 1; goto done; }

    if (opt_bytecode) {
        chunk_disassemble(fn->chunk, "<script>", stdout);
        for (int i = 0; i < fn->chunk->constants.count; i++) {
            Value v = fn->chunk->constants.values[i];
            if (IS_FUNCTION(v)) {
                ObjFunction *sub = AS_FUNCTION(v);
                char nm[128];
                snprintf(nm, sizeof(nm), "fn %s", sub->name ? sub->name->data : "?");
                chunk_disassemble(sub->chunk, nm, stdout);
            }
        }
        release(mk_obj((Obj *)fn));
        goto done;
    }

    /* Serialize to the output file. */
    char *derived = NULL;
    const char *target = out_path ? out_path : (derived = default_output(path));
    bool to_stdout = strcmp(target, "-") == 0;
    FILE *out = to_stdout ? stdout : fopen(target, "wb");
    if (!out) { fprintf(stderr, "vulnvmc: cannot write '%s'\n", target); rc = 1; }
    else {
        if (program_write(out, fn) != 0) { fprintf(stderr, "vulnvmc: serialization failed\n"); rc = 1; }
        if (!to_stdout) fclose(out);
        else fflush(out);
        if (rc == 0 && !to_stdout) fprintf(stderr, "vulnvmc: wrote %s\n", target);
    }
    free(derived);
    release(mk_obj((Obj *)fn));

done:
    vm_free(source);
    object_system_shutdown();
    arena_destroy();
    return rc;
}
