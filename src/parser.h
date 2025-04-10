#ifndef CLAM_PARSER_H
#define CLAM_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "result.h"
#include <stdio.h>

// Stores parser state
typedef struct {
    const String file_name;
    const String source;
    Lexer lexer;
    ASTVec ast_arena;
} Parser;

// Creates a new parser that operates on 'source'
Parser Parser_new(const String file_name, const String source);

typedef struct SyntaxError_InvalidEscSeq {
    Span string;
    Span escape_sequence;
} SyntaxError_InvalidEscSeq;

typedef struct SyntaxError_UnexpectedToken {
    // 'expected.length' should exclude the null terminator (which should be
    // present)
    String expected;
    // TODO: Remove `span`, or change `got` to `TokenKind`
    Token got;
    Span span;
} SyntaxError_UnexpectedToken;

typedef struct {
    enum SyntaxErrorTag {
        ERROR_INVALID_ESC_SEQ,
        ERROR_UNEXPECTED_TOKEN,
    } tag;
    union SyntaxErrorUnion {
        SyntaxError_InvalidEscSeq invalid_esc_seq;
        SyntaxError_UnexpectedToken unexpected_token;
    } error;
} SyntaxError;

// Generate an error diagnostic message from a `SyntaxError`
void Parser_print_diag(Parser *self, SyntaxError error, FILE *stream);

DEF_RESULT(ASTIndex, SyntaxError, Parse);

// Parse the source as an expression, pushing the AST nodes to 'self.ast_arena'
// and returning the index of the parent expression
ParseResult Parser_parse_expr(Parser *self);

// Free all the allocations within each AST node and then free the AST arena
// itself
void Parser_free(Parser *self);

#endif
