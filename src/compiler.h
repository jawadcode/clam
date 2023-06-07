#ifndef CLAM_COMPILER_H
#define CLAM_COMPILER_H

#include <stdint.h>

#include "common.h"
#include "parser.h"
#include "vm.h"

typedef struct CompileError_NotFound {
    Span span;
    String not_found;
} CompileError_NotFound;

typedef struct CompileError {
    enum CompileErrorTag {
        COMPILE_ERROR_NOT_FOUND,
        COMPILE_ERROR_MAX_CONSTS,
        COMPILE_ERROR_MAX_LOCALS,
    } tag;
    union CompileErrorUnion {
        CompileError_NotFound not_found;
        Span max_consts;
        Span max_locals;
    } err;
} CompileError;

DEF_RESULT(VM_Chunk, CompileError, Compile);

// Output a chunk of bytecode given the ast arena and the index of the start
// expression
CompileResult compile(String source, ASTVec arena, size_t index);

#endif
