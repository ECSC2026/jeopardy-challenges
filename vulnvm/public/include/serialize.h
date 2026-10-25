/* serialize.h — compiled-program (.vmb) serialization.
 *
 * Splits the toolchain into a compiler and an interpreter: the compiler emits a
 * self-contained program (bytecode + constant pool, recursively including nested
 * function chunks) with program_write(); the interpreter reconstitutes it with
 * program_read() and runs it — no lexer/parser/compiler required.
 *
 * Constants are stored by value: immediates inline, strings/symbols by their
 * bytes/name (re-interned on load, since interned ids are process-local), and
 * nested functions recursively. Native functions are never serialized.
 */
#ifndef VULNVM_SERIALIZE_H
#define VULNVM_SERIALIZE_H

#include "object.h"
#include <stdio.h>
#include <stdint.h>

#define VMB_MAGIC0 'V'
#define VMB_MAGIC1 'V'
#define VMB_MAGIC2 'M'
#define VMB_MAGIC3 'B'
#define VMB_VERSION 1

/* Write `script` (a top-level FN_VM function, arity 0) to `out`. Returns 0 on
 * success, non-zero on error (e.g. a native function slipped into the pool). */
int program_write(FILE *out, const ObjFunction *script);

/* Read a program previously written by program_write from an in-memory buffer.
 * Returns a new owning ObjFunction reference (refcount 1) on success, or NULL on
 * malformed input, writing a message into err (if non-NULL). Requires the object
 * system to be initialised (it interns strings/symbols and allocates objects). */
ObjFunction *program_read(const uint8_t *buf, size_t len, char *err, size_t errcap);

#endif /* VULNVM_SERIALIZE_H */
