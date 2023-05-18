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
    VM_OP_NOT = 30,
    // Check the top of the stack is a number, and then negate it in place
    VM_OP_NEGATE = 41,

    /* BINARY OPS */
    // Pops RHS from the stack, then modifies the LHS (at the top of the stack)
    // in place by applying the operation with the RHS to it
    VM_OP_ADD = 25,
    VM_OP_SUB = 26,
    VM_OP_MUL = 27,
    VM_OP_DIV = 28,
    VM_OP_MOD = 29, // uses fmod
    VM_OP_AND = 31, // not short-circuiting
    VM_OP_OR = 32,  // not short-circuiting
    VM_OP_LT = 33,
    VM_OP_LEQ = 34,
    VM_OP_GT = 35,
    VM_OP_GEQ = 36,
    VM_OP_EQ = 37,
    VM_OP_NEQ = 38,

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

// Points to an address in the chunk
typedef uint16_t VM_Function;

typedef struct VM_Value VM_Value;

DECL_VEC_HEADER(struct VM_Value, ValueVec)

// Values as stored on the stack
typedef struct VM_Value {
    enum VM_ValueTag {
        VM_VALUE_UNIT = 0,
        VM_VALUE_BOOL = 1,
        VM_VALUE_NUMBER = 2,
        VM_VALUE_STRING = 3,
        VM_VALUE_LIST = 4,
        VM_FUNCTION = 5,
    } tag;
    union VM_ValueUnion {
        bool boolean;
        double number;
        String string;
        ValueVec list;
        VM_Function fun;
    } value;
} VM_Value;

DECL_VEC_HEADER(uint16_t, CodeVec)

// Passed to the interpreter and executed
typedef struct VM_Chunk {
    // All the constants referenced in the bytecode
    ValueVec constants;
    // The actual bytecode
    CodeVec code;
} VM_Chunk;

#endif
