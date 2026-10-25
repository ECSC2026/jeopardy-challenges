/* vm.c — VulnVM bytecode execution engine. */
#include "vm.h"
#include "vm_internal.h"
#include "builtins.h"
#include "alloc.h"
#include "serialize.h"
#include "debug.h"
#include "chunk.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#define FRAMES_MAX 256
#define STACK_MAX  (FRAMES_MAX * 256)

typedef struct {
    ObjFunction *fn;
    uint8_t     *ip;
    Value       *slots;
    ObjClass    *method_class; /* class defining this method (NULL for plain calls) */
} CallFrame;

/* An active try block: where to resume (catch_ip) and how far to unwind. */
typedef struct {
    int      frame_count; /* frame_count when the try was entered */
    Value   *sp;          /* value-stack top when the try was entered */
    uint8_t *catch_ip;    /* where to jump when an exception is thrown */
} Handler;

#define HANDLERS_MAX 256

/* Global name -> value table (open addressing, keyed by interned ObjString). */
typedef struct { ObjString *key; Value val; } GEntry;

struct VM {
    Value     *stack;
    Value     *sp;
    CallFrame  frames[FRAMES_MAX];
    int        frame_count;

    GEntry    *globals;
    int        globals_cap, globals_count;

    ObjFunction *script;   /* currently running script (kept alive) */
    bool       has_error;
    char       error[256];
    bool       trace;

    Handler    handlers[HANDLERS_MAX];
    int        handler_count;
    Value      pending_exc;    /* exception being thrown/propagated (owned)   */
    ObjClass  *exc_class;      /* cached base Lang.Exception class (borrowed)  */
    uint32_t   sym_message;    /* interned "mMessage" symbol                   */
};

/* ------------------------------------------------------------- stack prims - */

/* takes ownership. Defensive backstop only: correct frame-size reservation (see
 * call_function / vm_interpret) guarantees this never fires; if it ever did, a
 * frame_size miscalculation would be a compiler bug, so abort rather than corrupt. */
static void push(VM *vm, Value v) {
    if (VM_UNLIKELY(vm->sp >= vm->stack + STACK_MAX)) vm_panic("value stack overflow");
    *vm->sp++ = v;
}
static Value pop(VM *vm)          { return *--vm->sp; }           /* gives ownership */
static Value peek(VM *vm, int d)  { return vm->sp[-1 - d]; }      /* borrow */

static void reset_stack(VM *vm) {
    for (Value *p = vm->stack; p < vm->sp; p++) release(*p);
    vm->sp = vm->stack;
    vm->frame_count = 0;
    vm->handler_count = 0;
}

/* Build a catchable exception value from an error message: an instance of the
 * base Exception class (with mMessage set) once the class is registered, else a
 * plain String (used before/while the exception prelude is loaded). Owned. */
static Value make_exception(VM *vm, const char *msg) {
    if (vm->exc_class) {
        ObjInstance *in = instance_new(vm->exc_class);
        if (in) {
            ObjString *s = string_from_cstr(msg);
            if (s) {
                dict_set(in->fields, mk_symbol(vm->sym_message), mk_obj((Obj *)s));
                release(mk_obj((Obj *)s));
            }
            return mk_obj((Obj *)in);
        }
    }
    ObjString *s = string_from_cstr(msg);
    return s ? mk_obj((Obj *)s) : mk_null();
}

/* Fill vm->error with a human-readable description of an exception value. */
static void describe_exception(VM *vm, Value exc) {
    if (IS_INSTANCE(exc)) {
        ObjInstance *in = AS_INSTANCE(exc);
        const char *cname = (in->klass && in->klass->name) ? in->klass->name->data : "Exception";
        Value m;
        if (dict_get(in->fields, mk_symbol(vm->sym_message), &m) && IS_STRING(m))
            snprintf(vm->error, sizeof(vm->error), "%s: %s", cname, AS_STRING(m)->data);
        else
            snprintf(vm->error, sizeof(vm->error), "unhandled %s", cname);
    } else {
        ObjString *s = value_to_string(exc);
        snprintf(vm->error, sizeof(vm->error), "%.*s", (int)s->length, s->data);
        release(mk_obj((Obj *)s));
    }
}

static bool is_falsey(Value v) {
    return IS_NULL(v) || (IS_BOOL(v) && !AS_BOOL(v));
}

static const char *value_type_name(Value v) {
    switch (v.type) {
    case VAL_NULL:   return "Null";
    case VAL_BOOL:   return "Boolean";
    case VAL_NUMBER: return "Number";
    case VAL_LONG:   return "Long";
    case VAL_FLOAT:  return "Float";
    case VAL_DOUBLE: return "Double";
    case VAL_CHAR:   return "Char";
    case VAL_SYMBOL: return "Symbol";
    case VAL_OBJ:    return obj_type_name(obj_type(v.as.obj));
    }
    return "?";
}

/* ------------------------------------------------------------ error report - */

/* Format the error message + current source line and mark the VM errored, but do
 * not touch the stack. */
static void set_error_v(VM *vm, const char *fmt, va_list ap) {
    vsnprintf(vm->error, sizeof(vm->error), fmt, ap);
    vm->has_error = true;
    if (vm->frame_count > 0) {
        CallFrame *f = &vm->frames[vm->frame_count - 1];
        int off = (int)(f->ip - f->fn->chunk->code) - 1;
        if (off >= 0 && off < f->fn->chunk->count) {
            int line = f->fn->chunk->lines[off];
            size_t n = strlen(vm->error);
            snprintf(vm->error + n, sizeof(vm->error) - n, " (line %d)", line);
        }
    }
}

/* Report an error and unwind (releases the whole in-flight stack). */
/* Set the pending exception from an error message. Returns false so callers can
 * `return runtime_error(...)`. The invoking opcode/loop propagates it (goto
 * propagate), giving try/catch a chance to handle it. */
static void set_pending_v(VM *vm, const char *fmt, va_list ap) {
    set_error_v(vm, fmt, ap);
    if (!IS_NULL(vm->pending_exc)) { release(vm->pending_exc); vm->pending_exc = mk_null(); }
    vm->pending_exc = make_exception(vm, vm->error);
}
static bool runtime_error(VM *vm, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    set_pending_v(vm, fmt, ap);
    va_end(ap);
    return false;
}

