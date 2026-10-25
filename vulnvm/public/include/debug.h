/* debug.h — heap-inspection tooling.
 *
 * heap_dump() prints the whole arena in physical (address) order: every block
 * with its offset, size and used/free state, and — for live objects — the type,
 * id, refcount and a short value preview. Raw buffers are annotated with the
 * object that owns them (a ByteArray's bytes, an Array's items, a function's
 * chunk, …) where that can be determined. It is read-only and allocation-free,
 * so calling it does not perturb the heap it reports. Exposed to MonkeyC as the
 * built-in dumpHeap([label]).
 */
#ifndef VULNVM_DEBUG_H
#define VULNVM_DEBUG_H

#include <stdio.h>

/* Dump the heap to `out`. `label` (may be NULL) is printed in the banner so
 * successive dumps can be told apart. */
void heap_dump(FILE *out, const char *label);

#endif /* VULNVM_DEBUG_H */
