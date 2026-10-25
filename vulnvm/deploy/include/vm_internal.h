/* vm_internal.h — bridge between the VM core (vm.c) and the source-front-end
 * (vm_source.c). vm_run_source is the only VM entry point that pulls in the
 * lexer/parser/compiler; it lives in its own translation unit so the interpreter
 * binary can link the VM without the compiler. This header lets that unit report
 * a compile error through the VM's error channel without exposing the VM struct.
 */
#ifndef VULNVM_VM_INTERNAL_H
#define VULNVM_VM_INTERNAL_H

#include "vm.h"

/* Record a compile-time error message (retrievable via vm_last_error) and mark
 * the VM as errored. */
void vm_set_compile_error(VM *vm, const char *msg);

#endif /* VULNVM_VM_INTERNAL_H */