/* Native-method-facing error (same behaviour; kept as a public entry point). */
bool vm_raise(VM *vm, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    set_pending_v(vm, fmt, ap);
    va_end(ap);
    return false;
}

/* --------------------------------------------------------------- globals --- */

static void globals_grow(VM *vm) {
    int ncap = vm->globals_cap < 16 ? 16 : vm->globals_cap * 2;
    GEntry *ne = (GEntry *)vm_alloc((size_t)ncap * sizeof(GEntry));
    memset(ne, 0, (size_t)ncap * sizeof(GEntry));
    for (int i = 0; i < vm->globals_cap; i++) {
        if (!vm->globals[i].key) continue;
        ObjString *k = vm->globals[i].key;
        size_t j = k->hash & (ncap - 1);
        while (ne[j].key) j = (j + 1) & (ncap - 1);
        ne[j] = vm->globals[i];
    }
    if (vm->globals) vm_free(vm->globals);
    vm->globals = ne;
    vm->globals_cap = ncap;
}

static GEntry *globals_find(VM *vm, ObjString *key) {
    if (vm->globals_cap == 0) return NULL;
    size_t mask = vm->globals_cap - 1;
    size_t i = key->hash & mask;
    while (vm->globals[i].key) {
        if (vm->globals[i].key == key) return &vm->globals[i];
        i = (i + 1) & mask;
    }
    return NULL;
}

/* Define or assign a global. Retains the key (once) and the value; releases a
 * previously stored value on overwrite. */
static void globals_set(VM *vm, ObjString *key, Value v) {
    if ((vm->globals_count + 1) * 10 >= vm->globals_cap * 7) globals_grow(vm);
    size_t mask = vm->globals_cap - 1;
    size_t i = key->hash & mask;
    while (vm->globals[i].key) {
        if (vm->globals[i].key == key) {
            release(vm->globals[i].val);
            vm->globals[i].val = retain(v);
            return;
        }
        i = (i + 1) & mask;
    }
    vm->globals[i].key = key;
    obj_incref((Obj *)key);
    vm->globals[i].val = retain(v);
    vm->globals_count++;
}

/* -------------------------------------------------------------- natives ---- */

void vm_define_native(VM *vm, const char *name, uint8_t arity, NativeFn fn) {
    ObjString *nm = string_from_cstr(name);
    ObjFunction *native = function_new_native(nm, arity, (void *)fn);
    globals_set(vm, nm, mk_obj((Obj *)native));
    release(mk_obj((Obj *)native));
    release(mk_obj((Obj *)nm));
}

/* Simple built-ins so programs can produce observable output. */
static Value native_println(VM *vm, Value recv, int argc, Value *args) {
    (void)vm; (void)recv;
    if (argc >= 1) {
        ObjString *s = value_to_string(args[0]);
        printf("%.*s\n", (int)s->length, s->data);
        release(mk_obj((Obj *)s));
    } else printf("\n");
    return mk_null();
}
static Value native_print(VM *vm, Value recv, int argc, Value *args) {
    (void)vm; (void)recv;
    if (argc >= 1) {
        ObjString *s = value_to_string(args[0]);
        printf("%.*s", (int)s->length, s->data);
        release(mk_obj((Obj *)s));
    }
    return mk_null();
}

/* dumpHeap([label]) — print the whole heap (blocks + live objects) to stdout. A
 * debugging aid: allocation-free, so it does not disturb the heap it reports. */
static Value native_dump_heap(VM *vm, Value recv, int argc, Value *args) {
    (void)vm; (void)recv;
    char label[128];
    const char *lp = NULL;
    if (argc >= 1) {
        ObjString *s = value_to_string(args[0]);
        uint32_t n = s->length < sizeof(label) - 1 ? s->length : (uint32_t)sizeof(label) - 1;
        memcpy(label, s->data, n);
        label[n] = '\0';
        lp = label;
        release(mk_obj((Obj *)s));   /* freed before the dump, so it stays clean */
    }
    fflush(stdout);
    heap_dump(stdout, lp);
    fflush(stdout);
    return mk_null();
}

/* heapOk() — true if the allocator metadata is self-consistent (boundary tags,
 * coalescing, free lists). Handy for spotting exactly when corruption lands. */
static Value native_heap_ok(VM *vm, Value recv, int argc, Value *args) {
    (void)vm; (void)recv; (void)argc; (void)args;
    return mk_bool(alloc_check_heap(false));
}

/* --------------------------------------------------- numeric helpers ------- */

static int numrank(Value v) {
    switch (v.type) {
    case VAL_NUMBER: return 0;
    case VAL_LONG:   return 1;
    case VAL_FLOAT:  return 2;
    case VAL_DOUBLE: return 3;
    default:         return -1;
    }
}
static bool is_numeric(Value v) { return numrank(v) >= 0; }
static int64_t as_i64(Value v) {
    switch (v.type) {
    case VAL_NUMBER: return v.as.num;
    case VAL_LONG:   return v.as.lng;
    case VAL_FLOAT:  return (int64_t)v.as.f32;
    case VAL_DOUBLE: return (int64_t)v.as.f64;
    default:         return 0;
    }
}
static double as_f64(Value v) {
    switch (v.type) {
    case VAL_NUMBER: return v.as.num;
    case VAL_LONG:   return (double)v.as.lng;
    case VAL_FLOAT:  return v.as.f32;
    case VAL_DOUBLE: return v.as.f64;
    default:         return 0;
    }
}

/* Build a String from the toString() display of two values (for `+`). */
static ObjString *concat_display(Value a, Value b) {
    ObjString *sa = value_to_string(a); /* owned */
    ObjString *sb = value_to_string(b);
    uint32_t al = sa->length, bl = sb->length;
    char *tmp = (char *)vm_alloc((size_t)al + (size_t)bl + 1);
    memcpy(tmp, sa->data, al);
    memcpy(tmp + al, sb->data, bl);
    ObjString *s = string_new(tmp, (size_t)al + (size_t)bl);
    vm_free(tmp);
    release(mk_obj((Obj *)sa));
    release(mk_obj((Obj *)sb));
    return s;
}

/* ------------------------------------------------------------ arithmetic --- */

