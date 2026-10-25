/* common.h — shared fixed-width types, macros and error codes for VulnVM. */
#ifndef VULNVM_COMMON_H
#define VULNVM_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Compiler hints. */
#if defined(__GNUC__) || defined(__clang__)
#  define VM_LIKELY(x)   __builtin_expect(!!(x), 1)
#  define VM_UNLIKELY(x) __builtin_expect(!!(x), 0)
#  define VM_NORETURN    __attribute__((noreturn))
#  define VM_UNUSED      __attribute__((unused))
#else
#  define VM_LIKELY(x)   (x)
#  define VM_UNLIKELY(x) (x)
#  define VM_NORETURN
#  define VM_UNUSED
#endif

/* Alignment helper: round `n` up to the next multiple of `a` (a power of two). */
#define VM_ALIGN_UP(n, a) (((size_t)(n) + ((size_t)(a) - 1)) & ~((size_t)(a) - 1))

/* Fatal error used only for unrecoverable internal invariants (not user errors).
 * Writes a message and aborts. Declared here, defined in alloc.c for now. */
VM_NORETURN void vm_panic(const char *fmt, ...);

#endif /* VULNVM_COMMON_H */
