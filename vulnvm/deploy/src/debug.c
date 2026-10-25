/* debug.c — heap_dump(): an object-aware view of the whole arena. See debug.h.
 *
 * Allocation-free by construction (fixed stack buffers, direct fprintf), so it
 * never disturbs the heap it is inspecting — important when debugging grooming
 * and overflows. Correlating a block with an object is exact: a used block is a
 * live object iff objmgr_get(header.id) points back at that block's payload.
 */
#include "debug.h"
#include "alloc.h"
#include "object.h"
#include "value.h"
#include "chunk.h"

#include <string.h>

/* ---- small formatting helpers (no allocation) ----------------------------- */

static void print_escaped(FILE *out, const char *data, uint32_t len, uint32_t cap) {
    uint32_t n = len < cap ? len : cap;
    for (uint32_t i = 0; i < n; i++) {
        unsigned char ch = (unsigned char)data[i];
        if (ch == '"' || ch == '\\') { fputc('\\', out); fputc(ch, out); }
        else if (ch >= 0x20 && ch < 0x7F) fputc(ch, out);
        else fprintf(out, "\\x%02X", ch);
    }
    if (len > cap) fprintf(out, "...");
}

static void print_hex(FILE *out, const uint8_t *p, uint32_t len, uint32_t cap) {
    uint32_t n = len < cap ? len : cap;
    for (uint32_t i = 0; i < n; i++) fprintf(out, "%s%02X", i ? " " : "", p[i]);
    if (len > cap) fprintf(out, " ...");
}

/* ---- reverse lookup: which object owns this raw buffer? ------------------- */

typedef struct { const void *target; char desc[80]; int found; } BufSearch;

static void buf_probe(Obj *o, void *ud) {
    BufSearch *s = (BufSearch *)ud;
    if (s->found) return;
    const void *t = s->target;
    switch (obj_type(o)) {
    case OBJ_ARRAY:
        if ((const void *)((ObjArray *)o)->items == t)
            { snprintf(s->desc, sizeof s->desc, "items of Array#%u", obj_id(o)); s->found = 1; }
        break;
    case OBJ_BYTEARRAY:
        if ((const void *)((ObjByteArray *)o)->bytes == t)
            { snprintf(s->desc, sizeof s->desc, "bytes of ByteArray#%u", obj_id(o)); s->found = 1; }
        break;
    case OBJ_DICT:
        if ((const void *)((ObjDict *)o)->entries == t)
            { snprintf(s->desc, sizeof s->desc, "entries of Dict#%u", obj_id(o)); s->found = 1; }
        break;
    case OBJ_FUNCTION: {
        ObjFunction *f = (ObjFunction *)o;
        if (f->kind != FN_VM || !f->chunk) break;
        if ((const void *)f->chunk == t)
            { snprintf(s->desc, sizeof s->desc, "chunk of Fn#%u", obj_id(o)); s->found = 1; }
        else if ((const void *)f->chunk->code == t)
            { snprintf(s->desc, sizeof s->desc, "code of Fn#%u", obj_id(o)); s->found = 1; }
        else if ((const void *)f->chunk->lines == t)
            { snprintf(s->desc, sizeof s->desc, "lines of Fn#%u", obj_id(o)); s->found = 1; }
        else if ((const void *)f->chunk->constants.values == t)
            { snprintf(s->desc, sizeof s->desc, "constants of Fn#%u", obj_id(o)); s->found = 1; }
        break;
    }
    default: break;
    }
}

/* ---- per-object annotation ------------------------------------------------ */

static void describe_object(FILE *out, Obj *o) {
    ObjType t = obj_type(o);
    fprintf(out, "%s#%u rc=%u", obj_type_name(t), obj_id(o), o->refcount);
    switch (t) {
    case OBJ_STRING: {
        ObjString *s = (ObjString *)o;
        fprintf(out, " len=%u \"", s->length);
        print_escaped(out, s->data, s->length, 32);
        fputc('"', out);
        break;
    }
    case OBJ_ARRAY: {
        ObjArray *a = (ObjArray *)o;
        fprintf(out, " count=%u cap=%u", a->count, a->capacity);
        break;
    }
    case OBJ_BYTEARRAY: {
        ObjByteArray *b = (ObjByteArray *)o;
        fprintf(out, " count=%u cap=%u", b->count, b->capacity);
        if (b->count <= b->capacity && b->bytes) { fprintf(out, " ["); print_hex(out, b->bytes, b->count, 8); fputc(']', out); }
        else fprintf(out, "  <<< count>cap: CORRUPT?");
        break;
    }
    case OBJ_DICT: {
        ObjDict *d = (ObjDict *)o;
        fprintf(out, " count=%u cap=%u", d->count, d->capacity);
        break;
    }
    case OBJ_FUNCTION: {
        ObjFunction *f = (ObjFunction *)o;
        fprintf(out, " %s name=%s", f->kind == FN_NATIVE ? "native" : "vm",
                f->name ? f->name->data : "<script>");
        break;
    }
    case OBJ_CLASS: {
        ObjClass *k = (ObjClass *)o;
        fprintf(out, " name=%s", k->name ? k->name->data : "?");
        break;
    }
    case OBJ_INSTANCE: {
        ObjInstance *in = (ObjInstance *)o;
        fprintf(out, " of %s", (in->klass && in->klass->name) ? in->klass->name->data : "?");
        break;
    }
    case OBJ_MODULE: {
        ObjModule *m = (ObjModule *)o;
        fprintf(out, " name=%s", m->name ? m->name->data : "?");
        break;
    }
    default: break;
    }
}

