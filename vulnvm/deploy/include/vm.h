/* vm.h — the VulnVM bytecode execution engine.
 *
 * A stack machine: values live on a contiguous value stack; each active call has
 * a CallFrame with its instruction pointer and a `slots` base into that stack.
 * The stack owns a reference to every heap object it holds (move semantics: push
 * transfers a ref in, pop transfers it out), so refcounts stay balanced.
 */
#ifndef VULNVM_VM_H
#define VULNVM_VM_H

#include "object.h"
#include "compiler.h"

typedef struct VM VM;

/* Native (C-backed) function signature. `recv` is the receiver for method calls
 * (null for plain function calls). The returned Value is an owned reference. */
typedef Value (*NativeFn)(VM *vm, Value recv, int argc, Value *args);

typedef enum { VM_OK, VM_COMPILE_ERROR, VM_RUNTIME_ERROR } VMResult;

/* Create/destroy a VM. Requires the object system to be initialised. */
VM  *vm_new(void);
void vm_free_vm(VM *vm);

/* Run a compiled script function (arity 0). Consumes nothing; the caller still
 * owns `script`. */
VMResult vm_interpret(VM *vm, ObjFunction *script);

/* Convenience: lex + parse + compile + run a source string. */
VMResult vm_run_source(VM *vm, const char *source);

/* Read a global by name after a run (borrowed value; retain if you keep it). */
bool vm_get_global(VM *vm, const char *name, Value *out);

/* Register a native global function. */
void vm_define_native(VM *vm, const char *name, uint8_t arity, NativeFn fn);

/* Invoke a method by symbol on any receiver, synchronously — dispatching user
 * instance/class/module methods (via a nested run) as well as native built-in
 * methods. `args` is a plain array (not the VM stack). On success stores an owned
 * value in *result and returns true; on failure calls vm_raise and returns false.
 * Used by Method.invoke and callback-style natives. */
bool vm_invoke_value(VM *vm, Value recv, uint32_t sym, int argc, Value *args, Value *result);

/* Raise a runtime error from a native method: formats the message (appending the
 * current source line) and marks the VM errored, but does NOT unwind the stack —
 * the invoking opcode handler resets it. Always returns false, for convenient
 * `return vm_raise(vm, ...)`. */
bool vm_raise(VM *vm, const char *fmt, ...);

/* Last error message (valid after VM_RUNTIME_ERROR / VM_COMPILE_ERROR). */
const char *vm_last_error(const VM *vm);

/* Enable/disable execution tracing (dumps stack + each instruction). */
void vm_set_trace(VM *vm, bool on);

#endif /* VULNVM_VM_H */
