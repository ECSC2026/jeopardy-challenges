/* value.h — the tagged VulnVM Value and its operations.
 *
 * A Value is a 16-byte tagged union. Immediate types (Null/Bool/Number/Long/
 * Float/Double/Char/Symbol) carry their data inline; VAL_OBJ carries a pointer to
 * a reference-counted heap object (see object.h). This mirrors MonkeyC semantics
 * where everything is an Object but primitives need no heap allocation.
 */
#ifndef VULNVM_VALUE_H
#define VULNVM_VALUE_H

#include "common.h"

typedef struct Obj Obj; /* forward decl; full definition in object.h */

typedef enum {
    VAL_NULL,
    VAL_BOOL,
    VAL_NUMBER,  /* 32-bit signed int   */
    VAL_LONG,    /* 64-bit signed int   */
    VAL_FLOAT,   /* 32-bit IEEE754      */
    VAL_DOUBLE,  /* 64-bit IEEE754      */
    VAL_CHAR,    /* unicode code point  */
    VAL_SYMBOL,  /* interned symbol id  */
    VAL_OBJ      /* heap object pointer */
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool     boolean;
        int32_t  num;
        int64_t  lng;
        float    f32;
        double   f64;
        uint32_t ch;
        uint32_t sym;
        Obj     *obj;
    } as;
} Value;

/* ------------------------------------------------------- constructors ------ */

static inline Value mk_null(void)         { Value v; v.type = VAL_NULL;   v.as.obj = NULL; return v; }
static inline Value mk_bool(bool b)       { Value v; v.type = VAL_BOOL;   v.as.boolean = b; return v; }
static inline Value mk_number(int32_t n)  { Value v; v.type = VAL_NUMBER; v.as.num = n;  return v; }
static inline Value mk_long(int64_t n)    { Value v; v.type = VAL_LONG;   v.as.lng = n;  return v; }
static inline Value mk_float(float f)     { Value v; v.type = VAL_FLOAT;  v.as.f32 = f;  return v; }
static inline Value mk_double(double d)   { Value v; v.type = VAL_DOUBLE; v.as.f64 = d;  return v; }
static inline Value mk_char(uint32_t c)   { Value v; v.type = VAL_CHAR;   v.as.ch = c;   return v; }
static inline Value mk_symbol(uint32_t s) { Value v; v.type = VAL_SYMBOL; v.as.sym = s;  return v; }
static inline Value mk_obj(Obj *o)        { Value v; v.type = VAL_OBJ;    v.as.obj = o;  return v; }

/* --------------------------------------------------------- predicates ------ */

#define IS_NULL(v)   ((v).type == VAL_NULL)
#define IS_BOOL(v)   ((v).type == VAL_BOOL)
#define IS_NUMBER(v) ((v).type == VAL_NUMBER)
#define IS_LONG(v)   ((v).type == VAL_LONG)
#define IS_FLOAT(v)  ((v).type == VAL_FLOAT)
#define IS_DOUBLE(v) ((v).type == VAL_DOUBLE)
#define IS_CHAR(v)   ((v).type == VAL_CHAR)
#define IS_SYMBOL(v) ((v).type == VAL_SYMBOL)
#define IS_OBJ(v)    ((v).type == VAL_OBJ)

#define AS_BOOL(v)   ((v).as.boolean)
#define AS_NUMBER(v) ((v).as.num)
#define AS_LONG(v)   ((v).as.lng)
#define AS_FLOAT(v)  ((v).as.f32)
#define AS_DOUBLE(v) ((v).as.f64)
#define AS_CHAR(v)   ((v).as.ch)
#define AS_SYMBOL(v) ((v).as.sym)
#define AS_OBJ(v)    ((v).as.obj)

/* ---------------------------------------------------- reference counting --- */

/* retain: if v is an object, increment its refcount. Returns v (for chaining,
 * e.g. `slot = retain(x);`). */
Value retain(Value v);

/* release: if v is an object, decrement its refcount; at zero, run the object's
 * finaliser (releasing children), deregister it, and free it. */
void release(Value v);

/* ----------------------------------------------------- equality / hash ----- */

/* Structural equality: same type & value; strings compare by content; other
 * objects by identity (object id). Numeric cross-type comparison is handled by
 * the VM, not here. */
bool value_equals(Value a, Value b);

/* 32-bit hash consistent with value_equals (strings by content, objects by id). */
uint32_t value_hash(Value v);

/* Render a debug/toString representation into buf (always NUL-terminated).
 * Returns the number of bytes that would be written (may exceed n-1). Objects
 * render their content (strings) or "<Type:id>" using the heap object id. */
size_t value_repr(Value v, char *buf, size_t n);

#endif /* VULNVM_VALUE_H */
