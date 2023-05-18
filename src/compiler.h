#ifndef CLAM_COMPILER_H
#define CLAM_COMPILER_H

#include <stdint.h>

#include "common.h"
#include "parser.h"
#include "vm.h"

// Output a chunk of bytecode given the ast arena and the index of the start
// expression
VM_Chunk compile(String source, ASTVec arena, size_t index);

#endif
