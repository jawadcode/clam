#ifndef CLAM_LEXER_H
#define CLAM_LEXER_H

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
    TK_IDENT = 12,   // Identifier
    TK_ASSIGN = 13,  // "="
    TK_ARROW = 14,   // "=>"
    TK_LPAREN = 15,  // "("
    TK_RPAREN = 16,  // ")"
    TK_LSQUARE = 17, // "["
    TK_RSQUARE = 18, // "]"
    TK_COMMA = 19,   // ","
    TK_FNPIPE = 20,  // "|>"

    /* ARITHMETIC OPS */
    // TK_SUB (unary)
    TK_ADD = 21, // "+"
    TK_SUB = 22, // "-"
    TK_MUL = 23, // "*"
    TK_DIV = 24, // "/"
    TK_MOD = 25, // "%"

    /* BOOLEAN OPS */
    TK_NOT = 26, // "not"
    TK_AND = 27, // "and"
    TK_OR = 28,  // "or"

    /* COMPARISON OPS */
    TK_LT = 29,  // "<"
    TK_LEQ = 30, // "<="
    TK_GT = 31,  // ">"
    TK_GEQ = 32, // ">="
    TK_EQ = 33,  // "=="
    TK_NEQ = 34, // "!="

    /* SPECIAL */
    TK_INVALID = 35, // Invalid token
    TK_EOF = 36,     // End Of File
} TokenKind;

// A singular token
typedef struct {
    TokenKind kind;
    Span span;
} Token;

// Uninitialised if 'tag == MAYBE_NONE' and intialised if 'tag == MAYBE_SOME'
typedef struct {
    enum {
        MAYBE_SOME,
        MAYBE_NONE,
    } tag;
    Token some;
} MaybeToken;

// Stores lexer state
typedef struct {
    String source;
    size_t start;
    size_t current;
    MaybeToken peeked;
} Lexer;

DECL_VEC_HEADER(Token, TokenVec)
// clang-format off

// Create a new lexer that operates on `source`
Lexer Lexer_new(const String source);
// clang-format on

// Get the current peeked token
Token *Lexer_peek_token(Lexer *lexer);

// Scan the next token from `lexer->source`
Token Lexer_next_token(Lexer *lexer);

// Convert a token kind to a string
String TK_to_string(TokenKind kind);

// Get the token as a slice of the source string
String Token_to_string(Lexer lexer, Token token);

// Print a token using printf
void Token_print(const String source, Token token);

#endif
