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

uint16_t find_or_push_value(ValueVec *values, VM_Value value) {
    for (size_t i = 0; i < values->length; i++) {
        VM_Value current = values->buffer[i];
        if (VM_Value_eq(current, value))
            return i;
    }
    size_t index = ValueVec_push(values, value);
    if (index > UINT16_MAX) {
        fputs("Reached size limit on constants table (UINT16_MAX)", stderr);
        exit(1);
    } else
        return (uint16_t)index;
}

uint16_t find_or_push_literal(ValueVec *values, AST_Literal literal) {
    VM_Value value = lit_to_value(literal);
    return find_or_push_value(values, value);
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
                 (uint16_t)find_or_push_literal(&chunk->constants, literal));
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

static void compile_ast(Compiler *compiler, VM_Chunk *chunk, size_t index);

// Another beautiful demonstration of the power of stack machines, we can
// compile lists by compiling each sub-expression and appending it to what
// starts out as an empty list, forming our result
static void compile_list(Compiler *compiler, VM_Chunk *chunk, AST_List list) {
    uint16_t empty = find_or_push_value(
        &chunk->constants,
        (VM_Value){.tag = VM_VALUE_LIST, .value = {.list = ValueVec_new()}});
    CodeVec_push(&chunk->code, VM_OP_CONST);
    CodeVec_push(&chunk->code, empty);
    for (size_t i = 0; i < list.length; i++) {
        ASTIndex item = list.buffer[i];
        compile_ast(compiler, chunk, item);
        CodeVec_push(&chunk->code, (uint16_t)VM_OP_APPEND);
    }
}

static void compile_let_in(Compiler *compiler, VM_Chunk *chunk,
                           AST_LetIn let_in) {
    compiler->scope_depth++;
    LocalVec_push(&compiler->locals, (Local){/* Zhu-Li, do the thing */});
    compiler->scope_depth--;
}

static void compile_ast(Compiler *compiler, VM_Chunk *chunk, size_t index) {
    AST ast = compiler->arena.buffer[index];
    switch (ast.tag) {
    case AST_LITERAL:
        return compile_literal(chunk, ast.value.literal);
    case AST_IDENT:
        return compile_ident(compiler, chunk, ast.value.ident);
    case AST_LIST:
        return compile_list(compiler, chunk, ast.value.list);
    case AST_LET_IN:
        return;
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
    compile_ast(&compiler, &chunk, index);
    return chunk;
}
