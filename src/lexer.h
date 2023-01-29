#ifndef LLAMBDA_LEXER_H
#define LLAMBDA_LEXER_H

#include "common.h"
#include "vec.h"
#include <stdint.h>
#include <stdlib.h>

// An enumeration of the different kinds of tokens
typedef enum {
    /* KEYWORDS */
    TK_LET = 0,   // "let"
    TK_IN = 1,    // "in"
    TK_FUN = 2,   // "fun"
    TK_IF = 3,    // "if"
    TK_THEN = 4,  // "then"
    TK_ELSE = 5,  // "else"
    TK_PRINT = 6, // "print"

    /* LITERALS */
    TK_TRUE = 7,    // "true"
    TK_FALSE = 8,   // "false"
    TK_UNIT = 9,    // "unit"
    TK_NUMBER = 10, // Numeric literal, e.g: "0.123" or "1234"
    TK_STRING = 11, // String literal, e.g: "this is a string"

    /* MISC */
    TK_IDENT = 12,  // Identifier
    TK_ASSIGN = 13, // "="
    TK_ARROW = 14,  // "=>"
    TK_LPAREN = 15, // "("
    TK_RPAREN = 16, // ")"
    TK_COMMA = 17,  // ","
    TK_FNPIPE = 18, // "|>"

    /* ARITHMETIC OPS */
    // TK_SUB (unary)
    TK_ADD = 19, // "+"
    TK_SUB = 20, // "-"
    TK_MUL = 21, // "*"
    TK_DIV = 22, // "/"
    TK_MOD = 23, // "%"

    /* BOOLEAN OPS */
    TK_NOT = 24, // "not"
    TK_AND = 25, // "and"
    TK_OR = 26,  // "or"

    /* COMPARISON OPS */
    TK_LT = 27,  // "<"
    TK_LEQ = 28, // "<="
    TK_GT = 29,  // ">"
    TK_GEQ = 30, // ">="
    TK_EQ = 31,  // "=="
    TK_NEQ = 32, // "!="

    /* SPECIAL */
    TK_INVALID = 33, // Invalid token
    TK_EOF = 34,     // End Of File
} TokenKind;

// A singular token
typedef struct {
    TokenKind kind;
    Span span;
} Token;

typedef struct {
    enum {
        MAYBE_SOME,
        MAYBE_NONE,
    } tag;
    union {
        Token some;
        void *none;
    } value;
} MaybeToken;

// Stores lexer state
typedef struct {
    const char *source;
    const char *start;
    const char *current;
    MaybeToken peeked;
} Lexer;

DECL_VEC_HEADER(Token, TokenVec)
// clang-format off

// Create a new lexer that operates on `source`
Lexer new_lexer(const char *source);
// clang-format on

// Get the current peeked token
Token *peek_token(Lexer *lexer);

// Scan the next token from `lexer->source`
Token next_token(Lexer *lexer);

// Convert a token kind to a string
const char *token_kind_to_string(TokenKind kind);

// Print a token using printf
void print_token(const char *source, Token token);

#endif
