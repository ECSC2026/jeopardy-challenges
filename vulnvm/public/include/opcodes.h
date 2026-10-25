/* opcodes.h — VulnVM bytecode instruction set (our own stack-machine ISA).
 *
 * Operand encodings (big-endian):
 *   u16 const  : 2-byte index into the chunk constant pool
 *   u16 slot   : 2-byte local/upvalue slot index
 *   u16 count  : 2-byte element count
 *   u16 jump   : 2-byte unsigned jump distance
 *   u8  argc   : 1-byte argument count
 * See the operand table in disasm.c / compiler.c for each opcode's operands.
 */
#ifndef VULNVM_OPCODES_H
#define VULNVM_OPCODES_H

/* X-macro list: X(name). Keep in sync with disasm operand handling. */
#define OPCODE_LIST(X)                                                    \
    X(CONSTANT)        /* u16 const : push constants[idx]              */ \
    X(NULL)            /* push null                                    */ \
    X(TRUE)            /* push true                                    */ \
    X(FALSE)           /* push false                                   */ \
    X(POP)             /* discard top                                  */ \
    X(DUP)             /* duplicate top                                */ \
    X(DUP2)            /* duplicate top two (a b -> a b a b)           */ \
    X(GET_LOCAL)       /* u16 slot                                     */ \
    X(SET_LOCAL)       /* u16 slot : assign top to slot (keeps top)    */ \
    X(GET_GLOBAL)      /* u16 const(name) : push global                */ \
    X(SET_GLOBAL)      /* u16 const(name) : assign (keeps top)         */ \
    X(DEFINE_GLOBAL)   /* u16 const(name) : define global from top     */ \
    X(GET_UPVALUE)     /* u16 slot                                     */ \
    X(SET_UPVALUE)     /* u16 slot                                     */ \
    X(GET_INDEX)       /* a[i]  (obj idx -> value)                     */ \
    X(SET_INDEX)       /* a[i]=v (obj idx value -> value)              */ \
    X(GET_PROPERTY)    /* u16 const(sym) : obj -> value                */ \
    X(SET_PROPERTY)    /* u16 const(sym) : obj value -> value          */ \
    X(ADD) X(SUB) X(MUL) X(DIV) X(MOD) X(NEG)                             \
    X(BAND) X(BOR) X(BXOR) X(BNOT) X(SHL) X(SHR)                          \
    X(EQ) X(NEQ) X(LT) X(GT) X(LE) X(GE) X(NOT)                           \
    X(BUILD_ARRAY)     /* u16 count                                    */ \
    X(BUILD_BYTEARRAY) /* u16 count                                    */ \
    X(BUILD_DICT)      /* u16 count (of key/value pairs)               */ \
    X(JUMP)            /* u16 jump : ip += dist                        */ \
    X(JUMP_IF_FALSE)   /* u16 jump : pop; if falsey ip += dist         */ \
    X(JUMP_IF_TRUE)    /* u16 jump : pop; if truthy ip += dist         */ \
    X(LOOP)            /* u16 jump : ip -= dist                        */ \
    X(CALL)            /* u8 argc                                      */ \
    X(INVOKE)          /* u16 const(sym), u8 argc : method call        */ \
    X(INVOKE_SUPER)    /* u16 const(sym), u8 argc                      */ \
    X(NEW)             /* u8 argc : construct (cls below args)         */ \
    X(NEW_ARRAY)       /* new [size] : size -> array                   */ \
    X(RETURN)          /* return top                                   */ \
    X(CLOSURE)         /* u16 const(function)                          */ \
    X(CLASS)           /* u16 const(name string) : push new class      */ \
    X(INHERIT)         /* class super -> class (set superclass)        */ \
    X(METHOD)          /* u16 const(sym) : class fn -> class (add mthd)*/ \
    X(MEMBER)          /* u16 const(sym) : ns value -> ns (add static) */ \
    X(MODULE)          /* u16 const(name string) : push new module     */ \
    X(INSTANCEOF)      /* expr type -> bool                            */ \
    X(HAS)             /* u16 const(sym) : obj -> bool                 */ \
    X(THROW)           /* throw top                                    */ \
    X(TRY_PUSH)        /* u16 jump : push handler at ip+dist           */ \
    X(TRY_POP)         /* pop handler                                  */

typedef enum {
#define X(name) OP_##name,
    OPCODE_LIST(X)
#undef X
    OP__COUNT
} OpCode;

const char *opcode_name(OpCode op);

#endif /* VULNVM_OPCODES_H */
