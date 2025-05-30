#ifndef CLAM_COMPILER_H
#define CLAM_COMPILER_H

#include <stdint.h>

#include "ast.h"
#include "maybe.h"
#include "vec.h"
#include "vm.h"

DECL_VEC_HEADER(uint16_t, Code)

typedef struct Chunk {
    Values constants;
    Code code;
} Chunk;

typedef struct NameError {
    Span location;
    ValueType got;
    ValueType expected;
} NameError;

CREATE_MAYBE(NameError, MaybeNameError);

MaybeNameError resolve_names(ASTVec arena, ASTIndex root);

#endif