/* Pops b then a, computes, pushes result. Returns false and reports on error. */
static bool do_arith(VM *vm, OpCode op) {
    Value b = peek(vm, 0), a = peek(vm, 1);

    /* String concatenation for `+`. */
    if (op == OP_ADD && (IS_STRING(a) || IS_STRING(b))) {
        ObjString *s = concat_display(a, b);
        pop(vm); pop(vm);
        release(a); release(b);
        push(vm, mk_obj((Obj *)s)); /* refcount 1, owned by stack */
        return true;
    }
    if (!is_numeric(a) || !is_numeric(b)) {
        runtime_error(vm, "operands must be numeric");
        return false;
    }

    int r = numrank(a) > numrank(b) ? numrank(a) : numrank(b);
    Value result;
    if (r <= 1) { /* integer arithmetic — wrap on overflow (2's complement) */
        int64_t x = as_i64(a), y = as_i64(b), z = 0;
        switch (op) {
        case OP_ADD: z = (int64_t)((uint64_t)x + (uint64_t)y); break;
        case OP_SUB: z = (int64_t)((uint64_t)x - (uint64_t)y); break;
        case OP_MUL: z = (int64_t)((uint64_t)x * (uint64_t)y); break;
        case OP_DIV:
            if (y == 0) { runtime_error(vm, "integer division by zero"); return false; }
            z = (x == INT64_MIN && y == -1) ? INT64_MIN : x / y; break;
        case OP_MOD:
            if (y == 0) { runtime_error(vm, "integer modulo by zero"); return false; }
            z = (x == INT64_MIN && y == -1) ? 0 : x % y; break;
        default: break;
        }
        result = (r == 0) ? mk_number((int32_t)z) : mk_long(z);
    } else if (r == 2) { /* float */
        float x = (float)as_f64(a), y = (float)as_f64(b), z = 0;
        switch (op) {
        case OP_ADD: z = x + y; break;
        case OP_SUB: z = x - y; break;
        case OP_MUL: z = x * y; break;
        case OP_DIV: z = x / y; break;
        case OP_MOD: z = fmodf(x, y); break;
        default: break;
        }
        result = mk_float(z);
    } else { /* double */
        double x = as_f64(a), y = as_f64(b), z = 0;
        switch (op) {
        case OP_ADD: z = x + y; break;
        case OP_SUB: z = x - y; break;
        case OP_MUL: z = x * y; break;
        case OP_DIV: z = x / y; break;
        case OP_MOD: z = fmod(x, y); break;
        default: break;
        }
        result = mk_double(z);
    }
    pop(vm); pop(vm);
    release(a); release(b);
    push(vm, result);
    return true;
}

static bool do_bitwise(VM *vm, OpCode op) {
    Value b = peek(vm, 0), a = peek(vm, 1);
    if ((a.type != VAL_NUMBER && a.type != VAL_LONG) ||
        (b.type != VAL_NUMBER && b.type != VAL_LONG)) {
        runtime_error(vm, "bitwise operands must be integers");
        return false;
    }
    bool wide = (a.type == VAL_LONG || b.type == VAL_LONG);
    int64_t x = as_i64(a), y = as_i64(b), z = 0;
    /* A 32-bit Number cannot represent a shift by >= 32 bits, so widen the result
     * to Long (e.g. assembling a 64-bit value from bytes: `b << 40`). The Long
     * then propagates through any surrounding bitwise chain. */
    if ((op == OP_SHL || op == OP_SHR) && y >= 32) wide = true;
    switch (op) {
    case OP_BAND: z = x & y; break;
    case OP_BOR:  z = x | y; break;
    case OP_BXOR: z = x ^ y; break;
    /* Shift in unsigned to avoid UB on negative/sign-bit shifts; mask the amount
     * to the operand width (63 for Long, 31 for the 32-bit Number). */
    case OP_SHL:
        if (wide) z = (int64_t)((uint64_t)x << (unsigned)(y & 63));
        else      z = (int32_t)((uint32_t)(int32_t)x << (unsigned)(y & 31));
        break;
    case OP_SHR:
        if (wide) z = x >> (unsigned)(y & 63);         /* arithmetic (defined) */
        else      z = (int32_t)x >> (unsigned)(y & 31);
        break;
    default: break;
    }
    pop(vm); pop(vm);
    push(vm, wide ? mk_long(z) : mk_number((int32_t)z));
    return true;
}

static bool do_compare(VM *vm, OpCode op) {
    Value b = peek(vm, 0), a = peek(vm, 1);
    int cmp;
    if (is_numeric(a) && is_numeric(b)) {
        if (numrank(a) <= 1 && numrank(b) <= 1) {
            int64_t x = as_i64(a), y = as_i64(b);
            cmp = (x < y) ? -1 : (x > y) ? 1 : 0;
        } else {
            double x = as_f64(a), y = as_f64(b);
            cmp = (x < y) ? -1 : (x > y) ? 1 : 0;
        }
    } else if (IS_STRING(a) && IS_STRING(b)) {
        ObjString *sa = AS_STRING(a), *sb = AS_STRING(b);
        int m = memcmp(sa->data, sb->data, sa->length < sb->length ? sa->length : sb->length);
        cmp = m ? (m < 0 ? -1 : 1) : (int)sa->length - (int)sb->length;
    } else if (IS_CHAR(a) && IS_CHAR(b)) {
        cmp = (AS_CHAR(a) < AS_CHAR(b)) ? -1 : (AS_CHAR(a) > AS_CHAR(b)) ? 1 : 0;
    } else {
        runtime_error(vm, "operands are not comparable");
        return false;
    }
    bool res = (op == OP_LT) ? cmp < 0 : (op == OP_GT) ? cmp > 0
             : (op == OP_LE) ? cmp <= 0 : cmp >= 0;
    pop(vm); pop(vm);
    release(a); release(b);
    push(vm, mk_bool(res));
    return true;
}

static bool values_equal_numeric(Value a, Value b) {
    if (is_numeric(a) && is_numeric(b)) {
        if (numrank(a) <= 1 && numrank(b) <= 1) return as_i64(a) == as_i64(b);
        return as_f64(a) == as_f64(b);
    }
    return value_equals(a, b);
}

/* ------------------------------------------------------------- calls ------- */