/* ---- the block walk ------------------------------------------------------- */

/* Does this used, unregistered block look like a *forged* Array/ByteArray — the
 * fake object a heap overflow fabricates? We demand a real invariant violation
 * (a valid tag, an in-heap backing pointer, and count > capacity, which a genuine
 * collection never has) so a clean heap yields no false positives. */
static int looks_forged(Obj *o, uint8_t tag, size_t bsize) {
    if (bsize > 512) return 0;               /* objects are small */
    if (tag == OBJ_BYTEARRAY) {
        ObjByteArray *b = (ObjByteArray *)o;
        return alloc_in_heap(b->bytes) && b->count > b->capacity;
    }
    if (tag == OBJ_ARRAY) {
        ObjArray *a = (ObjArray *)o;
        return alloc_in_heap(a->items) && a->count > a->capacity;
    }
    return 0;
}

static void describe_forged(FILE *out, Obj *o, HeapHeader *hh, uint8_t tag) {
    fprintf(out, "?%s UNREGISTERED (hdr.id=%u rc=%u) <<< forged/UAF/corrupt?",
            obj_type_name((ObjType)tag), hh->id, o->refcount);
    if (tag == OBJ_BYTEARRAY) {
        ObjByteArray *b = (ObjByteArray *)o;
        fprintf(out, " count=%u cap=%u", b->count, b->capacity);
    } else if (tag == OBJ_ARRAY) {
        ObjArray *a = (ObjArray *)o;
        fprintf(out, " count=%u cap=%u", a->count, a->capacity);
    }
}

typedef struct { FILE *out; const uint8_t *base; size_t objs, bufs, forged, raw; } DumpCtx;

static void block_cb(void *payload, size_t bsize, int used, void *ud) {
    DumpCtx *c = (DumpCtx *)ud;
    FILE *out = c->out;
    long off = (long)((const uint8_t *)payload - c->base);

    fprintf(out, "  +0x%06lx  %p  %6zu  %s  ",
            off, payload, bsize, used ? "USED" : "free");
    if (!used) { fputc('\n', out); return; }

    /* Exact object test: header.id maps back to this payload's Obj. */
    HeapHeader *hh = (HeapHeader *)payload;
    Obj *o = (Obj *)((uint8_t *)payload + sizeof(HeapHeader));
    if (objmgr_get(hh->id) == o) {
        describe_object(out, o);
        c->objs++;
    } else {
        BufSearch s = { payload, "", 0 };
        objmgr_foreach(buf_probe, &s);
        uint8_t tag = *(uint8_t *)o;                   /* candidate Obj tag */
        if (s.found) { fprintf(out, "%s", s.desc); c->bufs++; }
        else if (looks_forged(o, tag, bsize)) {
            describe_forged(out, o, hh, tag); c->forged++;
        } else { fprintf(out, "<runtime/buffer>"); c->raw++; }
    }
    fputc('\n', out);
}

void heap_dump(FILE *out, const char *label) {
    AllocStats st = alloc_stats();
    fprintf(out, "==== heap dump%s%s ====\n",
            label ? ": " : "", label ? label : "");
    fprintf(out, "arena=%zuB  in_use=%zuB/%zu blk  free=%zuB/%zu blk  "
                 "largest_free=%zuB  live_objs=%zu\n",
            st.arena_size, st.bytes_in_use, st.used_blocks,
            st.bytes_free, st.free_blocks, st.largest_free, objmgr_live_count());
    fprintf(out, "  offset      address              size  state annotation\n");

    DumpCtx c = { out, (const uint8_t *)alloc_arena_base(), 0, 0, 0, 0 };
    if (!c.base) { fprintf(out, "  (arena not initialised)\n"); return; }
    alloc_foreach_block(block_cb, &c);

    fprintf(out, "---- %zu objects, %zu owned buffers, %zu forged?, %zu runtime/other ----\n",
            c.objs, c.bufs, c.forged, c.raw);
}
