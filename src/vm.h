#ifndef CLAM_VM_H
#define CLAM_VM_H

#include "src/common.h"
#include "src/vec.h"
#include <stdint.h>

typedef enum VM_Op {
    // Load a constant from the chunk's constant table
    VM_OP_CONST,
    // Bind a variable
    VM_OP_BIND,
    // Get a variable
    VM_OP_GET,
    // Peek at the top of the stack and print the value to stdout
    VM_OP_PRINT,
    // Pop the condition off of the stack and if it is true then jump
    VM_OP_JUMP_IF,
    // Jump regardless
    VM_OP_JUMP,
    // Jump to the start of a function, storing the return address in a register
    VM_OP_CALL,
    // Jump back to the return address
    VM_OP_RET,
} VM_Op;

typedef struct VM_String {
    enum VM_StringType {
        VM_STRING_INTERNED,
        VM_STRING_BUF,
    } tag;
    union VM_StringUnion {
        // An index into the constants table
        size_t str_index;
        // A heap allocated string
        StringBuf str_buf;
    } value;
} VM_String;

// Points to an address in the chunk
typedef size_t VM_Function;

// Values as stored on the stack
typedef struct VM_Value {
    enum VM_ValueTag {
        VM_VALUE_UNIT,
        VM_VALUE_BOOL,
        VM_VALUE_NUMBER,
        VM_VALUE_STRING,
        VM_FUNCTION,
    } tag;
    union VM_ValueUnion {
        bool boolean;
        double number;
        VM_String string;
        VM_Function fun;
    } value;
} VM_Value;

DECL_VEC_HEADER(VM_Value, ValueVec)
DECL_VEC_HEADER(uint8_t, CodeVec)

// Passed to the interpreter and executed
typedef struct VM_Chunk {
    // All of the constants referenced in the bytecode
    ValueVec constants;
    // The actual bytecode
    CodeVec code;
} VM_Chunk;

#endif