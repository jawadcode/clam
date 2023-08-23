#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "compiler.h"
#include "lexer.h"
#include "result.h"
#include "vec.h"
#include "vm.h"

DEF_VEC(uint16_t, CodeVec)
DEF_VEC(VM_Value, ValueVec)
VEC_WITH_CAP(VM_Value, ValueVec)

static VM_Value lit_to_value(AST_Literal literal) {
    switch (literal.tag) {
    case LITERAL_UNIT:
        return (VM_Value){.tag = VM_VALUE_UNIT};
    case LITERAL_BOOL:
        return (VM_Value){.tag = VM_VALUE_BOOL,
                          .value = {.boolean = literal.value.boolean}};
    case LITERAL_INT:
        return (VM_Value){.tag = VM_VALUE_INT,
                          .value = {.integer = literal.value.integer}};
    case LITERAL_FLOAT:
        return (VM_Value){.tag = VM_VALUE_FLOAT,
                          .value = {.floate = literal.value.floate}};
    case LITERAL_STRING:
        return (VM_Value){
            .tag = VM_VALUE_STRING,
            .value = {.string = BUF_TO_STR(literal.value.string)}};
    }
}

DEF_RESULT(uint16_t, CompileError, Index);

IndexResult find_or_push_value(ValueVec *values, VM_Value value, Span span) {
    for (size_t i = 0; i < values->length; i++) {
        VM_Value current = values->buffer[i];
        if (VM_Value_eq(current, value))
            return (IndexResult){.tag = RESULT_OK,
                                 .value = {.ok = (uint16_t)i}};
    }
    size_t index = ValueVec_push(values, value);
    if (index > UINT16_MAX) {
        return (IndexResult){.tag = RESULT_ERR,
                             .value = {.err = {.tag = COMPILE_ERROR_MAX_CONSTS,
                                               .err = {.max_consts = span}}}};
    } else
        return (IndexResult){.tag = RESULT_OK,
                             .value = {.ok = (uint16_t)index}};
}

IndexResult find_or_push_literal(ValueVec *values, AST_Literal literal,
                                 Span span) {
    VM_Value value = lit_to_value(literal);
    return find_or_push_value(values, value, span);
}

typedef struct Local {
    Span span;
    String source;
    uint16_t depth;
} Local;

typedef enum FunType {
    FUN_TYPE_TOPLEVEL,
    FUN_TYPE_CLOSURE,
} FunType;

DEF_VEC_T(Local, LocalVec)

typedef struct Compiler {
    String source;
    ASTVec arena;
    LocalVec locals;
    uint16_t offset;
    uint16_t scope_depth;
} Compiler;

DEF_RESULT(void *, CompileError, VoidCompile);

static VoidCompileResult compile_literal(VM_Chunk *chunk, AST_Literal literal,
                                         Span span) {
    CompileError error;
    CodeVec_push(&chunk->code, (uint16_t)VM_OP_CONST);
    uint16_t index;
    RET_ERR_ASSIGN(index, IndexResult,
                   find_or_push_literal(&chunk->constants, literal, span));
    CodeVec_push(&chunk->code, index);
    return (VoidCompileResult){.tag = RESULT_OK, .value = {.ok = NULL}};
FAILURE:
    return (VoidCompileResult){.tag = RESULT_ERR, .value = {.err = error}};
}

static VoidCompileResult compile_ident(Compiler *compiler, VM_Chunk *chunk,
                                       String ident, Span span) {
    // The length of 'Compiler::locals' is guaranteed to not exceed 'UINT16_MAX'
    for (uint16_t i = 0; i < compiler->locals.length; i++) {
        Local local = compiler->locals.buffer[i];
        if (String_eq(local.source, ident)) {
            CodeVec_push(&chunk->code, (uint16_t)VM_OP_GET);
            CodeVec_push(&chunk->code, i);
            return (VoidCompileResult){.tag = RESULT_OK, .value = {.ok = NULL}};
        }
    }

    return (VoidCompileResult){
        .tag = RESULT_ERR,
        .value = {
            .err = {.tag = COMPILE_ERROR_NOT_FOUND,
                    .err = {.not_found = {.span = span, .not_found = ident}}}}};
}

static VoidCompileResult compile_ast(Compiler *compiler, VM_Chunk *chunk,
                                     size_t index);

// Another beautiful demonstration of the power of stack machines, we can
// compile list initialisers by compiling each sub-expression and appending it
// to what starts out as an empty list, forming our result
static VoidCompileResult compile_list(Compiler *compiler, VM_Chunk *chunk,
                                      AST_List list, Span span) {
    CompileError error;
    uint16_t empty;
    RET_ERR_ASSIGN(
        empty, IndexResult,
        find_or_push_value(
            &chunk->constants,
            (VM_Value){.tag = VM_VALUE_LIST, .value = {.list = ValueVec_new()}},
            span));
    CodeVec_push(&chunk->code, VM_OP_CONST);
    CodeVec_push(&chunk->code, empty);
    for (size_t i = 0; i < list.length; i++) {
        ASTIndex item = list.buffer[i];
        RET_ERR(VoidCompileResult, compile_ast(compiler, chunk, item));
        CodeVec_push(&chunk->code, (uint16_t)VM_OP_APPEND);
    }
    return (VoidCompileResult){.tag = RESULT_OK, .value = {.ok = NULL}};
FAILURE:
    return (VoidCompileResult){.tag = RESULT_ERR, .value = {.err = error}};
}

static VoidCompileResult compile_let_in(Compiler *compiler, VM_Chunk *chunk,
                                        AST_LetIn let_in) {
    compiler->scope_depth++;
    for (size_t i = 0; i < let_in.bindings.length; i++) {
        AST_LetBind bind = let_in.bindings.buffer[i];
        if (compiler->locals.length + 1 > UINT16_MAX) {
            return (VoidCompileResult){
                .tag = RESULT_ERR,
                .value = {.err = {.tag = COMPILE_ERROR_MAX_LOCALS,
                                  .err = {.max_locals = bind.span}}}};
        }
        LocalVec_push(&compiler->locals, (Local){
                                             .span = bind.span,
                                             .source = bind.ident,
                                             .depth = compiler->scope_depth,
                                         });
        compile_ast(compiler, chunk, bind.value);
    }
    compile_ast(compiler, chunk, let_in.body);
    compiler->scope_depth--;
    return (VoidCompileResult){.tag = RESULT_OK, .value = {.ok = NULL}};
}

static VoidCompileResult compile_ast(Compiler *compiler, VM_Chunk *chunk,
                                     size_t index) {
    AST ast = compiler->arena.buffer[index];
    switch (ast.tag) {
    case AST_LITERAL:
        return compile_literal(chunk, ast.value.literal, ast.span);
    case AST_IDENT:
        return compile_ident(compiler, chunk, ast.value.ident, ast.span);
    case AST_LIST:
        return compile_list(compiler, chunk, ast.value.list, ast.span);
    case AST_LET_IN:
        return compile_let_in(compiler, chunk, ast.value.let_in);
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
    return (VoidCompileResult){.tag = RESULT_OK, .value = {.ok = NULL}};
}

CompileResult compile(String source, ASTVec arena, size_t index) {
    CompileError error;
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
    RET_ERR(VoidCompileResult, compile_ast(&compiler, &chunk, index));
    LocalVec_free(&compiler.locals);
    return (CompileResult){.tag = RESULT_OK, .value = {.ok = chunk}};
FAILURE:
    LocalVec_free(&compiler.locals);
    return (CompileResult){.tag = RESULT_ERR, .value = {.err = error}};
}
