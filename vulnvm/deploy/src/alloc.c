/* alloc.c — VulnVM custom allocator implementation.
 *
 * Classic boundary-tag allocator (à la CS:APP) over a single mmap'd arena, with
 * segregated free lists for fast fits. Every block has an 8-byte header and an
 * 8-byte footer, both storing `size | flags`. Free blocks additionally store two
 * pointers (predecessor/successor) in their payload, linking them into one of
 * NUM_CLASSES size-class free lists.
 *
 * Block picture (sizes in bytes, WSIZE = 8):
 *
 *     bp-8   [ header: size | USED? ]
 *     bp     [ payload ...                    ]   <- returned to caller
 *            [ (free block: pred, succ here)  ]
 *            [ footer: size | USED?           ]   at bp + size - 16
 *
 * `size` in a header/footer is the WHOLE block size (header + payload + footer).
 */
#define _GNU_SOURCE
#include "alloc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* ------------------------------------------------------------------ panic --- */

void vm_panic(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fputs("vulnvm: fatal: ", stderr);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    abort();
}

/* ------------------------------------------------------------ constants --- */

#define WSIZE       8u                 /* word / header / footer size          */
#define DSIZE       16u                /* double word; header+footer overhead  */
#define ALIGN       VM_MALLOC_ALIGN    /* payload alignment (16)               */
#define MIN_BLOCK   32u                /* header + 2 ptrs + footer             */
#define NUM_CLASSES 32                 /* size-class free lists                */

#define FLAG_USED   0x1u               /* low bit of header/footer word        */
#define SIZE_MASK   (~(size_t)0xFu)    /* size occupies bits above the flags   */

/* Poison bytes written into freed payloads in debug builds (helps catch UAF in
 * the baseline). Kept cheap; can be disabled by defining VM_NO_POISON. */
#ifndef VM_NO_POISON
#  define POISON_FREE 0xAB
#  define POISON_CAP  64u   /* cap poison per free so it stays O(1), not O(size) */
#endif

/* ------------------------------------------------------------ arena state -- */

#ifdef __SANITIZE_ADDRESS__
/* Under AddressSanitizer, mmap(NULL) occasionally places our arena at an address
 * whose ASan shadow falls in the sanitizer's PROT_NONE shadow gap, causing a
 * spurious SIGSEGV in ASan's own load/store instrumentation on the very first
 * arena access. To keep the sanitizer gate reliable, back the arena with a
 * static buffer that lives in ordinary, fully-shadow-mapped application memory.
 * (The mmap path is still exercised by the non-sanitized build.) */
#  define ASAN_ARENA_CAP (32u * 1024 * 1024)
static uint8_t g_asan_arena[ASAN_ARENA_CAP] __attribute__((aligned(4096)));
#endif

static uint8_t *g_arena      = NULL;   /* base of the mmap region              */
static size_t   g_arena_size = 0;      /* bytes mmap'd                          */
static uint8_t *g_heap_lo    = NULL;   /* first block payload                   */
static uint8_t *g_epilogue   = NULL;   /* epilogue header (size 0, USED)        */
static void    *g_free[NUM_CLASSES];   /* segregated free-list heads (payloads) */

/* ------------------------------------------------------- word accessors ---- */

static inline size_t word_get(const void *p)          { return *(const size_t *)p; }
static inline void   word_put(void *p, size_t v)      { *(size_t *)p = v; }
static inline size_t pack(size_t size, size_t used)   { return (size & SIZE_MASK) | used; }

static inline size_t blk_size(const void *hdr)  { return word_get(hdr) & SIZE_MASK; }
static inline size_t blk_used(const void *hdr)  { return word_get(hdr) & FLAG_USED; }

/* bp is the payload pointer. */
static inline void  *hdrp(void *bp)  { return (uint8_t *)bp - WSIZE; }
static inline void  *ftrp(void *bp)  { return (uint8_t *)bp + blk_size(hdrp(bp)) - DSIZE; }
static inline void  *next_blk(void *bp) { return (uint8_t *)bp + blk_size((uint8_t *)bp - WSIZE); }
static inline void  *prev_blk(void *bp) { return (uint8_t *)bp - blk_size((uint8_t *)bp - DSIZE); }