static bool call_function(VM *vm, ObjFunction *fn, int argc) {
    if (fn->kind == FN_NATIVE) {
        NativeFn native = (NativeFn)fn->native;
        Value result = native(vm, mk_null(), argc, vm->sp - argc);
        /* pop function + args */
        for (Value *p = vm->sp - argc - 1; p < vm->sp; p++) release(*p);
        vm->sp -= argc + 1;
        push(vm, result);
        return !vm->has_error;
    }
    if (argc != fn->arity) {
        runtime_error(vm, "expected %d argument(s) but got %d", fn->arity, argc);
        return false;
    }
    if (vm->frame_count == FRAMES_MAX) { runtime_error(vm, "call stack overflow"); return false; }
    /* Reserve the callee's whole frame so no push during it can overflow the value
     * stack. The args are already on the stack (they are slots 0..argc); the frame
     * needs `frame_size` slots from its base at sp-argc-1. */
    if ((vm->sp - vm->stack) - argc - 1 + (long)fn->frame_size > STACK_MAX) {
        runtime_error(vm, "value stack overflow (frame needs %u slots)", fn->frame_size);
        return false;
    }
    CallFrame *f = &vm->frames[vm->frame_count++];
    f->fn = fn;
    f->ip = fn->chunk->code;
    f->slots = vm->sp - argc - 1; /* slot 0 = the function/receiver */
    f->method_class = NULL;
    return true;
}

static bool call_value(VM *vm, Value callee, int argc) {
    if (IS_FUNCTION(callee)) return call_function(vm, AS_FUNCTION(callee), argc);
    runtime_error(vm, "can only call functions");
    return false;
}

/* Re-entrant dispatch loop; returns when frame_count drops back to `stop`. */
static VMResult run(VM *vm, int stop);

/* Call a VM function to completion synchronously. The receiver + args must already
 * be on the stack (receiver at sp-argc-1); on success the result is left on the
 * stack top. Used for field initialisers and (later) callbacks. */
static bool call_sync(VM *vm, ObjFunction *fn, int argc, ObjClass *mclass) {
    int base = vm->frame_count;
    if (!call_function(vm, fn, argc)) return false;
    vm->frames[vm->frame_count - 1].method_class = mclass;
    return run(vm, base) == VM_OK;
}

/* Run instance-field initializers for the whole class chain, base class first, so
 * inherited fields are set even when a subclass does not call the parent's
 * initialize. Each class's own $fieldinit (if any) is called with `self`. */
static bool run_field_inits(VM *vm, ObjClass *k, Value self, uint32_t fisym) {
    if (!k) return true;
    if (k->super && !run_field_inits(vm, k->super, self, fisym)) return false;
    Value fiv;
    if (dict_get(k->methods, mk_symbol(fisym), &fiv) && IS_FUNCTION(fiv)) {
        push(vm, retain(self));                 /* receiver for the 0-arg $fieldinit */
        if (!call_sync(vm, AS_FUNCTION(fiv), 0, k)) return false;
        release(pop(vm));                        /* discard $fieldinit's result */
    }
    return true;
}

/* ------------------------------------------------------------- run loop ---- */

