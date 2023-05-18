#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "lexer.h"
#include "vec.h"
#include "vm.h"

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
        fputs("Reached size limit on constants table (UINT16_MAX)", stderr);
        exit(1);
    } else
        return (uint16_t)index;
}

typedef struct Local {
    Token token;
    uint16_t depth;
} Local;

DEF_VEC_T(Local, LocalVec)

typedef struct Compiler {
    String source;
    ASTVec arena;
    LocalVec locals;
    uint16_t scope_depth;
} Compiler;

static void compile_literal(VM_Chunk *chunk, AST_Literal literal) {
    CodeVec_push(&chunk->code, (uint16_t)VM_OP_CONST);
    CodeVec_push(&chunk->code,
                 (uint16_t)find_or_push(&chunk->constants, literal));
}

static void compile_ident(Compiler *compiler, VM_Chunk *chunk, String ident) {
    // The length of 'Compiler::locals' is guaranteed to not exceed 'UINT16_MAX'
    for (uint16_t i = 0; i < compiler->locals.length; i++) {
        Local local = compiler->locals.buffer[i];
        if (String_eq(Token_to_string(compiler->source, local.token), ident)) {
            CodeVec_push(&chunk->code, (uint16_t)VM_OP_GET);
            CodeVec_push(&chunk->code, i);
            return;
        }
    }

    fputs("Variable not found: ", stderr);
    String_write(ident, stderr);
    fputc('\n', stderr);
}

// Another beautiful demonstration of the power of stack machines, we can
// compile lists by compiling each sub-expression and appending it to what
// starts out as an empty list, forming our result
static void compile_list(Compiler *compiler, VM_Chunk *chunk, AST_List list) {}

static void compile_ast(Compiler *compiler, VM_Chunk *chunk, ASTVec arena,
                        size_t index) {
    AST ast = arena.buffer[index];
    switch (ast.tag) {
    case AST_LITERAL:
        return compile_literal(chunk, ast.value.literal);
    case AST_IDENT:
        return compile_ident(compiler, chunk, ast.value.ident);
    case AST_LIST:
        return compile_list(compiler, chunk, ast.value.list);
    case AST_LET_IN:
        break;
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

VM_Chunk compile(String source, ASTVec arena, size_t index) {
    Compiler compiler = {
        .source = source,
        .arena = arena,
        .locals = LocalVec_new(),
        .scope_depth = 0,
    };
    VM_Chunk chunk = {
        .constants = ValueVec_new(),
        .code = CodeVec_new(),
    };
    compile_ast(&compiler, &chunk, arena, index);
    return chunk;
}
