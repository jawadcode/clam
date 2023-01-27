#ifndef LLAMBDA_AST_H
#define LLAMBDA_AST_H

#include "common.h"
#include "lexer.h"
#include "result.h"
#include "vec.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

// A type alias which represents the index into the arena of nodes
typedef size_t ASTIndex;

// A literal, e.g: 'unit', 'true', '0.123' and '"a string"'
typedef struct {
    enum {
        LITERAL_UNIT,
        LITERAL_BOOL,
        LITERAL_NUMBER,
        LITERAL_STRING,
    } tag;
    union {
        void *unit;
        bool boolean;
        double number;
        char *string;
    } value;
} AST_Literal;

// A singular let binding (variable definition) in 'AST_LetIn'
typedef struct {
    String ident;
    ASTIndex value;
} AST_LetBind;

// clang-format off
DECL_VEC_HEADER(AST_LetBind, AST_LetBindVec)

// An expression which is equivalent to 'body', given the variables defined
// in 'bindings'
typedef struct {
    AST_LetBindVec bindings;
    ASTIndex body;
    // clang-format on
} AST_LetIn;

// An anonymous function with an argument 'argument' and a body 'body'
typedef struct {
    String argument;
    ASTIndex body;
} AST_Abstraction;

// An expression where 'function' is called with the argument 'argument'
typedef struct {
    ASTIndex function;
    ASTIndex argument;
} AST_Application;

// An expression that when evaluated, prints 'expr', this is built-in because
// I'm lazy
typedef struct {
    ASTIndex expr;
} AST_Print;

// An if-else expression, operating on the condition 'condition', with a
// consequence 'then' and an alternative 'else_'
typedef struct {
    ASTIndex condition;
    ASTIndex then;
    ASTIndex else_;
} AST_IfElse;

// A binary operation, with a value matching that of the corresponding
// enumeration in 'TokenKind' so we can safely cast from it
typedef enum {
    AST_UNOP_NOT = 24,
    AST_UNOP_NEGATE = 20,
} AST_UnOp;

// A unary operation `op` on the node referenced by `operand`
typedef struct {
    AST_UnOp op;
    ASTIndex operand;
} AST_UnaryOp;

// A binary operation, with a value matching that of the corresponding
// enumeration in 'TokenKind' so we can safely cast from it
typedef enum {
    BINOP_FNPIPE = 18,

    BINOP_ADD = 19,
    BINOP_SUB = 20,
    BINOP_MUL = 21,
    BINOP_DIV = 22,
    BINOP_MOD = 23,

    BINOP_AND = 25,
    BINOP_OR = 26,

    BINOP_LT = 27,
    BINOP_LEQ = 28,
    BINOP_GT = 29,
    BINOP_GEQ = 30,
    BINOP_EQ = 31,
    BINOP_NEQ = 32,
} AST_BinOp;

// A binary operation 'op' on the nodes referenced by 'lhs' and 'rhs'
typedef struct {
    AST_BinOp op;
    ASTIndex lhs;
    ASTIndex rhs;
} AST_BinaryOp;

// The *A*bstract *S*yntax *T*ree
typedef struct {
    enum {
        AST_LITERAL,
        AST_IDENT,
        AST_LET_IN,
        AST_ABSTRACTION,
        AST_APPLICATION,
        AST_PRINT,
        AST_IF_ELSE,
        AST_UNARY_OP,
        AST_BINARY_OP,
    } tag;
    union {
        AST_Literal literal;
        String ident;
        AST_LetIn let_in;
        AST_Abstraction abstraction;
        AST_Application application;
        AST_Print print;
        AST_IfElse if_else;
        AST_UnaryOp unary_op;
        AST_BinaryOp binary_op;
    } value;
    Span span;
} AST;

typedef struct {
    Span string;
    Span escape_sequence;
} SyntaxError_InvalidEscSeq;

typedef struct {
    // 'expected.length' should exclude the null terminator (which should be
    // present)
    String expected;
    Token got;
} SyntaxError_UnexpectedToken;

typedef struct {
    enum {
        ERROR_INVALID_ESC_SEQ,
        ERROR_UNEXPECTED_TOKEN,
    } tag;
    union {
        SyntaxError_InvalidEscSeq invalid_esc_seq;
        SyntaxError_UnexpectedToken unexpected_token;
    } error;
} SyntaxError;

// clang-format off
DECL_VEC_HEADER(AST, ASTVec)

String binop_to_string(AST_BinOp op);

#endif
