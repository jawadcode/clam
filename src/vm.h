#ifndef CLAM_VM_H
#define CLAM_VM_H

#include <stdint.h>

#include "vec.h"

typedef enum OpCode : uint16_t {
    VM_OP_LOAD_CONST = 1,
    VM_OP_POP = 2,
    VM_OP_PRINT = 3,

    /* BINARY OPERATIONS (values match up with AST_BinOp) */

    VM_OP_ADD = 26,
    VM_OP_SUB = 27,
    VM_OP_MUL = 28,
    VM_OP_DIV = 29,
    VM_OP_MOD = 30,

    VM_OP_AND = 32,
    VM_OP_OR = 33,

    VM_OP_LT = 34,
    VM_OP_LEQ = 35,
    VM_OP_GT = 36,
    VM_OP_GEQ = 37,
    VM_OP_EQ = 38,
    VM_OP_NEQ = 39,
} OpCode;

typedef enum ValueType : uint8_t {
    VALUE_TYPE_BOOL = 0,
    VALUE_TYPE_INT = 1
} ValueType;

typedef struct Value {
    ValueType tag;
    union ValueUnion {
        bool boolean;
        int32_t integer;
    } value;
} Value;

DECL_VEC_HEADER(Value, Values)

#endif
