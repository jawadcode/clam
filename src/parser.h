#ifndef LLAMBDA_PARSER_H
#define LLAMBDA_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "result.h"
#include "vec.h"

// Stores parser state
typedef struct {
    const char *source;
    Lexer lexer;
    ASTVec ast_arena;
} Parser;

// Creates a new parser that operates on 'source'
Parser new_parser(const char *source);

typedef struct {
    ASTIndex index;
    Span span;
} SpannedIndex;

CreateResult(SpannedIndex, SyntaxError, Parse);

// Parse the source as an expression, pushing the AST nodes to 'self.ast_arena'
ParseResult parse_expr(Parser *self);

// Format the AST as an S-Expression
String format_ast(Parser *self);

#endif
