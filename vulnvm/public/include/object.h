/* object.h — heap object model, object manager, interned strings & symbols.
 *
 * Memory layout of every heap object (a single allocation from vm_alloc), exactly
 * as specified for the challenge:
 *
 *     +0   u32  object_size    ┐ HeapHeader (allocator/manager view)
 *     +4   u32  id             │  monotonic, never reused within a run
 *     +8   u32  heap_obj_size  ┘  size of the payload region below
 *     +12  u8   tag (ObjType)  ┐ payload header
 *     +13  u32  refcount       ┘
 *     +17  ...  type-specific data
 *
 * A Value's Obj* points at the payload (the tag, offset +12). The HeapHeader sits
 * immediately before it at ((HeapHeader*)obj) - 1. All header/payload structs are
 * packed so the byte offsets above hold on every target; the compiler emits
 * unaligned-safe accesses for their members.
 */
#ifndef VULNVM_OBJECT_H
#define VULNVM_OBJECT_H

#include "value.h"
#include "chunk.h"

typedef enum {
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_BYTEARRAY,
    OBJ_DICT,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_FUNCTION,
    OBJ_METHOD,
    OBJ_WEAKREF,
    OBJ_EXCEPTION,
    OBJ_MODULE,
    OBJ__COUNT
} ObjType;

typedef struct __attribute__((packed)) {
    uint32_t object_size;   /* allocator-usable bytes of the whole allocation */
    uint32_t id;            /* object id (from the object manager)            */
    uint32_t heap_obj_size; /* logical payload size (tag..end)               */
} HeapHeader;

struct __attribute__((packed)) Obj {
    uint8_t  tag;           /* ObjType */
    uint32_t refcount;
    /* type-specific fields follow in the concrete structs below */
};

/* ---- header helpers (hide the pointer arithmetic / packing) --------------- */

static inline HeapHeader *obj_header(Obj *o) { return ((HeapHeader *)o) - 1; }
static inline uint32_t obj_id(Obj *o)           { return obj_header(o)->id; }
static inline uint32_t obj_size(Obj *o)         { return obj_header(o)->object_size; }
static inline uint32_t obj_payload_size(Obj *o) { return obj_header(o)->heap_obj_size; }
static inline ObjType  obj_type(Obj *o)         { return (ObjType)o->tag; }

const char *obj_type_name(ObjType t);

/* ---- lifecycle ------------------------------------------------------------ */

/* Allocate a heap object with the given tag and payload size (bytes from `tag`
 * to end of the type-specific data). Fills in the HeapHeader, assigns the next
 * object id, sets refcount = 1, and registers it with the object manager.
 * Returns NULL if the arena is exhausted. */
Obj *obj_alloc(ObjType type, size_t payload_size);

/* refcount primitives on the packed field. */
void obj_incref(Obj *o);
void obj_decref(Obj *o); /* frees + finalises at zero */

/* ---- object manager ------------------------------------------------------- */

/* Initialise the object system (manager + intern tables + symbol table). Call
 * once after arena_init. object_system_shutdown releases the tables. */
void object_system_init(void);
void object_system_shutdown(void);

/* Number of live (registered) heap objects — 0 after everything is released. */
size_t objmgr_live_count(void);

/* Look up a live object by id; NULL if it has been freed. Used by weak refs. */
Obj *objmgr_get(uint32_t id);

/* Invoke `cb` for every live (registered) object, in table order. Read-only;
 * used by heap-inspection tooling. Do not allocate/free objects from within cb. */
void objmgr_foreach(void (*cb)(Obj *o, void *ud), void *ud);

/* ---- strings (interned, immutable) ---------------------------------------- */

typedef struct __attribute__((packed)) {
    Obj      obj;
    uint32_t length;  /* number of bytes (UTF-8) */
    uint32_t hash;    /* cached FNV-1a content hash */
    char     data[];  /* `length` bytes + a trailing NUL */
} ObjString;

/* Create/find an interned string. Returns a new owning reference (refcount +1).
 * Equal contents always yield the same ObjString*. */
ObjString *string_new(const char *bytes, size_t len);
ObjString *string_from_cstr(const char *cstr);

