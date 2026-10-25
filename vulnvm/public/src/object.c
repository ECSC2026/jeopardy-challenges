/* object.c — heap object model, object manager, string & symbol interning.
 *
 * Everything here allocates from the custom arena (vm_alloc), never libc malloc.
 * Three global tables live for the lifetime of the object system:
 *   - the object manager: open-addressed map id -> Obj*, for teardown/debug and
 *     weak-reference liveness. Ids are assigned monotonically and never reused.
 *   - the string intern table: open-addressed *weak* set of ObjString*, so equal
 *     string contents share one object. Entries are removed when a string's
 *     refcount hits zero.
 *   - the symbol table: name -> small integer id (interned), with reverse lookup.
 */
#include "object.h"
#include "alloc.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------ hashing ------ */

static uint32_t fnv1a(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

/* ------------------------------------------------------- type names -------- */

static const char *const kTypeNames[OBJ__COUNT] = {
    "String", "Array", "ByteArray", "Dictionary", "Class",
    "Object" /*Instance*/, "Method" /*Function*/, "Method",
    "WeakReference", "Exception", "Module",
};

const char *obj_type_name(ObjType t) {
    if ((unsigned)t >= OBJ__COUNT) return "?";
    return kTypeNames[t];
}

/* ================================================================ manager == */

typedef struct { uint32_t id; Obj *obj; } MgrEntry;

static struct {
    MgrEntry *slots;
    size_t    cap;        /* power of two */
    size_t    count;      /* live entries */
    size_t    tombstones;
    uint32_t  next_id;    /* monotonic id source */
} g_mgr;

#define MGR_TOMB ((Obj *)(uintptr_t)1) /* sentinel for a deleted slot */

static void mgr_init(size_t cap) {
    g_mgr.cap = cap;
    g_mgr.count = 0;
    g_mgr.tombstones = 0;
    g_mgr.next_id = 1;
    g_mgr.slots = (MgrEntry *)vm_alloc(cap * sizeof(MgrEntry));
    if (!g_mgr.slots) vm_panic("object manager: out of arena memory at init");
    memset(g_mgr.slots, 0, cap * sizeof(MgrEntry));
}

/* Integer hash (Fibonacci-ish) to scatter sequential ids across the table. */
static inline size_t id_hash(uint32_t id) { return (size_t)(id * 2654435761u); }

static void mgr_insert_raw(MgrEntry *slots, size_t cap, uint32_t id, Obj *obj) {
    size_t mask = cap - 1;
    size_t i = id_hash(id) & mask;
    while (slots[i].obj != NULL && slots[i].obj != MGR_TOMB) {
        i = (i + 1) & mask;
    }
    slots[i].id = id;
    slots[i].obj = obj;
}

static void mgr_grow(void) {
    size_t ncap = g_mgr.cap * 2;
    MgrEntry *ns = (MgrEntry *)vm_alloc(ncap * sizeof(MgrEntry));
    if (!ns) vm_panic("object manager: out of arena memory on grow");
    memset(ns, 0, ncap * sizeof(MgrEntry));
    for (size_t i = 0; i < g_mgr.cap; i++) {
        if (g_mgr.slots[i].obj && g_mgr.slots[i].obj != MGR_TOMB)
            mgr_insert_raw(ns, ncap, g_mgr.slots[i].id, g_mgr.slots[i].obj);
    }
    vm_free(g_mgr.slots);
    g_mgr.slots = ns;
    g_mgr.cap = ncap;
    g_mgr.tombstones = 0;
}

static uint32_t mgr_register(Obj *obj) {
    /* Grow if load (count + tombstones) exceeds 70%. */
    if ((g_mgr.count + g_mgr.tombstones) * 10 >= g_mgr.cap * 7) mgr_grow();
    uint32_t id = g_mgr.next_id++;
    mgr_insert_raw(g_mgr.slots, g_mgr.cap, id, obj);
    g_mgr.count++;
    return id;
}

static void mgr_unregister(uint32_t id) {
    size_t mask = g_mgr.cap - 1;
    size_t i = id_hash(id) & mask;
    while (g_mgr.slots[i].obj != NULL) {
        if (g_mgr.slots[i].obj != MGR_TOMB && g_mgr.slots[i].id == id) {
            g_mgr.slots[i].obj = MGR_TOMB;
            g_mgr.count--;
            g_mgr.tombstones++;
            return;
        }
        i = (i + 1) & mask;
    }
}

Obj *objmgr_get(uint32_t id) {
    if (!g_mgr.slots) return NULL;
    size_t mask = g_mgr.cap - 1;
    size_t i = id_hash(id) & mask;
    while (g_mgr.slots[i].obj != NULL) {
        if (g_mgr.slots[i].obj != MGR_TOMB && g_mgr.slots[i].id == id)
            return g_mgr.slots[i].obj;
        i = (i + 1) & mask;
    }
    return NULL;
}

size_t objmgr_live_count(void) { return g_mgr.count; }

void objmgr_foreach(void (*cb)(Obj *o, void *ud), void *ud) {
    if (!g_mgr.slots) return;
    for (size_t i = 0; i < g_mgr.cap; i++) {
        Obj *o = g_mgr.slots[i].obj;
        if (o && o != MGR_TOMB) cb(o, ud);
    }
}

/* ============================================================ intern table = */

static struct {
    ObjString **slots;
    size_t      cap;
    size_t      count;
    size_t      tombstones;
} g_intern;

#define STR_TOMB ((ObjString *)(uintptr_t)1)

static void intern_init(size_t cap) {
    g_intern.cap = cap;
    g_intern.count = 0;
    g_intern.tombstones = 0;
    g_intern.slots = (ObjString **)vm_alloc(cap * sizeof(ObjString *));
    if (!g_intern.slots) vm_panic("intern table: out of arena memory at init");
    memset(g_intern.slots, 0, cap * sizeof(ObjString *));
}

static void intern_insert_raw(ObjString **slots, size_t cap, ObjString *s) {
    size_t mask = cap - 1;
    size_t i = s->hash & mask;
    while (slots[i] != NULL && slots[i] != STR_TOMB) i = (i + 1) & mask;
    slots[i] = s;
}

static void intern_grow(void) {
    size_t ncap = g_intern.cap * 2;
    ObjString **ns = (ObjString **)vm_alloc(ncap * sizeof(ObjString *));
    if (!ns) vm_panic("intern table: out of arena memory on grow");
    memset(ns, 0, ncap * sizeof(ObjString *));
    for (size_t i = 0; i < g_intern.cap; i++) {
        if (g_intern.slots[i] && g_intern.slots[i] != STR_TOMB)
            intern_insert_raw(ns, ncap, g_intern.slots[i]);
    }
    vm_free(g_intern.slots);
    g_intern.slots = ns;
    g_intern.cap = ncap;
    g_intern.tombstones = 0;
}

static ObjString *intern_find(const char *bytes, size_t len, uint32_t hash) {
    size_t mask = g_intern.cap - 1;
    size_t i = hash & mask;
    while (g_intern.slots[i] != NULL) {
        ObjString *s = g_intern.slots[i];
        if (s != STR_TOMB && s->hash == hash && s->length == len &&
            memcmp(s->data, bytes, len) == 0)
            return s;
        i = (i + 1) & mask;
    }
    return NULL;
}

static void intern_remove(ObjString *s) {
    size_t mask = g_intern.cap - 1;
    size_t i = s->hash & mask;
    while (g_intern.slots[i] != NULL) {
        if (g_intern.slots[i] == s) {
            g_intern.slots[i] = STR_TOMB;
            g_intern.count--;
            g_intern.tombstones++;
            return;
        }
        i = (i + 1) & mask;
    }
}

/* ============================================================ symbol table = */

static struct {
    uint32_t *index;      /* hash slots -> id (0 = empty)      */
    size_t    index_cap;  /* power of two                       */
    char    **names;      /* id-1 -> interned name (arena copy) */
    size_t    names_count;
    size_t    names_cap;
} g_sym;

static void sym_init(size_t index_cap, size_t names_cap) {
    g_sym.index_cap = index_cap;
    g_sym.index = (uint32_t *)vm_alloc(index_cap * sizeof(uint32_t));
    if (!g_sym.index) vm_panic("symbol table: out of arena memory at init");
    memset(g_sym.index, 0, index_cap * sizeof(uint32_t));
    g_sym.names_cap = names_cap;
    g_sym.names_count = 0;
    g_sym.names = (char **)vm_alloc(names_cap * sizeof(char *));
    if (!g_sym.names) vm_panic("symbol table: out of arena memory at init");
    memset(g_sym.names, 0, names_cap * sizeof(char *));
}

static void sym_index_grow(void) {
    size_t ncap = g_sym.index_cap * 2;
    uint32_t *ni = (uint32_t *)vm_alloc(ncap * sizeof(uint32_t));
    if (!ni) vm_panic("symbol table: out of arena memory on grow");
    memset(ni, 0, ncap * sizeof(uint32_t));
    size_t mask = ncap - 1;
    for (size_t i = 0; i < g_sym.index_cap; i++) {
        uint32_t id = g_sym.index[i];
        if (id == 0) continue;
        const char *nm = g_sym.names[id - 1];
        size_t j = fnv1a(nm, strlen(nm)) & mask;
        while (ni[j] != 0) j = (j + 1) & mask;
        ni[j] = id;
    }
    vm_free(g_sym.index);
    g_sym.index = ni;
    g_sym.index_cap = ncap;
}

uint32_t symbol_intern(const char *name, size_t len) {
    uint32_t hash = fnv1a(name, len);
    size_t mask = g_sym.index_cap - 1;
    size_t i = hash & mask;
    while (g_sym.index[i] != 0) {
        uint32_t id = g_sym.index[i];
        const char *nm = g_sym.names[id - 1];
        if (strlen(nm) == len && memcmp(nm, name, len) == 0) return id;
        i = (i + 1) & mask;
    }
    /* Not found: create a new symbol. */
    if (g_sym.names_count + 1 >= g_sym.names_cap) {
        size_t ncap = g_sym.names_cap * 2;
        g_sym.names = (char **)vm_realloc(g_sym.names, ncap * sizeof(char *));
        if (!g_sym.names) vm_panic("symbol table: out of arena memory");
        g_sym.names_cap = ncap;
    }
    char *copy = (char *)vm_alloc(len + 1);
    if (!copy) vm_panic("symbol table: out of arena memory");
    memcpy(copy, name, len);
    copy[len] = '\0';
    uint32_t id = (uint32_t)(++g_sym.names_count); /* ids start at 1 */
    g_sym.names[id - 1] = copy;

    /* Grow index if load >= 70%, then insert. */
    if ((g_sym.names_count) * 10 >= g_sym.index_cap * 7) {
        sym_index_grow();
        mask = g_sym.index_cap - 1;
    }
    i = hash & mask;
    while (g_sym.index[i] != 0) i = (i + 1) & mask;
    g_sym.index[i] = id;
    return id;
}

uint32_t symbol_intern_cstr(const char *name) {
    return symbol_intern(name, strlen(name));
}

const char *symbol_name(uint32_t id) {
    if (id == 0 || id > g_sym.names_count) return NULL;
    return g_sym.names[id - 1];
}

size_t symbol_count(void) { return g_sym.names_count; }

/* ============================================================ lifecycle ==== */

void object_system_init(void) {
    mgr_init(64);
    intern_init(64);
    sym_init(128, 64);
}

void object_system_shutdown(void) {
    if (g_mgr.slots)    { vm_free(g_mgr.slots);    g_mgr.slots = NULL; }
    if (g_intern.slots) { vm_free(g_intern.slots); g_intern.slots = NULL; }
    if (g_sym.index)    { vm_free(g_sym.index);    g_sym.index = NULL; }
    if (g_sym.names) {
        for (size_t i = 0; i < g_sym.names_count; i++) vm_free(g_sym.names[i]);
        vm_free(g_sym.names);
        g_sym.names = NULL;
    }
    memset(&g_mgr, 0, sizeof(g_mgr));
    memset(&g_intern, 0, sizeof(g_intern));
    memset(&g_sym, 0, sizeof(g_sym));
}

/* ---- allocation ----------------------------------------------------------- */

Obj *obj_alloc(ObjType type, size_t payload_size) {
    size_t total = sizeof(HeapHeader) + payload_size;
    void *base = vm_alloc(total);
    if (!base) return NULL;

    HeapHeader *h = (HeapHeader *)base;
    h->object_size = (uint32_t)vm_alloc_usable_size(base);
    h->heap_obj_size = (uint32_t)payload_size;

    Obj *o = (Obj *)((uint8_t *)base + sizeof(HeapHeader));
    o->tag = (uint8_t)type;
    o->refcount = 1;

    h->id = mgr_register(o);
    return o;
}

void obj_incref(Obj *o) { o->refcount++; }

/* Finalise (release children / drop external table entries) then free. */
static void obj_destroy(Obj *o) {
    switch (obj_type(o)) {
    case OBJ_STRING:
        intern_remove((ObjString *)o);
        break;
    case OBJ_FUNCTION: {
        ObjFunction *fn = (ObjFunction *)o;
        if (fn->kind == FN_VM && fn->chunk) {
            chunk_free(fn->chunk);
            vm_free(fn->chunk);
        }
        if (fn->name) obj_decref((Obj *)fn->name);
        break;
    }
    case OBJ_ARRAY: {
        ObjArray *a = (ObjArray *)o;
        for (uint32_t i = 0; i < a->count; i++) release(a->items[i]);
        if (a->items) vm_free(a->items);
        break;
    }
    case OBJ_DICT: {
        ObjDict *d = (ObjDict *)o;
        for (uint32_t i = 0; i < d->capacity; i++) {
            if (d->entries[i].state == DICT_USED) {
                release(d->entries[i].key);
                release(d->entries[i].val);
            }
        }
        if (d->entries) vm_free(d->entries);
        break;
    }
    case OBJ_BYTEARRAY: {
        ObjByteArray *b = (ObjByteArray *)o;
        if (b->bytes) vm_free(b->bytes);
        break;
    }
    case OBJ_METHOD:
        release(((ObjMethod *)o)->receiver);
        break;
    case OBJ_WEAKREF:
        break; /* holds a non-owning id */
    case OBJ_CLASS: {
        ObjClass *k = (ObjClass *)o;
        if (k->name) obj_decref((Obj *)k->name);
        if (k->super) obj_decref((Obj *)k->super);
        if (k->methods) obj_decref((Obj *)k->methods);
        if (k->statics) obj_decref((Obj *)k->statics);
        break;
    }
    case OBJ_INSTANCE: {
        ObjInstance *in = (ObjInstance *)o;
        if (in->klass) obj_decref((Obj *)in->klass);
        if (in->fields) obj_decref((Obj *)in->fields);
        break;
    }
    case OBJ_MODULE: {
        ObjModule *m = (ObjModule *)o;
        if (m->name) obj_decref((Obj *)m->name);
        if (m->members) obj_decref((Obj *)m->members);
        break;
    }
    /* Remaining types (e.g. strings) own no child references to release here. */
    default:
        break;
    }
    mgr_unregister(obj_id(o));
    vm_free(obj_header(o)); /* free from the HeapHeader base */
}

void obj_decref(Obj *o) {
    if (o->refcount == 0) vm_panic("obj_decref: refcount underflow (id %u)", obj_id(o));
    if (--o->refcount == 0) obj_destroy(o);
}

/* ---- strings -------------------------------------------------------------- */

ObjString *string_new(const char *bytes, size_t len) {
    uint32_t hash = fnv1a(bytes, len);

    ObjString *found = intern_find(bytes, len, hash);
    if (found) {
        obj_incref((Obj *)found);
        return found;
    }

    /* payload = [tag+refcount+length+hash] fields + data bytes + trailing NUL */
    size_t payload = offsetof(ObjString, data) + len + 1;
    Obj *o = obj_alloc(OBJ_STRING, payload);
    if (!o) return NULL;

    ObjString *s = (ObjString *)o;
    s->length = (uint32_t)len;
    s->hash = hash;
    memcpy(s->data, bytes, len);
    s->data[len] = '\0';

    if ((g_intern.count + g_intern.tombstones) * 10 >= g_intern.cap * 7) intern_grow();
    intern_insert_raw(g_intern.slots, g_intern.cap, s);
    g_intern.count++;
    return s;
}

ObjString *string_from_cstr(const char *cstr) {
    return string_new(cstr, strlen(cstr));
}

/* ---- arrays --------------------------------------------------------------- */

ObjArray *array_new(uint32_t capacity) {
    Obj *o = obj_alloc(OBJ_ARRAY, sizeof(ObjArray));
    if (!o) return NULL;
    ObjArray *a = (ObjArray *)o;
    a->count = 0;
    a->capacity = capacity;
    a->items = NULL;
    if (capacity) {
        a->items = (Value *)vm_alloc((size_t)capacity * sizeof(Value));
        if (!a->items) { a->capacity = 0; obj_decref(o); return NULL; } /* arena exhausted */
    }
    return a;
}

ObjArray *array_new_from(const Value *src, uint32_t n) {
    ObjArray *a = array_new(n);
    if (!a) return NULL;
    if (n) memcpy(a->items, src, (size_t)n * sizeof(Value)); /* move (ownership transfers) */
    a->count = n;
    return a;
}

void array_reserve(ObjArray *a, uint32_t need) {
    if (need <= a->capacity) return;
    uint32_t nc = a->capacity < 8 ? 8 : a->capacity;
    while (nc < need) {
        if (nc > UINT32_MAX / 2) { nc = need; break; } /* avoid doubling overflow */
        nc *= 2;
    }
    Value *ni = (Value *)vm_realloc(a->items, (size_t)nc * sizeof(Value));
    /* array_push/reserve return void and can't propagate an error, so treat arena
     * exhaustion as a clean fatal abort rather than writing through a NULL items. */
    if (!ni) vm_panic("out of memory growing array to %u elements", nc);
    a->items = ni;
    a->capacity = nc;
}

void array_push(ObjArray *a, Value v) {
    array_reserve(a, a->count + 1);
    a->items[a->count++] = retain(v);
}

Value array_get(ObjArray *a, uint32_t i) { return a->items[i]; }

void array_set(ObjArray *a, uint32_t i, Value v) {
    release(a->items[i]);
    a->items[i] = retain(v);
}

/* ---- byte arrays ---------------------------------------------------------- */

ObjByteArray *bytearray_new(uint32_t capacity) {
    Obj *o = obj_alloc(OBJ_BYTEARRAY, sizeof(ObjByteArray));
    if (!o) return NULL;
    ObjByteArray *b = (ObjByteArray *)o;
    b->count = 0;
    b->capacity = capacity;
    b->bytes = NULL;
    if (capacity) {
        b->bytes = (uint8_t *)vm_alloc(capacity);
        if (!b->bytes) { b->capacity = 0; obj_decref(o); return NULL; }
    }
    return b;
}

void bytearray_reserve(ObjByteArray *b, uint32_t need) {
    if (need <= b->capacity) return;
    uint32_t nc = b->capacity < 16 ? 16 : b->capacity;
    while (nc < need) {
        if (nc > UINT32_MAX / 2) { nc = need; break; } /* avoid doubling overflow */
        nc *= 2;
    }
    uint8_t *nb = (uint8_t *)vm_realloc(b->bytes, nc);
    if (!nb) vm_panic("out of memory growing byte array to %u bytes", nc);
    b->bytes = nb;
    b->capacity = nc;
}

void bytearray_push(ObjByteArray *b, uint8_t byte) {
    bytearray_reserve(b, b->count + 1);
    b->bytes[b->count++] = byte;
}

/* ---- bound methods & weak references -------------------------------------- */

ObjMethod *method_new(Value receiver, uint32_t symbol) {
    Obj *o = obj_alloc(OBJ_METHOD, sizeof(ObjMethod));
    if (!o) return NULL;
    ObjMethod *m = (ObjMethod *)o;
    m->receiver = retain(receiver);
    m->symbol = symbol;
    return m;
}

ObjWeakRef *weakref_new(uint32_t target_id) {
    Obj *o = obj_alloc(OBJ_WEAKREF, sizeof(ObjWeakRef));
    if (!o) return NULL;
    ObjWeakRef *w = (ObjWeakRef *)o;
    w->target_id = target_id;
    return w;
}

/* ---- classes / instances / modules ---------------------------------------- */

ObjClass *class_new(ObjString *name, ObjClass *super) {
    Obj *o = obj_alloc(OBJ_CLASS, sizeof(ObjClass));
    if (!o) return NULL;
    ObjClass *k = (ObjClass *)o;
    k->name = name;
    if (name) obj_incref((Obj *)name);
    k->super = super;
    if (super) obj_incref((Obj *)super);
    k->methods = dict_new();
    k->statics = dict_new();
    if (!k->methods || !k->statics) { obj_decref(o); return NULL; }
    return k;
}

ObjInstance *instance_new(ObjClass *klass) {
    Obj *o = obj_alloc(OBJ_INSTANCE, sizeof(ObjInstance));
    if (!o) return NULL;
    ObjInstance *in = (ObjInstance *)o;
    in->klass = klass;
    if (klass) obj_incref((Obj *)klass);
    in->fields = dict_new();
    if (!in->fields) { obj_decref(o); return NULL; }
    return in;
}

ObjModule *module_new(ObjString *name) {
    Obj *o = obj_alloc(OBJ_MODULE, sizeof(ObjModule));
    if (!o) return NULL;
    ObjModule *m = (ObjModule *)o;
    m->name = name;
    if (name) obj_incref((Obj *)name);
    m->members = dict_new();
    if (!m->members) { obj_decref(o); return NULL; }
    return m;
}

bool class_find_method(ObjClass *k, uint32_t sym, Value *out, ObjClass **owner) {
    Value key = mk_symbol(sym);
    for (ObjClass *c = k; c; c = c->super) {
        if (dict_get(c->methods, key, out)) { if (owner) *owner = c; return true; }
    }
    return false;
}

bool class_is_subclass(ObjClass *k, ObjClass *target) {
    for (ObjClass *c = k; c; c = c->super) if (c == target) return true;
    return false;
}

/* ---- dictionaries --------------------------------------------------------- */

#define DICT_MIN_CAP 8

ObjDict *dict_new(void) {
    Obj *o = obj_alloc(OBJ_DICT, sizeof(ObjDict));
    if (!o) return NULL;
    ObjDict *d = (ObjDict *)o;
    d->count = 0;
    d->tombstones = 0;
    d->capacity = DICT_MIN_CAP;
    d->entries = (DictEntry *)vm_alloc(DICT_MIN_CAP * sizeof(DictEntry));
    memset(d->entries, 0, DICT_MIN_CAP * sizeof(DictEntry));
    return d;
}

/* Find the slot for `key`: the matching used slot, or the first insertable slot
 * (empty or the first tombstone seen). Returns index; *found set accordingly. */
static uint32_t dict_slot(DictEntry *entries, uint32_t cap, Value key,
                          uint32_t hash, bool *found) {
    uint32_t mask = cap - 1;
    uint32_t i = hash & mask;
    uint32_t first_tomb = cap; /* sentinel */
    for (;;) {
        DictEntry *e = &entries[i];
        if (e->state == DICT_EMPTY) {
            *found = false;
            return (first_tomb != cap) ? first_tomb : i;
        }
        if (e->state == DICT_TOMB) {
            if (first_tomb == cap) first_tomb = i;
        } else if (value_equals(e->key, key)) {
            *found = true;
            return i;
        }
        i = (i + 1) & mask;
    }
}

static void dict_grow(ObjDict *d) {
    uint32_t ncap = d->capacity * 2;
    DictEntry *ne = (DictEntry *)vm_alloc((size_t)ncap * sizeof(DictEntry));
    memset(ne, 0, (size_t)ncap * sizeof(DictEntry));
    for (uint32_t i = 0; i < d->capacity; i++) {
        DictEntry *e = &d->entries[i];
        if (e->state != DICT_USED) continue;
        bool found;
        uint32_t j = dict_slot(ne, ncap, e->key, value_hash(e->key), &found);
        ne[j] = *e;
    }
    vm_free(d->entries);
    d->entries = ne;
    d->capacity = ncap;
    d->tombstones = 0;
}

bool dict_get(ObjDict *d, Value key, Value *out) {
    bool found;
    uint32_t i = dict_slot(d->entries, d->capacity, key, value_hash(key), &found);
    if (found && out) *out = d->entries[i].val;
    return found;
}

bool dict_has(ObjDict *d, Value key) {
    bool found;
    dict_slot(d->entries, d->capacity, key, value_hash(key), &found);
    return found;
}

void dict_set(ObjDict *d, Value key, Value val) {
    if ((d->count + d->tombstones + 1) * 10 >= d->capacity * 7) dict_grow(d);
    bool found;
    uint32_t i = dict_slot(d->entries, d->capacity, key, value_hash(key), &found);
    DictEntry *e = &d->entries[i];
    if (found) {
        release(e->val);
        e->val = retain(val);
        return;
    }
    if (e->state == DICT_TOMB) d->tombstones--;
    e->state = DICT_USED;
    e->key = retain(key);
    e->val = retain(val);
    d->count++;
}

bool dict_remove(ObjDict *d, Value key) {
    bool found;
    uint32_t i = dict_slot(d->entries, d->capacity, key, value_hash(key), &found);
    if (!found) return false;
    release(d->entries[i].key);
    release(d->entries[i].val);
    d->entries[i].state = DICT_TOMB;
    d->entries[i].key = mk_null();
    d->entries[i].val = mk_null();
    d->count--;
    d->tombstones++;
    return true;
}

/* ---- functions ------------------------------------------------------------ */

ObjFunction *function_new(ObjString *name, uint8_t arity) {
    /* Payload is the whole ObjFunction (it begins with the Obj header). */
    Obj *o = obj_alloc(OBJ_FUNCTION, sizeof(ObjFunction));
    if (!o) return NULL;
    ObjFunction *fn = (ObjFunction *)o;
    fn->kind = FN_VM;
    fn->arity = arity;
    fn->upvalue_count = 0;
    fn->frame_size = 0;
    fn->name = name;
    if (name) obj_incref((Obj *)name);
    fn->chunk = (Chunk *)vm_alloc(sizeof(Chunk));
    if (!fn->chunk) { obj_decref(o); return NULL; }
    chunk_init(fn->chunk);
    fn->native = NULL;
    return fn;
}

ObjFunction *function_new_native(ObjString *name, uint8_t arity, void *native) {
    Obj *o = obj_alloc(OBJ_FUNCTION, sizeof(ObjFunction));
    if (!o) return NULL;
    ObjFunction *fn = (ObjFunction *)o;
    fn->kind = FN_NATIVE;
    fn->arity = arity;
    fn->upvalue_count = 0;
    fn->frame_size = 0;
    fn->name = name;
    if (name) obj_incref((Obj *)name);
    fn->chunk = NULL;
    fn->native = native;
    return fn;
}