static VMResult run(VM *vm, int stop) {
    CallFrame *frame = &vm->frames[vm->frame_count - 1];

#define READ_BYTE()  (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONST() (frame->fn->chunk->constants.values[READ_SHORT()])
/* Raise a runtime error (as a catchable exception) and route to `propagate`. */
#define RAISE(...) do { runtime_error(vm, __VA_ARGS__); goto propagate; } while (0)

    for (;;) {
        if (vm->trace) {
            printf("          ");
            for (Value *s = vm->stack; s < vm->sp; s++) {
                char b[64]; value_repr(*s, b, sizeof(b));
                printf("[ %s ]", b);
            }
            printf("\n");
            chunk_disasm_instruction(frame->fn->chunk,
                                     (int)(frame->ip - frame->fn->chunk->code), stdout);
        }

        uint8_t inst = READ_BYTE();
        switch (inst) {
        case OP_CONSTANT: push(vm, retain(READ_CONST())); break;
        case OP_NULL:  push(vm, mk_null()); break;
        case OP_TRUE:  push(vm, mk_bool(true)); break;
        case OP_FALSE: push(vm, mk_bool(false)); break;
        case OP_POP:   release(pop(vm)); break;
        case OP_DUP:   push(vm, retain(peek(vm, 0))); break;
        case OP_DUP2: {
            Value a = peek(vm, 1), b = peek(vm, 0);
            push(vm, retain(a)); push(vm, retain(b));
            break;
        }
        case OP_GET_LOCAL: { uint16_t s = READ_SHORT(); push(vm, retain(frame->slots[s])); break; }
        case OP_SET_LOCAL: {
            uint16_t s = READ_SHORT();
            release(frame->slots[s]);
            frame->slots[s] = retain(peek(vm, 0));
            break;
        }
        case OP_GET_GLOBAL: {
            ObjString *name = AS_STRING(READ_CONST());
            GEntry *e = globals_find(vm, name);
            if (!e) RAISE("undefined variable '%s'", name->data);
            push(vm, retain(e->val));
            break;
        }
        case OP_SET_GLOBAL: {
            ObjString *name = AS_STRING(READ_CONST());
            if (!globals_find(vm, name))
                RAISE("assignment to undefined variable '%s'", name->data);
            globals_set(vm, name, peek(vm, 0)); /* keeps value on stack */
            break;
        }
        case OP_DEFINE_GLOBAL: {
            ObjString *name = AS_STRING(READ_CONST());
            globals_set(vm, name, peek(vm, 0));
            release(pop(vm));
            break;
        }
        case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: case OP_MOD:
            if (!do_arith(vm, (OpCode)inst)) goto propagate;
            break;
        case OP_BAND: case OP_BOR: case OP_BXOR: case OP_SHL: case OP_SHR:
            if (!do_bitwise(vm, (OpCode)inst)) goto propagate;
            break;
        case OP_LT: case OP_GT: case OP_LE: case OP_GE:
            if (!do_compare(vm, (OpCode)inst)) goto propagate;
            break;
        case OP_EQ: {
            Value b = pop(vm), a = pop(vm);
            bool e = values_equal_numeric(a, b);
            release(a); release(b);
            push(vm, mk_bool(e));
            break;
        }
        case OP_NEQ: {
            Value b = pop(vm), a = pop(vm);
            bool e = values_equal_numeric(a, b);
            release(a); release(b);
            push(vm, mk_bool(!e));
            break;
        }
        case OP_NEG: {
            Value v = peek(vm, 0);
            if (!is_numeric(v)) RAISE("operand of '-' must be numeric");
            pop(vm);
            switch (v.type) {
            /* Negate via unsigned so INT32_MIN/INT64_MIN wrap instead of trapping. */
            case VAL_NUMBER: push(vm, mk_number((int32_t)(0u - (uint32_t)v.as.num))); break;
            case VAL_LONG:   push(vm, mk_long((int64_t)(0ull - (uint64_t)v.as.lng))); break;
            case VAL_FLOAT:  push(vm, mk_float(-v.as.f32)); break;
            default:         push(vm, mk_double(-v.as.f64)); break;
            }
            break;
        }
        case OP_BNOT: {
            Value v = peek(vm, 0);
            if (v.type != VAL_NUMBER && v.type != VAL_LONG)
                RAISE("operand of '~' must be an integer");
            pop(vm);
            if (v.type == VAL_NUMBER) push(vm, mk_number(~v.as.num));
            else push(vm, mk_long(~v.as.lng));
            break;
        }
        case OP_NOT: {
            Value v = pop(vm);
            bool f = is_falsey(v);
            release(v);
            push(vm, mk_bool(f));
            break;
        }
        case OP_JUMP: { uint16_t d = READ_SHORT(); frame->ip += d; break; }
        case OP_JUMP_IF_FALSE: {
            uint16_t d = READ_SHORT();
            Value v = pop(vm);
            if (is_falsey(v)) frame->ip += d;
            release(v);
            break;
        }
        case OP_JUMP_IF_TRUE: {
            uint16_t d = READ_SHORT();
            Value v = pop(vm);
            if (!is_falsey(v)) frame->ip += d;
            release(v);
            break;
        }
        case OP_LOOP: { uint16_t d = READ_SHORT(); frame->ip -= d; break; }
        case OP_CALL: {
            int argc = READ_BYTE();
            if (!call_value(vm, peek(vm, argc), argc)) goto propagate;
            frame = &vm->frames[vm->frame_count - 1];
            break;
        }
        case OP_RETURN: {
            Value result = pop(vm);
            CallFrame *done = &vm->frames[vm->frame_count - 1];
            for (Value *p = done->slots; p < vm->sp; p++) release(*p);
            vm->sp = done->slots;
            vm->frame_count--;
            /* Drop any handlers left by a return-inside-try (safety net). */
            while (vm->handler_count > 0 &&
                   vm->handlers[vm->handler_count - 1].frame_count > vm->frame_count)
                vm->handler_count--;
            if (vm->frame_count == stop) {
                if (stop == 0) { release(result); return VM_OK; } /* top-level script */
                push(vm, result);                                 /* nested sync call */
                return VM_OK;
            }
            push(vm, result);
            frame = &vm->frames[vm->frame_count - 1];
            break;
        }
        case OP_BUILD_ARRAY: {
            uint16_t n = READ_SHORT();
            ObjArray *a = array_new_from(vm->sp - n, n); /* moves n values in */
            if (!a) RAISE("out of memory building array");
            vm->sp -= n;                                  /* values moved, not released */
            push(vm, mk_obj((Obj *)a));
            break;
        }
        case OP_BUILD_DICT: {
            uint16_t n = READ_SHORT();
            ObjDict *d = dict_new();
            if (!d) RAISE("out of memory building dictionary");
            Value *base = vm->sp - 2 * n;
            for (uint16_t i = 0; i < n; i++) dict_set(d, base[2 * i], base[2 * i + 1]);
            for (Value *p = base; p < vm->sp; p++) release(*p);
            vm->sp = base;
            push(vm, mk_obj((Obj *)d));
            break;
        }
        case OP_BUILD_BYTEARRAY: {
            uint16_t n = READ_SHORT();
            ObjByteArray *b = bytearray_new(n);
            if (!b) RAISE("out of memory building byte array");
            Value *base = vm->sp - n;
            for (uint16_t i = 0; i < n; i++) {
                Value v = base[i];
                if (!IS_NUMBER(v) && !IS_CHAR(v)) {
                    obj_decref((Obj *)b);
                    RAISE("byte array element must be a Number or Char");
                }
                b->bytes[i] = (uint8_t)(IS_NUMBER(v) ? v.as.num : (int)v.as.ch);
            }
            b->count = n;
            vm->sp -= n;                 /* scalar elements: no refs to release */
            push(vm, mk_obj((Obj *)b));
            break;
        }
        case OP_NEW_ARRAY: {
            Value sz = pop(vm);
            if (!IS_NUMBER(sz)) RAISE("array size must be a Number");
            if (sz.as.num < 0) RAISE("array size must be non-negative");
            ObjArray *a = array_new((uint32_t)sz.as.num);
            if (!a) RAISE("cannot allocate array of size %d", sz.as.num);
            for (int32_t i = 0; i < sz.as.num; i++) a->items[i] = mk_null();
            a->count = (uint32_t)sz.as.num;
            push(vm, mk_obj((Obj *)a));
            break;
        }
        case OP_GET_INDEX: {
            Value idx = peek(vm, 0), obj = peek(vm, 1);
            Value out;
            if (IS_ARRAY(obj)) {
                if (!IS_NUMBER(idx)) RAISE("array index must be a Number");
                ObjArray *a = AS_ARRAY(obj);
                int32_t i = idx.as.num;
                if (i < 0 || (uint32_t)i >= a->count)
                    RAISE("array index %d out of bounds (size %u)", i, a->count);
                out = retain(a->items[i]);
            } else if (IS_DICT(obj)) {
                Value v;
                out = dict_get(AS_DICT(obj), idx, &v) ? retain(v) : mk_null();
            } else if (IS_STRING(obj)) {
                if (!IS_NUMBER(idx)) RAISE("string index must be a Number");
                ObjString *s = AS_STRING(obj);
                int32_t i = idx.as.num;
                if (i < 0 || (uint32_t)i >= s->length)
                    RAISE("string index %d out of bounds (length %u)", i, s->length);
                out = mk_char((unsigned char)s->data[i]);
            } else if (IS_BYTEARRAY(obj)) {
                if (!IS_NUMBER(idx)) RAISE("byte array index must be a Number");
                ObjByteArray *ba = AS_BYTEARRAY(obj);
                int32_t i = idx.as.num;
                if (i < 0 || (uint32_t)i >= ba->count)
                    RAISE("byte array index %d out of bounds (size %u)", i, ba->count);
                out = mk_number(ba->bytes[i]);
            } else {
                RAISE("cannot index a %s", value_type_name(obj));
            }
            pop(vm); pop(vm);
            release(idx); release(obj);
            push(vm, out);
            break;
        }
        case OP_SET_INDEX: {
            Value val = peek(vm, 0), idx = peek(vm, 1), obj = peek(vm, 2);
            if (IS_ARRAY(obj)) {
                if (!IS_NUMBER(idx)) RAISE("array index must be a Number");
                ObjArray *a = AS_ARRAY(obj);
                int32_t i = idx.as.num;
                if (i < 0 || (uint32_t)i >= a->count)
                    RAISE("array index %d out of bounds (size %u)", i, a->count);
                array_set(a, (uint32_t)i, val);
            } else if (IS_DICT(obj)) {
                dict_set(AS_DICT(obj), idx, val);
            } else if (IS_BYTEARRAY(obj)) {
                if (!IS_NUMBER(idx)) RAISE("byte array index must be a Number");
                if (!IS_NUMBER(val) && !IS_CHAR(val))
                    RAISE("byte array value must be a Number or Char");
                ObjByteArray *ba = AS_BYTEARRAY(obj);
                int32_t i = idx.as.num;
                if (i < 0 || (uint32_t)i >= ba->count)
                    RAISE("byte array index %d out of bounds (size %u)", i, ba->count);
                ba->bytes[i] = (uint8_t)(IS_NUMBER(val) ? val.as.num : (int)val.as.ch);
            } else {
                RAISE("cannot index-assign a %s", value_type_name(obj));
            }
            Value res = retain(val);
            pop(vm); pop(vm); pop(vm);
            release(val); release(idx); release(obj);
            push(vm, res);
            break;
        }
        case OP_INVOKE: {
            Value symv = READ_CONST();
            uint8_t argc = READ_BYTE();
            uint32_t sym = symv.as.sym;
            Value recv = peek(vm, argc);
            Value mv;
            ObjClass *owner;
            /* User-defined dispatch: instance method chain, class static, module fn. */
            if (IS_INSTANCE(recv)) {
                if (class_find_method(AS_INSTANCE(recv)->klass, sym, &mv, &owner)) {
                    if (!call_function(vm, AS_FUNCTION(mv), argc)) goto propagate;
                    vm->frames[vm->frame_count - 1].method_class = owner;
                    frame = &vm->frames[vm->frame_count - 1];
                    break;
                }
            } else if (IS_CLASS(recv)) {
                if (dict_get(AS_CLASS(recv)->statics, symv, &mv) && IS_FUNCTION(mv)) {
                    if (!call_function(vm, AS_FUNCTION(mv), argc)) goto propagate;
                    frame = &vm->frames[vm->frame_count - 1];
                    break;
                }
                /* Qualified instance-method call `ClassName.method(args)` (the MonkeyC
                 * super-call idiom): run ClassName's method with the CURRENT self. */
                if (class_find_method(AS_CLASS(recv), sym, &mv, &owner)) {
                    if (!frame->method_class)
                        RAISE("cannot call instance method '%s' on class %s without an instance",
                                             symbol_name(sym), AS_CLASS(recv)->name->data);
                    Value self = frame->slots[0];
                    Value *slot = vm->sp - argc - 1; /* holds the class */
                    release(*slot);
                    *slot = retain(self);
                    if (!call_function(vm, AS_FUNCTION(mv), argc)) goto propagate;
                    vm->frames[vm->frame_count - 1].method_class = owner;
                    frame = &vm->frames[vm->frame_count - 1];
                    break;
                }
            } else if (IS_MODULE(recv)) {
                if (dict_get(AS_MODULE(recv)->members, symv, &mv) && IS_FUNCTION(mv)) {
                    if (!call_function(vm, AS_FUNCTION(mv), argc)) goto propagate;
                    frame = &vm->frames[vm->frame_count - 1];
                    break;
                }
            }
            /* Fall back to native/built-in method dispatch (incl. instance base methods). */
            Value result;
            if (!vm_invoke(vm, recv, sym, argc, vm->sp - argc, &result)) {
                goto propagate;
            }
            for (Value *p = vm->sp - argc - 1; p < vm->sp; p++) release(*p);
            vm->sp -= argc + 1;
            push(vm, result);
            break;
        }
        case OP_INVOKE_SUPER: {
            Value symv = READ_CONST();
            uint8_t argc = READ_BYTE();
            ObjClass *defining = frame->method_class;
            if (!defining || !defining->super)
                RAISE("no superclass for super.%s()", symbol_name(symv.as.sym));
            Value mv; ObjClass *owner;
            if (!class_find_method(defining->super, symv.as.sym, &mv, &owner))
                RAISE("superclass has no method '%s'", symbol_name(symv.as.sym));
            if (!call_function(vm, AS_FUNCTION(mv), argc)) goto propagate;
            vm->frames[vm->frame_count - 1].method_class = owner;
            frame = &vm->frames[vm->frame_count - 1];
            break;
        }
        case OP_CLASS: {
            ObjClass *k = class_new(AS_STRING(READ_CONST()), NULL);
            if (!k) RAISE("out of memory creating class");
            push(vm, mk_obj((Obj *)k));
            break;
        }
        case OP_INHERIT: {
            Value super = peek(vm, 0), klass = peek(vm, 1);
            if (!IS_CLASS(super)) RAISE("superclass must be a class");
            ObjClass *k = AS_CLASS(klass);
            if (k->super) obj_decref((Obj *)k->super);
            k->super = AS_CLASS(super);
            obj_incref((Obj *)k->super);
            pop(vm);
            release(super);
            break;
        }
        case OP_METHOD: {
            Value symv = READ_CONST();
            Value fn = peek(vm, 0), klass = peek(vm, 1);
            dict_set(AS_CLASS(klass)->methods, symv, fn);
            pop(vm);
            release(fn);
            break;
        }
        case OP_MEMBER: {
            Value symv = READ_CONST();
            Value val = peek(vm, 0), ns = peek(vm, 1);
            ObjDict *target = IS_CLASS(ns) ? AS_CLASS(ns)->statics
                            : IS_MODULE(ns) ? AS_MODULE(ns)->members : NULL;
            if (!target) RAISE("internal: OP_MEMBER on non-namespace");
            dict_set(target, symv, val);
            pop(vm);
            release(val);
            break;
        }
        case OP_MODULE: {
            ObjModule *m = module_new(AS_STRING(READ_CONST()));
            if (!m) RAISE("out of memory creating module");
            push(vm, mk_obj((Obj *)m));
            break;
        }
        case OP_NEW: {
            uint8_t argc = READ_BYTE();
            Value classv = peek(vm, argc);
            if (!IS_CLASS(classv)) RAISE("'new' requires a class, got %s",
                                                        value_type_name(classv));
            ObjClass *k = AS_CLASS(classv);
            ObjInstance *newinst = instance_new(k);
            if (!newinst) RAISE("out of memory creating instance");
            Value *slot = vm->sp - argc - 1;
            release(*slot);                        /* release the class stack ref */
            *slot = mk_obj((Obj *)newinst);        /* becomes slot 0 / self */
            /* Run field initializers for the whole chain (base -> derived) first. */
            if (!run_field_inits(vm, k, *slot, symbol_intern_cstr("$fieldinit")))
                goto propagate;
            frame = &vm->frames[vm->frame_count - 1]; /* run_field_inits may have re-based */
            Value initv; ObjClass *owner;
            if (class_find_method(k, symbol_intern_cstr("initialize"), &initv, &owner)) {
                if (!call_function(vm, AS_FUNCTION(initv), argc)) goto propagate;
                vm->frames[vm->frame_count - 1].method_class = owner;
                frame = &vm->frames[vm->frame_count - 1];
            } else if (argc != 0) {
                RAISE("class %s has no initialize accepting %d argument(s)",
                                     k->name->data, argc);
            } else {
                for (Value *p = slot + 1; p < vm->sp; p++) release(*p);
                vm->sp = slot + 1;
            }
            break;
        }
        case OP_GET_PROPERTY: {
            Value symv = READ_CONST();
            uint32_t sym = symv.as.sym;
            Value obj = peek(vm, 0);
            Value out;
            if (IS_INSTANCE(obj)) {
                ObjInstance *in = AS_INSTANCE(obj);
                Value v, mv;
                if (dict_get(in->fields, symv, &v)) out = retain(v);
                else if (class_find_method(in->klass, sym, &mv, NULL)) {
                    ObjMethod *m = method_new(obj, sym);
                    if (!m) RAISE("out of memory");
                    out = mk_obj((Obj *)m);
                } else out = mk_null();
            } else if (IS_CLASS(obj)) {
                Value v;
                out = dict_get(AS_CLASS(obj)->statics, symv, &v) ? retain(v) : mk_null();
            } else if (IS_MODULE(obj)) {
                Value v;
                out = dict_get(AS_MODULE(obj)->members, symv, &v) ? retain(v) : mk_null();
            } else {
                RAISE("cannot read property '%s' of %s",
                                     symbol_name(sym), value_type_name(obj));
            }
            pop(vm);
            release(obj);
            push(vm, out);
            break;
        }
        case OP_SET_PROPERTY: {
            Value symv = READ_CONST();
            Value val = peek(vm, 0), obj = peek(vm, 1);
            if (IS_INSTANCE(obj)) dict_set(AS_INSTANCE(obj)->fields, symv, val);
            else if (IS_CLASS(obj)) dict_set(AS_CLASS(obj)->statics, symv, val);
            else if (IS_MODULE(obj)) dict_set(AS_MODULE(obj)->members, symv, val);
            else RAISE("cannot set property '%s' of %s",
                                      symbol_name(symv.as.sym), value_type_name(obj));
            Value res = retain(val);
            pop(vm); pop(vm);
            release(val); release(obj);
            push(vm, res);
            break;
        }
        case OP_INSTANCEOF: {
            Value type = pop(vm), obj = pop(vm);
            bool is = IS_CLASS(type) && IS_INSTANCE(obj) &&
                      class_is_subclass(AS_INSTANCE(obj)->klass, AS_CLASS(type));
            release(type); release(obj);
            push(vm, mk_bool(is));
            break;
        }
        case OP_HAS: {
            Value symv = READ_CONST();
            Value obj = peek(vm, 0);
            bool has = vm_has_method(obj, symv.as.sym);
            pop(vm);
            release(obj);
            push(vm, mk_bool(has));
            break;
        }
        case OP_TRY_PUSH: {
            uint16_t off = READ_SHORT();
            if (vm->handler_count >= HANDLERS_MAX) RAISE("too many nested try blocks");
            Handler *h = &vm->handlers[vm->handler_count++];
            h->frame_count = vm->frame_count;
            h->sp = vm->sp;
            h->catch_ip = frame->ip + off;
            break;
        }
        case OP_TRY_POP:
            if (vm->handler_count > 0) vm->handler_count--;
            break;
        case OP_THROW: {
            Value exc = pop(vm); /* owned; hand to pending_exc */
            if (!IS_NULL(vm->pending_exc)) release(vm->pending_exc);
            vm->pending_exc = exc;
            describe_exception(vm, exc);
            vm->has_error = true;
            goto propagate;
        }
        default:
            RAISE("opcode %s is not yet implemented",
                                 opcode_name((OpCode)inst));
        }
        continue;

    propagate:
        /* An exception is pending. Catch it at the nearest try handler that lives
         * within this run() invocation's frames; otherwise report (top level) or
         * propagate to the enclosing run() (nested sync call). */
        if (vm->handler_count > 0 && vm->handlers[vm->handler_count - 1].frame_count > stop) {
            Handler h = vm->handlers[--vm->handler_count];
            vm->frame_count = h.frame_count;
            for (Value *p = h.sp; p < vm->sp; p++) release(*p);
            vm->sp = h.sp;
            Value exc = vm->pending_exc;
            vm->pending_exc = mk_null();
            push(vm, exc);                     /* exception for the catch dispatch */
            frame = &vm->frames[vm->frame_count - 1];
            frame->ip = h.catch_ip;
            vm->has_error = false;
            continue;
        }
        if (stop == 0) {                       /* uncaught at top level */
            describe_exception(vm, vm->pending_exc);
            vm->has_error = true;
            release(vm->pending_exc);
            vm->pending_exc = mk_null();
            reset_stack(vm);
            return VM_RUNTIME_ERROR;
        }
        return VM_RUNTIME_ERROR;               /* propagate to the enclosing run() */
    }
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONST
}

