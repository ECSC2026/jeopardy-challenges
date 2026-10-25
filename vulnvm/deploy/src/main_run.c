/* main_run.c — the `vulnvm` interpreter driver.
 *
 * Loads a self-contained .vmb program (bytecode + constants produced by
 * `vulnvmc`) and executes it. The lexer/parser/compiler are NOT linked in: the
 * interpreter only deserializes and runs bytecode.
 *
 * Usage: vulnvm [flags] program.vmb
 *   --trace      trace execution (stack + each instruction)
 *   --bytecode   disassemble the loaded program and exit
 *   --arena N    allocator arena size in MiB (default 64)
 */
#include "alloc.h"
#include "serialize.h"
#include "chunk.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *read_file_bin(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "vulnvm: cannot open '%s'\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return NULL; }
    uint8_t *buf = (uint8_t *)vm_alloc((size_t)size);
    size_t got = fread(buf, 1, (size_t)size, f);
    fclose(f);
    *out_len = got;
    return buf;
}

static void disasm_program(ObjFunction *fn) {
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
}

int main(int argc, char **argv) {
    /* Unbuffer stdout so output reaches the terminal/socket immediately. When
     * stdout is not a tty (e.g. the CTF wrapper points it at a socket) glibc
     * block-buffers it, and a crash — the whole point of this VM — would drop the
     * un-flushed tail (the last println before the fault). This must live here in
     * the VM, not the wrapper: stdio buffering is reset on exec. */
    setvbuf(stdout, NULL, _IONBF, 0);

    const char *path = NULL;
    bool opt_trace = false, opt_bytecode = false;
    size_t arena_mib = 64;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--trace") == 0) opt_trace = true;
        else if (strcmp(argv[i], "--bytecode") == 0) opt_bytecode = true;
        else if (strcmp(argv[i], "--arena") == 0 && i + 1 < argc) arena_mib = (size_t)strtoul(argv[++i], NULL, 10);
        else if (argv[i][0] == '-') { fprintf(stderr, "vulnvm: unknown flag '%s'\n", argv[i]); return 2; }
        else path = argv[i];
    }
    if (!path) { fprintf(stderr, "usage: vulnvm [--trace|--bytecode] program.vmb\n"); return 2; }

    arena_init(arena_mib * 1024 * 1024);
    object_system_init();

    size_t len = 0;
    uint8_t *blob = read_file_bin(path, &len);
    if (!blob) { object_system_shutdown(); arena_destroy(); return 1; }

    int rc = 0;
    VM *vm = vm_new();
    vm_set_trace(vm, opt_trace);

    char err[128];
    ObjFunction *script = program_read(blob, len, err, sizeof err);
    if (!script) {
        fprintf(stderr, "vulnvm: %s\n", err);
        rc = 1;
    } else if (opt_bytecode) {
        disasm_program(script);
        release(mk_obj((Obj *)script));
    } else {
        VMResult r = vm_interpret(vm, script);
        release(mk_obj((Obj *)script));
        if (r != VM_OK) { fprintf(stderr, "runtime error: %s\n", vm_last_error(vm)); rc = 1; }
    }

    vm_free_vm(vm);
    vm_free(blob);
    object_system_shutdown();
    arena_destroy();
    return rc;
}
