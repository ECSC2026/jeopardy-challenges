/* alloc.h — VulnVM custom allocator.
 *
 * A single mmap'd arena managed by a segregated-free-list allocator with
 * boundary-tag coalescing. It replaces libc malloc/free entirely: every dynamic
 * allocation in the VM routes through here, so freed memory is reclaimed and
 * reused without depending on the C library heap.
 *
 * Design summary (see PLAN.md §3):
 *   - Each block carries an 8-byte header and an 8-byte footer, both holding
 *     `size | flags` (boundary tags) so neighbours can be coalesced in O(1).
 *   - Free blocks are threaded onto per-size-class doubly linked lists.
 *   - vm_alloc splits large fits; vm_free coalesces with adjacent free blocks.
 */
#ifndef VULNVM_ALLOC_H
#define VULNVM_ALLOC_H

#include "common.h"

/* Public alignment guarantee: every pointer vm_alloc returns is 16-byte aligned. */
#define VM_MALLOC_ALIGN 16

/* Initialise the arena with `size` bytes (rounded up internally). Must be called
 * once before any vm_alloc. Aborts (vm_panic) if the mmap fails. */
void arena_init(size_t size);

/* Release the arena back to the OS (munmap). After this, arena_init may be
 * called again. */
void arena_destroy(void);

/* Drop every allocation at once and restore the arena to a single free block.
 * O(1)-ish; used for fast teardown between runs/tests. */
void arena_reset(void);

/* Allocate at least `n` bytes, 16-byte aligned. Returns NULL if the arena has no
 * block large enough (the arena does not grow). n == 0 returns a valid unique
 * minimum-size allocation. */
void *vm_alloc(size_t n);

/* Resize `p` to `n` bytes, preserving contents up to min(old, n). Semantics match
 * realloc: p==NULL behaves like vm_alloc; n==0 frees and returns NULL. */
void *vm_realloc(void *p, size_t n);

/* Free a pointer previously returned by vm_alloc/vm_realloc. Freeing NULL is a
 * no-op. Double-free is detected and panics in debug builds. */
void vm_free(void *p);

/* Usable payload size of an allocation (>= the requested size). */
size_t vm_alloc_usable_size(const void *p);

/* Allocator statistics, computed by walking the heap. */
typedef struct {
    size_t arena_size;    /* total managed bytes */
    size_t bytes_in_use;  /* payload bytes currently handed out */
    size_t bytes_free;    /* payload bytes available in free blocks */
    size_t overhead;      /* header/footer/padding bytes */
    size_t used_blocks;   /* number of allocated blocks */
    size_t free_blocks;   /* number of free blocks */
    size_t largest_free;  /* payload size of the largest free block */
} AllocStats;

AllocStats alloc_stats(void);

/* Integrity checker: walks the implicit block list and every free list, verifying
 * header==footer, alignment, contiguous coverage, no adjacent free blocks left
 * uncoalesced, and free-list/marked-free consistency. Returns true if the heap is
 * well-formed. Intended for tests and debugging. If `verbose`, prints the first
 * problem found. */
bool alloc_check_heap(bool verbose);

/* Base of the usable heap (payload of the first block). Heap-dump tools show
 * block addresses relative to this. NULL before arena_init. */
void *alloc_arena_base(void);

/* True if `p` points inside the live heap region (first block .. epilogue). Used
 * by heap-inspection tools to sanity-check pointers read out of the arena. */
int alloc_in_heap(const void *p);

/* Walk every block in physical (address) order, invoking `cb` with each block's
 * payload pointer, whole-block size in bytes, and used flag (1/0). Read-only and
 * allocation-free, so it is safe to call from a heap-inspection routine. Tolerant
 * of a corrupted heap: it bounds the walk and stops on an inconsistent block
 * rather than looping or running off the arena. */
void alloc_foreach_block(void (*cb)(void *payload, size_t block_size, int used, void *ud),
                         void *ud);

#endif /* VULNVM_ALLOC_H */
