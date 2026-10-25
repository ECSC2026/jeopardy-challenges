/* builtins.h — native (C-backed) methods for the Toybox.Lang types and the
 * method-dispatch entry point used by OP_INVOKE. */
#ifndef VULNVM_BUILTINS_H
#define VULNVM_BUILTINS_H

#include "vm.h"

/* Dispatch a method call `recv.name(args...)`. On success sets *result to an
 * owned Value and returns true. On failure (no such method / bad args / runtime
 * error) it calls vm_raise and returns false. */
bool vm_invoke(VM *vm, Value recv, uint32_t sym, int argc, Value *args, Value *result);

/* Whether the receiver type has a method with the given symbol (for the `has`
 * operator on built-in types). */
bool vm_has_method(Value recv, uint32_t sym);

/* Render any value as its toString()/display String (recursive for collections,
 * cycle-safe via a depth cap). Returns an owned reference. */
ObjString *value_to_string(Value v);

#endif /* VULNVM_BUILTINS_H */