/* --------------------------------------------------------- public API ------ */

VM *vm_new(void) {
    VM *vm = (VM *)vm_alloc(sizeof(VM));
    if (!vm) vm_panic("out of memory allocating VM");
    memset(vm, 0, sizeof(*vm));
    vm->stack = (Value *)vm_alloc(STACK_MAX * sizeof(Value));
    if (!vm->stack) vm_panic("arena too small for the VM value stack (%zu bytes)",
                             (size_t)STACK_MAX * sizeof(Value));
    vm->sp = vm->stack;
    vm_define_native(vm, "println", 1, native_println);
    vm_define_native(vm, "print", 1, native_print);
    vm_define_native(vm, "dumpHeap", 0, native_dump_heap);
    vm_define_native(vm, "heapOk", 0, native_heap_ok);

    /* ByteArray number-format and endianness constants (globals). */
    static const struct { const char *name; int32_t val; } kConsts[] = {
        {"NUMBER_FORMAT_UINT8", 0}, {"NUMBER_FORMAT_SINT8", 1},
        {"NUMBER_FORMAT_UINT16", 2}, {"NUMBER_FORMAT_SINT16", 3},
        {"NUMBER_FORMAT_UINT32", 4}, {"NUMBER_FORMAT_SINT32", 5},
        {"NUMBER_FORMAT_FLOAT", 6}, {"NUMBER_FORMAT_DOUBLE", 7},
        {"NUMBER_FORMAT_UINT64", 8}, {"NUMBER_FORMAT_SINT64", 9},
        {"ENDIAN_BIG", 0}, {"ENDIAN_LITTLE", 1},
    };
    for (size_t i = 0; i < sizeof(kConsts) / sizeof(kConsts[0]); i++) {
        ObjString *nm = string_from_cstr(kConsts[i].name);
        globals_set(vm, nm, mk_number(kConsts[i].val));
        release(mk_obj((Obj *)nm));
    }

    /* Load the Toybox.Lang exception hierarchy from the precompiled prelude blob
     * (built from runtime/prelude.mc). Running it here — rather than compiling
     * source — is what lets the interpreter link the VM without the compiler. */
    vm->sym_message = symbol_intern_cstr("mMessage");
    extern const unsigned char vulnvm_prelude_blob[];
    extern const unsigned int  vulnvm_prelude_blob_len;
    char perr[128];
    ObjFunction *prelude = program_read(vulnvm_prelude_blob, vulnvm_prelude_blob_len,
                                        perr, sizeof perr);
    if (!prelude) vm_panic("failed to load exception prelude: %s", perr);
    if (vm_interpret(vm, prelude) != VM_OK)
        vm_panic("failed to run exception prelude: %s", vm->error);
    release(mk_obj((Obj *)prelude));
    Value ev;
    if (vm_get_global(vm, "Exception", &ev) && IS_CLASS(ev)) vm->exc_class = AS_CLASS(ev);
    return vm;
}