/* Free-list link accessors (stored in the first two words of a free payload). */
static inline void  *fl_pred(void *bp)            { return *(void **)bp; }
static inline void  *fl_succ(void *bp)            { return *((void **)bp + 1); }
static inline void   fl_set_pred(void *bp, void *v) { *(void **)bp = v; }
static inline void   fl_set_succ(void *bp, void *v) { *((void **)bp + 1) = v; }

/* ------------------------------------------------------ size classes ------- */

/* Map a whole-block size to a free-list index. Class i holds sizes in
 * [MIN_BLOCK<<i, MIN_BLOCK<<(i+1)); the last class is a catch-all. */
static int size_class(size_t bsize) {
    int i = 0;
    size_t cap = MIN_BLOCK;
    while (i < NUM_CLASSES - 1 && bsize >= cap * 2) {
        cap *= 2;
        i++;
    }
    return i;
}

static void fl_insert(void *bp) {
    int c = size_class(blk_size(hdrp(bp)));
    void *head = g_free[c];
    fl_set_pred(bp, NULL);
    fl_set_succ(bp, head);
    if (head) fl_set_pred(head, bp);
    g_free[c] = bp;
}

static void fl_remove(void *bp) {
    int c = size_class(blk_size(hdrp(bp)));
    void *pred = fl_pred(bp);
    void *succ = fl_succ(bp);
    if (pred) fl_set_succ(pred, succ);
    else      g_free[c] = succ;
    if (succ) fl_set_pred(succ, pred);
}

/* ------------------------------------------------------ core helpers ------- */

static void mark_free_block(void *bp, size_t bsize) {
    word_put(hdrp(bp), pack(bsize, 0));
    word_put(ftrp(bp), pack(bsize, 0));
#ifdef POISON_FREE
    /* Poison a bounded prefix of the payload beyond the two link words. Bounding
     * it keeps free O(1): a full-block memset would be O(block size), which is
     * catastrophic when a small allocation is carved from the multi-megabyte
     * arena-tail free block (the remainder is re-marked free on every split). A
     * short poison still scrambles the start of freed data to catch use-after-free
     * in tests. */
    if (bsize > MIN_BLOCK) {
        uint8_t *start = (uint8_t *)bp + 2 * sizeof(void *);
        uint8_t *end   = (uint8_t *)ftrp(bp);
        size_t n = (end > start) ? (size_t)(end - start) : 0;
        if (n > POISON_CAP) n = POISON_CAP;
        if (n) memset(start, POISON_FREE, n);
    }
#endif
}

/* Coalesce bp with free neighbours; returns the (possibly enlarged) block. The
 * caller must have already marked bp free but NOT inserted it into a free list.
 * Neighbours that are merged are removed from their free lists here. */
static void *coalesce(void *bp) {
    size_t size = blk_size(hdrp(bp));
    size_t prev_used = blk_used(ftrp(prev_blk(bp)));
    size_t next_used = blk_used(hdrp(next_blk(bp)));

    if (prev_used && next_used) {
        return bp;
    }
    if (prev_used && !next_used) {                 /* merge with next */
        void *nb = next_blk(bp);
        fl_remove(nb);
        size += blk_size(hdrp(nb));
        word_put(hdrp(bp), pack(size, 0));
        word_put(ftrp(bp), pack(size, 0));         /* ftrp recomputed via new size */
        return bp;
    }
    if (!prev_used && next_used) {                 /* merge with prev */
        void *pb = prev_blk(bp);
        fl_remove(pb);
        size += blk_size(hdrp(pb));
        word_put(hdrp(pb), pack(size, 0));
        word_put(ftrp(pb), pack(size, 0));
        return pb;
    }
    /* merge with both */
    void *pb = prev_blk(bp);
    void *nb = next_blk(bp);
    fl_remove(pb);
    fl_remove(nb);
    size += blk_size(hdrp(pb)) + blk_size(hdrp(nb));
    word_put(hdrp(pb), pack(size, 0));
    word_put(ftrp(pb), pack(size, 0));
    return pb;
}

