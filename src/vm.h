#ifndef CLAM_VM_H
#define CLAM_VM_H

#include <stdint.h>

#include "common.h"
#include "vec.h"

typedef enum VM_Op {
    /* MISC */
    // Load a constant from the chunk's constant table
    VM_OP_CONST,
    // Bind a variable
    VM_OP_BIND,
    // Get a variable
    VM_OP_GET,
    // Peek at the top of the stack and print the value to stdout
    VM_OP_PRINT,

    /* UNARY OPS */
    // Check the top of the stack is a boolean, and then NOT it in place
    VM_OP_NOT = 31,
    // Check the top of the stack is a number, and then negate it in place
    VM_OP_NEGATE = 42,

    /* BINARY OPS */
    // Pops RHS from the stack, then modifies the LHS (at the top of the stack)
    // in place by applying the operation with the RHS to it
    VM_OP_APPEND = 24,
    VM_OP_CONCAT = 25,
    VM_OP_ADD = 26,
    VM_OP_SUB = 27,
    VM_OP_MUL = 28,
    VM_OP_DIV = 29,
    VM_OP_MOD = 30, // uses fmod
    VM_OP_AND = 32, // not short-circuiting
    VM_OP_OR = 33,  // not short-circuiting
    VM_OP_LT = 34,
    VM_OP_LEQ = 35,
    VM_OP_GT = 36,
    VM_OP_GEQ = 37,
    VM_OP_EQ = 38,
    VM_OP_NEQ = 39,

    /* BRANCHING */
    // Pop the condition off of the stack and if it is true then jump
    VM_OP_JUMP_IF,
    // Jump regardless
    VM_OP_JUMP,
    // Jump to the start of a function, storing the return address in a register
    VM_OP_CALL,
    // Jump back to the return address
    VM_OP_RET,
} VM_Op;

typedef struct VM_Chunk VM_Chunk;

// A function
typedef struct VM_Function {
    VM_Chunk *chunk;
} VM_Function;

// Values as stored on the stack
typedef struct VM_Value {
    enum VM_ValueTag {
        VM_VALUE_UNIT = 0,
        VM_VALUE_BOOL = 1,
        VM_VALUE_INT = 2,
        VM_VALUE_FLOAT = 3,
        VM_VALUE_STRING = 4,
        VM_VALUE_LIST = 5,
        VM_FUNCTION = 6,
    } tag;
    union VM_ValueUnion {
        bool boolean;
        int32_t integer;
        double floate;
        String string;
        struct ValueVec *list;
        VM_Function fun;
    } value;
} VM_Value;

bool VM_Value_eq(VM_Value a, VM_Value b);

// We do a little bit of inconsistent naming
inline bool is_obj(VM_Value value) { return value.tag >= 4; }

DECL_VEC_HEADER(uint16_t, CodeVec)
DECL_VEC_HEADER(size_t, LineVec)

DECL_VEC_HEADER(VM_Value, ValueVec)
VEC_WITH_CAP_SIG(VM_Value, ValueVec)

// Passed to the interpreter and executed
typedef struct VM_Chunk {
    // All the constants referenced in the bytecode
    ValueVec constants;
    // The actual bytecode
    CodeVec code;
    // RLE compressed line information for 'code'
    LineVec lines;
} VM_Chunk;

#endif