/* Convenience accessors. */
static inline const char *string_cstr(ObjString *s) { return s->data; }
static inline uint32_t    string_len(ObjString *s)  { return s->length; }

#define AS_STRING(v)  ((ObjString *)AS_OBJ(v))
#define IS_STRING(v)  (IS_OBJ(v) && obj_type(AS_OBJ(v)) == OBJ_STRING)

/* ---- functions (VM bytecode or native C) ---------------------------------- */

typedef enum { FN_VM, FN_NATIVE } FnKind;

/* A callable. FN_VM wraps a compiled Chunk (executed by pushing a call frame);
 * FN_NATIVE wraps a C function pointer (called directly). The chunk is a
 * separately-allocated, properly-aligned object rather than embedded inline, so
 * chunk pointers handed to chunk_* helpers are correctly aligned despite this
 * struct being packed. */
typedef struct __attribute__((packed)) {
    Obj        obj;
    uint8_t    kind;          /* FnKind */
    uint8_t    arity;
    uint16_t   upvalue_count; /* for future closures */
    uint32_t   frame_size;    /* max value-stack slots this frame needs (VM funcs) */
    ObjString *name;          /* function name (retained); NULL for the script */
    Chunk     *chunk;         /* FN_VM: the compiled bytecode                   */
    void      *native;        /* FN_NATIVE: C function pointer                  */
} ObjFunction;

/* Create a VM (bytecode) function with an empty, ready-to-fill chunk. `name` may
 * be NULL; if given it is retained. Returns with refcount 1. */
ObjFunction *function_new(ObjString *name, uint8_t arity);

/* Create a native function wrapping a C function pointer. */
ObjFunction *function_new_native(ObjString *name, uint8_t arity, void *native);

#define AS_FUNCTION(v) ((ObjFunction *)AS_OBJ(v))
#define IS_FUNCTION(v) (IS_OBJ(v) && obj_type(AS_OBJ(v)) == OBJ_FUNCTION)

/* ---- arrays (dynamic, heterogeneous) -------------------------------------- */

typedef struct __attribute__((packed)) {
    Obj      obj;
    uint32_t count;
    uint32_t capacity;
    Value   *items;   /* separately arena-allocated, 16-byte aligned */
} ObjArray;

ObjArray *array_new(uint32_t capacity);
/* Build an array by MOVING `n` values from `src` (ownership transfers in). */
ObjArray *array_new_from(const Value *src, uint32_t n);
void  array_reserve(ObjArray *a, uint32_t need);
void  array_push(ObjArray *a, Value v);      /* retains v */
Value array_get(ObjArray *a, uint32_t i);    /* borrow */
void  array_set(ObjArray *a, uint32_t i, Value v); /* releases old, retains v */

#define AS_ARRAY(v) ((ObjArray *)AS_OBJ(v))
#define IS_ARRAY(v) (IS_OBJ(v) && obj_type(AS_OBJ(v)) == OBJ_ARRAY)

/* ---- dictionaries (open-addressing hash map) ------------------------------ */

typedef enum { DICT_EMPTY = 0, DICT_USED = 1, DICT_TOMB = 2 } DictSlotState;
typedef struct { uint8_t state; Value key; Value val; } DictEntry;

typedef struct __attribute__((packed)) {
    Obj        obj;
    uint32_t   count;
    uint32_t   capacity;
    uint32_t   tombstones;
    DictEntry *entries;
} ObjDict;

ObjDict *dict_new(void);
bool  dict_get(ObjDict *d, Value key, Value *out);   /* borrow into *out */
void  dict_set(ObjDict *d, Value key, Value val);    /* retains key/val   */
bool  dict_remove(ObjDict *d, Value key);            /* releases stored   */
bool  dict_has(ObjDict *d, Value key);

#define AS_DICT(v) ((ObjDict *)AS_OBJ(v))
#define IS_DICT(v) (IS_OBJ(v) && obj_type(AS_OBJ(v)) == OBJ_DICT)

/* ---- byte arrays ---------------------------------------------------------- */

typedef struct __attribute__((packed)) {
    Obj      obj;
    uint32_t count;
    uint32_t capacity;
    uint8_t *bytes;   /* separately arena-allocated */
} ObjByteArray;