/* Place an allocation of asize into free block bp (already removed from its list),
 * splitting off the remainder as a new free block if it is big enough. */
static void place(void *bp, size_t asize) {
    size_t bsize = blk_size(hdrp(bp));
    if (bsize - asize >= MIN_BLOCK) {
        word_put(hdrp(bp), pack(asize, FLAG_USED));
        word_put(ftrp(bp), pack(asize, FLAG_USED));
        void *rem = next_blk(bp);
        mark_free_block(rem, bsize - asize);
        fl_insert(rem);
    } else {
        word_put(hdrp(bp), pack(bsize, FLAG_USED));
        word_put(ftrp(bp), pack(bsize, FLAG_USED));
    }
}

static void *find_fit(size_t asize) {
    for (int c = size_class(asize); c < NUM_CLASSES; c++) {
        for (void *bp = g_free[c]; bp; bp = fl_succ(bp)) {
            if (blk_size(hdrp(bp)) >= asize) return bp;
        }
    }
    return NULL;
}

/* Round a request into a whole-block size satisfying alignment and the minimum.
 * Block = header(8) + payload(aligned to 16) + footer(8); the 16-byte-aligned
 * payload keeps the returned pointer aligned given the 8-byte header offset.
 * Returns 0 if `n` is so large that the rounding would overflow — callers treat
 * 0 as "no such allocation is possible" and return NULL. */
static size_t adjust_size(size_t n) {
    if (n > SIZE_MAX - DSIZE - ALIGN) return 0;   /* would overflow below */
    size_t asize = VM_ALIGN_UP(n, ALIGN) + DSIZE;
    if (asize < MIN_BLOCK) asize = MIN_BLOCK;
    return asize;
}

/* -------------------------------------------------------- public API ------- */

void arena_init(size_t size) {
    if (g_arena) vm_panic("arena_init called twice");

    size = VM_ALIGN_UP(size, 4096);
    if (size < 1u << 16) size = 1u << 16;

#ifdef __SANITIZE_ADDRESS__
    if (size > ASAN_ARENA_CAP) size = ASAN_ARENA_CAP;
    void *mem = g_asan_arena;
#else
    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) vm_panic("mmap(%zu) failed", size);
#endif

    g_arena      = (uint8_t *)mem;
    g_arena_size = size;
    memset(g_free, 0, sizeof(g_free));

    /* Prologue: 8-byte alignment pad, then a 16-byte used prologue block, so the
     * first real payload lands at a 16-byte boundary. Epilogue is a 0-size used
     * header sentinel at the very end. */
    uint8_t *p = g_arena;
    word_put(p, 0);                                    /* +0  alignment pad     */
    word_put(p + WSIZE, pack(DSIZE, FLAG_USED));       /* +8  prologue header   */
    word_put(p + 2 * WSIZE, pack(DSIZE, FLAG_USED));   /* +16 prologue footer   */
    g_epilogue = g_arena + size - WSIZE;
    word_put(g_epilogue, pack(0, FLAG_USED));          /* epilogue header       */

    /* One big free block spanning the middle. Its payload starts at +32. */
    uint8_t *bp = g_arena + 3 * WSIZE + WSIZE;         /* payload at +32        */
    g_heap_lo = bp;
    size_t big = (size_t)(g_epilogue - (uint8_t *)hdrp(bp)); /* header .. epilogue */
    mark_free_block(bp, big);
    fl_insert(bp);
}

void arena_destroy(void) {
    if (!g_arena) return;
#ifndef __SANITIZE_ADDRESS__
    munmap(g_arena, g_arena_size);
#endif
    g_arena = NULL;
    g_arena_size = 0;
    g_heap_lo = g_epilogue = NULL;
    memset(g_free, 0, sizeof(g_free));
}