void vm_free_vm(VM *vm) {
    if (!vm) return;
    reset_stack(vm);
    release(vm->pending_exc);
    vm->pending_exc = mk_null();
    for (int i = 0; i < vm->globals_cap; i++) {
        if (vm->globals[i].key) {
            release(vm->globals[i].val);
            obj_decref((Obj *)vm->globals[i].key);
        }
    }
    if (vm->globals) vm_free(vm->globals);
    vm_free(vm->stack);
    vm_free(vm);
}

VMResult vm_interpret(VM *vm, ObjFunction *script) {
    vm->has_error = false;
    vm->script = script;
    if ((long)script->frame_size > STACK_MAX) {
        snprintf(vm->error, sizeof(vm->error),
                 "value stack overflow (script needs %u slots)", script->frame_size);
        vm->has_error = true;
        return VM_RUNTIME_ERROR;
    }
    /* Frame 0: slot 0 is the script function itself. */
    push(vm, retain(mk_obj((Obj *)script)));
    CallFrame *f = &vm->frames[vm->frame_count++];
    f->fn = script;
    f->ip = script->chunk->code;
    f->slots = vm->sp - 1;
    f->method_class = NULL;
    VMResult r = run(vm, 0);
    if (r != VM_OK) return r;
    /* Normal top-level RETURN already unwound frame 0 and its slot. */
    return VM_OK;
}

