#include "compiler.h"
#include "vm.h"
#include <string.h>

DEF_VEC(uint16_t, CodeVec)
DEF_VEC(VM_Value, ValueVec)

static VM_Value lit_to_value(AST_Literal literal) {
    switch (literal.tag) {
    case LITERAL_UNIT:
        return (VM_Value){.tag = VM_VALUE_UNIT};
    case LITERAL_BOOL:
        return (VM_Value){.tag = VM_VALUE_BOOL,
                          .value = {.boolean = literal.value.boolean}};
    case LITERAL_NUMBER:
        return (VM_Value){.tag = VM_VALUE_NUMBER,
                          .value = {.number = literal.value.number}};
    case LITERAL_STRING:
        return (VM_Value){
            .tag = VM_VALUE_STRING,
            .value = {.string = BUF_TO_STR(literal.value.string)}};
    }
}

uint16_t find_or_push(ValueVec *values, AST_Literal literal) {
    for (size_t i = 0; i < values->length; i++) {
        VM_Value current = values->buffer[i];
        if ((size_t)current.tag == (size_t)literal.tag) {
            switch (literal.tag) {
            case LITERAL_UNIT:
                return i;
            case LITERAL_BOOL:
                if (current.value.boolean == literal.value.boolean)
                    return i;
                else
                    break;
            case LITERAL_NUMBER:
                if (current.value.number == literal.value.number)
                    return i;
                else
                    break;
            case LITERAL_STRING: {
                String curr_str = current.value.string;
                StringBuf lit_str = literal.value.string;
                if (curr_str.length == lit_str.length &&
                    memcmp(curr_str.buffer, lit_str.buffer, curr_str.length) ==
                        0)
                    return i;
                else
                    break;
            }
            }
        }
    }
    size_t index = ValueVec_push(values, lit_to_value(literal));
    if (index > UINT16_MAX) {
        fputs("Reached limit on size of constants table", stderr);
        exit(1);
    } else
        return (uint16_t)index;
}

typedef struct Compiler {
    size_t local_count;
    size_t scope_depth;
} Compiler;

static VM_Chunk compile_ast(Compiler *compiler, VM_Chunk *chunk, ASTVec arena,
                            size_t index) {
    AST ast = arena.buffer[index];
    switch (ast.tag) {
    case AST_LITERAL: {
        AST_Literal lit = ast.value.literal;
        CodeVec_push(&chunk->code, (uint16_t)VM_OP_CONST);
        CodeVec_push(&chunk->code,
                     (uint16_t)find_or_push(&chunk->constants, lit));
        break;
    }
    case AST_IDENT: {
        break;
    }
    case AST_LIST:
        break;
    case AST_LET_IN: {
        compiler->scope_depth++;
        compiler->local_count++;
        break;
    }
    case AST_ABSTRACTION:
        break;
    case AST_APPLICATION:
        break;
    case AST_PRINT:
        break;
    case AST_IF_ELSE:
        break;
    case AST_UNARY_OP:
        break;
    case AST_BINARY_OP:
        break;
    case AST_LIST_INDEX:
        break;
    }
}

VM_Chunk compile(ASTVec arena, size_t index) {
    Compiler compiler = {
        .local_count = 0,
        .scope_depth = 0,
    };
    VM_Chunk chunk = {
        .constants = ValueVec_new(),
        .code = CodeVec_new(),
    };
    compile_ast(&compiler, &chunk, arena, index);
    return chunk;
}