void arena_reset(void) {
    if (!g_arena) vm_panic("arena_reset before arena_init");
    size_t size = g_arena_size;
    memset(g_free, 0, sizeof(g_free));
    uint8_t *bp = g_heap_lo;
    size_t big = (size_t)(g_epilogue - (uint8_t *)hdrp(bp));
    mark_free_block(bp, big);
    fl_insert(bp);
    (void)size;
}

void *vm_alloc(size_t n) {
    if (!g_arena) vm_panic("vm_alloc before arena_init");
    size_t asize = adjust_size(n);
    if (asize == 0) return NULL;         /* request too large to represent */
    void *bp = find_fit(asize);
    if (!bp) return NULL;             /* arena exhausted */
    fl_remove(bp);
    place(bp, asize);
    return bp;
}

void vm_free(void *p) {
    if (!p) return;
    if (!g_arena) vm_panic("vm_free before arena_init");
    if ((uint8_t *)p < g_heap_lo || (uint8_t *)p >= g_epilogue)
        vm_panic("vm_free(%p): pointer outside arena", p);
    if (!blk_used(hdrp(p)))
        vm_panic("vm_free(%p): double free or corruption", p);

    size_t size = blk_size(hdrp(p));
    mark_free_block(p, size);
    void *bp = coalesce(p);
    fl_insert(bp);
}

void *vm_realloc(void *p, size_t n) {
    if (!p) return vm_alloc(n);
    if (n == 0) { vm_free(p); return NULL; }

    size_t asize   = adjust_size(n);
    if (asize == 0) return NULL;         /* request too large to represent */
    size_t oldsize = blk_size(hdrp(p));

    if (oldsize >= asize) {
        /* Shrink in place; split off a free remainder if worthwhile. */
        if (oldsize - asize >= MIN_BLOCK) {
            word_put(hdrp(p), pack(asize, FLAG_USED));
            word_put(ftrp(p), pack(asize, FLAG_USED));
            void *rem = next_blk(p);
            mark_free_block(rem, oldsize - asize);
            void *cb = coalesce(rem);
            fl_insert(cb);
        }
        return p;
    }

    /* Try to grow into an adjacent free next block. */
    void *nb = next_blk(p);
    if (!blk_used(hdrp(nb))) {
        size_t combined = oldsize + blk_size(hdrp(nb));
        if (combined >= asize) {
            fl_remove(nb);
            if (combined - asize >= MIN_BLOCK) {
                word_put(hdrp(p), pack(asize, FLAG_USED));
                word_put(ftrp(p), pack(asize, FLAG_USED));
                void *rem = next_blk(p);
                mark_free_block(rem, combined - asize);
                fl_insert(rem);
            } else {
                word_put(hdrp(p), pack(combined, FLAG_USED));
                word_put(ftrp(p), pack(combined, FLAG_USED));
            }
            return p;
        }
    }

    /* Fall back to allocate-copy-free. */
    void *np = vm_alloc(n);
    if (!np) return NULL;
    size_t copy = oldsize - DSIZE;          /* old payload capacity */
    if (copy > n) copy = n;
    memcpy(np, p, copy);
    vm_free(p);
    return np;
}

size_t vm_alloc_usable_size(const void *p) {
    if (!p) return 0;
    return blk_size((const uint8_t *)p - WSIZE) - DSIZE;
}

/* --------------------------------------------------------- stats/check ----- */

AllocStats alloc_stats(void) {
    AllocStats s;
    memset(&s, 0, sizeof(s));
    s.arena_size = g_arena_size;
    if (!g_arena) return s;

    for (uint8_t *bp = g_heap_lo; (uint8_t *)hdrp(bp) < g_epilogue;
         bp = next_blk(bp)) {
        size_t bsize = blk_size(hdrp(bp));
        if (blk_used(hdrp(bp))) {
            s.used_blocks++;
            s.bytes_in_use += bsize - DSIZE;
        } else {
            s.free_blocks++;
            s.bytes_free += bsize - DSIZE;
            if (bsize - DSIZE > s.largest_free) s.largest_free = bsize - DSIZE;
        }
        s.overhead += DSIZE;
    }
    return s;
}

void *alloc_arena_base(void) { return g_heap_lo; }