/* Bridge for vm_source.c (see vm_internal.h): report a compile-time error. */
void vm_set_compile_error(VM *vm, const char *msg) {
    snprintf(vm->error, sizeof(vm->error), "%s", msg);
    vm->has_error = true;
}

bool vm_get_global(VM *vm, const char *name, Value *out) {
    ObjString *k = string_from_cstr(name);
    GEntry *e = globals_find(vm, k);
    release(mk_obj((Obj *)k));
    if (!e) return false;
    if (out) *out = e->val;
    return true;
}

bool vm_invoke_value(VM *vm, Value recv, uint32_t sym, int argc, Value *args, Value *result) {
    /* Resolve a user-defined method (instance chain / class static / module fn). */
    Value mv;
    ObjFunction *fn = NULL;
    ObjClass *mclass = NULL;
    if (IS_INSTANCE(recv)) {
        ObjClass *owner;
        if (class_find_method(AS_INSTANCE(recv)->klass, sym, &mv, &owner)) { fn = AS_FUNCTION(mv); mclass = owner; }
    } else if (IS_CLASS(recv)) {
        if (dict_get(AS_CLASS(recv)->statics, mk_symbol(sym), &mv) && IS_FUNCTION(mv)) fn = AS_FUNCTION(mv);
    } else if (IS_MODULE(recv)) {
        if (dict_get(AS_MODULE(recv)->members, mk_symbol(sym), &mv) && IS_FUNCTION(mv)) fn = AS_FUNCTION(mv);
    }
    if (fn) {
        /* Push receiver + args and run the method to completion synchronously. */
        push(vm, retain(recv));
        for (int i = 0; i < argc; i++) push(vm, retain(args[i]));
        if (!call_sync(vm, fn, argc, mclass)) return false;
        *result = pop(vm); /* call_sync left the (owned) result on the stack */
        return true;
    }
    /* Fall back to native built-in methods (incl. Object base methods). */
    return vm_invoke(vm, recv, sym, argc, args, result);
}

const char *vm_last_error(const VM *vm) { return vm->error; }
void vm_set_trace(VM *vm, bool on) { vm->trace = on; }