ObjByteArray *bytearray_new(uint32_t capacity);
void bytearray_reserve(ObjByteArray *b, uint32_t need);
void bytearray_push(ObjByteArray *b, uint8_t byte);

#define AS_BYTEARRAY(v) ((ObjByteArray *)AS_OBJ(v))
#define IS_BYTEARRAY(v) (IS_OBJ(v) && obj_type(AS_OBJ(v)) == OBJ_BYTEARRAY)

/* ---- bound methods (obj.method(:sym)) ------------------------------------- */

typedef struct __attribute__((packed)) {
    Obj      obj;
    Value    receiver;  /* retained */
    uint32_t symbol;
} ObjMethod;

ObjMethod *method_new(Value receiver, uint32_t symbol);

#define AS_METHOD(v) ((ObjMethod *)AS_OBJ(v))
#define IS_METHOD(v) (IS_OBJ(v) && obj_type(AS_OBJ(v)) == OBJ_METHOD)

/* ---- weak references (obj.weak()) ----------------------------------------- */

typedef struct __attribute__((packed)) {
    Obj      obj;
    uint32_t target_id;  /* non-owning object id; resolved via the object manager */
} ObjWeakRef;

ObjWeakRef *weakref_new(uint32_t target_id);

#define AS_WEAKREF(v) ((ObjWeakRef *)AS_OBJ(v))
#define IS_WEAKREF(v) (IS_OBJ(v) && obj_type(AS_OBJ(v)) == OBJ_WEAKREF)

/* ---- classes, instances, modules ------------------------------------------ */

typedef struct ObjClass {
    Obj             obj;
    ObjString      *name;     /* retained */
    struct ObjClass *super;   /* retained, or NULL */
    ObjDict        *methods;  /* symbol -> ObjFunction (instance methods) */
    ObjDict        *statics;  /* symbol -> Value (static members)         */
} __attribute__((packed)) ObjClass;

typedef struct __attribute__((packed)) {
    Obj       obj;
    ObjClass *klass;   /* retained */
    ObjDict  *fields;  /* symbol -> Value (instance state) */
} ObjInstance;

typedef struct __attribute__((packed)) {
    Obj        obj;
    ObjString *name;     /* retained */
    ObjDict   *members;  /* symbol -> Value (functions/consts/nested) */
} ObjModule;

ObjClass    *class_new(ObjString *name, ObjClass *super);
ObjInstance *instance_new(ObjClass *klass);
ObjModule   *module_new(ObjString *name);

/* Walk the superclass chain for an instance method. Returns the ObjFunction (as
 * a Value) and, if `owner` is non-NULL, the class it was found in. */
bool class_find_method(ObjClass *k, uint32_t sym, Value *out, ObjClass **owner);
/* True if `k` is `target` or a (transitive) subclass of it. */
bool class_is_subclass(ObjClass *k, ObjClass *target);

#define AS_CLASS(v)    ((ObjClass *)AS_OBJ(v))
#define IS_CLASS(v)    (IS_OBJ(v) && obj_type(AS_OBJ(v)) == OBJ_CLASS)
#define AS_INSTANCE(v) ((ObjInstance *)AS_OBJ(v))
#define IS_INSTANCE(v) (IS_OBJ(v) && obj_type(AS_OBJ(v)) == OBJ_INSTANCE)
#define AS_MODULE(v)   ((ObjModule *)AS_OBJ(v))
#define IS_MODULE(v)   (IS_OBJ(v) && obj_type(AS_OBJ(v)) == OBJ_MODULE)

/* ---- symbols (interned name -> small integer id) -------------------------- */

/* Intern a symbol name, returning its stable id (>= 1). Same name -> same id. */
uint32_t symbol_intern(const char *name, size_t len);
uint32_t symbol_intern_cstr(const char *name);

/* Reverse lookup: the name for an id, or NULL if the id is unknown. */
const char *symbol_name(uint32_t id);

/* Number of interned symbols (for tests/debug). */
size_t symbol_count(void);

#endif /* VULNVM_OBJECT_H */