int alloc_in_heap(const void *p) {
    return g_arena && (const uint8_t *)p >= g_heap_lo
                   && (const uint8_t *)p < (const uint8_t *)g_epilogue;
}

void alloc_foreach_block(void (*cb)(void *payload, size_t block_size, int used, void *ud),
                         void *ud) {
    if (!g_arena) return;
    /* Bound the walk so a corrupted heap (the very thing you'd want to dump)
     * can't loop forever or read past the arena. */
    size_t max_blocks = g_arena_size / MIN_BLOCK + 16;
    uint8_t *bp = g_heap_lo;
    for (size_t n = 0; n < max_blocks && (uint8_t *)hdrp(bp) < g_epilogue; n++) {
        size_t bsize = blk_size(hdrp(bp));
        cb(bp, bsize, blk_used(hdrp(bp)) ? 1 : 0, ud);
        if (bsize < MIN_BLOCK) break;                 /* corrupt size: bail out */
        uint8_t *nb = bp + bsize;                     /* next payload */
        if (nb <= bp || (uint8_t *)hdrp(nb) > g_epilogue) break;
        bp = nb;
    }
}

bool alloc_check_heap(bool verbose) {
#define FAIL(...) do { if (verbose) { fprintf(stderr, "heap check: "); \
                       fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } \
                       return false; } while (0)
    if (!g_arena) FAIL("arena not initialised");

    /* Walk the implicit list: coverage, alignment, header==footer, coalescing. */
    size_t counted_free = 0;
    int prev_free = 0;
    uint8_t *bp = g_heap_lo;
    for (; (uint8_t *)hdrp(bp) < g_epilogue; bp = next_blk(bp)) {
        if (((uintptr_t)bp % ALIGN) != 0)
            FAIL("block %p not %u-aligned", (void *)bp, ALIGN);
        size_t h = word_get(hdrp(bp));
        size_t f = word_get(ftrp(bp));
        if (h != f)
            FAIL("block %p header/footer mismatch (%zx vs %zx)", (void *)bp, h, f);
        size_t bsize = blk_size(hdrp(bp));
        if (bsize < MIN_BLOCK)
            FAIL("block %p size %zu below minimum", (void *)bp, bsize);
        if ((uint8_t *)hdrp(next_blk(bp)) > g_epilogue)
            FAIL("block %p overruns epilogue", (void *)bp);
        int is_free = !blk_used(hdrp(bp));
        if (is_free) {
            counted_free++;
            if (prev_free)
                FAIL("adjacent free blocks not coalesced near %p", (void *)bp);
        }
        prev_free = is_free;
    }
    if ((uint8_t *)hdrp(bp) != g_epilogue)
        FAIL("block walk did not land on epilogue (%p vs %p)",
             (void *)hdrp(bp), (void *)g_epilogue);
    if (word_get(g_epilogue) != pack(0, FLAG_USED))
        FAIL("epilogue corrupted");

    /* Walk every free list: entries must be marked free, in-range, size-class
     * correct, and links consistent. Count them against the implicit walk. */
    size_t listed_free = 0;
    for (int c = 0; c < NUM_CLASSES; c++) {
        for (void *fb = g_free[c]; fb; fb = fl_succ(fb)) {
            if ((uint8_t *)fb < g_heap_lo || (uint8_t *)fb >= g_epilogue)
                FAIL("free-list %d entry %p out of range", c, fb);
            if (blk_used(hdrp(fb)))
                FAIL("free-list %d entry %p marked used", c, fb);
            if (size_class(blk_size(hdrp(fb))) != c)
                FAIL("free block %p in wrong class %d", fb, c);
            void *succ = fl_succ(fb);
            if (succ && fl_pred(succ) != fb)
                FAIL("free-list %d broken link at %p", c, fb);
            listed_free++;
            if (listed_free > counted_free + 1)
                FAIL("free-list cycle detected in class %d", c);
        }
    }
    if (listed_free != counted_free)
        FAIL("free count mismatch: implicit=%zu listed=%zu",
             counted_free, listed_free);
    return true;
#undef FAIL
}
