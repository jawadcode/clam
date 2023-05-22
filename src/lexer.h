#ifndef CLAM_LEXER_H
#define CLAM_LEXER_H

#include <stdint.h>
#include <stdlib.h>

#include "common.h"
#include "vec.h"

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
    TK_INT = 10,    // Integer literal, e.g. "1234"
    TK_FLOAT = 11,  // Float literal e.g. "123.4"
    TK_STRING = 12, // String literal, e.g: "this is a string"

    /* MISC */
    TK_IDENT = 13,   // Identifier
    TK_ASSIGN = 14,  // "="
    TK_ARROW = 15,   // "=>"
    TK_LPAREN = 16,  // "("
    TK_RPAREN = 17,  // ")"
    TK_LSQUARE = 18, // "["
    TK_RSQUARE = 19, // "]"
    TK_LCURLY = 20,  // "{"
    TK_RCURLY = 21,  // "}"
    TK_COMMA = 22,   // ","
    TK_FNPIPE = 23,  // "|>"
    TK_APPEND = 24,  // "::"
    TK_CONCAT = 25,  // "++"

    /* ARITHMETIC OPS */
    // TK_SUB (unary)
    TK_ADD = 26, // "+"
    TK_SUB = 27, // "-"
    TK_MUL = 28, // "*"
    TK_DIV = 29, // "/"
    TK_MOD = 30, // "%"

    /* BOOLEAN OPS */
    TK_NOT = 31, // "not"
    TK_AND = 32, // "and"
    TK_OR = 33,  // "or"

    /* COMPARISON OPS */
    TK_LT = 34,  // "<"
    TK_LEQ = 35, // "<="
    TK_GT = 36,  // ">"
    TK_GEQ = 37, // ">="
    TK_EQ = 38,  // "=="
    TK_NEQ = 39, // "!="

    /* SPECIAL */
    TK_INVALID = 40, // Invalid token
    TK_EOF = 41,     // End Of File
} TokenKind;

// A singular token
typedef struct {
    TokenKind kind;
    Span span;
} Token;

// Uninitialised if 'tag == MAYBE_NONE' and initialised if 'tag == MAYBE_SOME'
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
String Token_to_string(String string, Token token);

// Print a token using printf
void Token_print(const String source, Token token);

#endif
