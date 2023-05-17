#ifndef CLAM_AST_H
#define CLAM_AST_H

#include "common.h"
#include "lexer.h"
#include "result.h"
#include "vec.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

// A type alias which represents the index into the arena of nodes
typedef size_t ASTIndex;

DECL_VEC_HEADER(ASTIndex, AST_List)

// A literal, e.g: 'unit', 'true', '0.123' and '"a string"'
typedef struct AST_Literal {
    // The values should match up with VM_ValueTag
    enum AST_LiteralTag {
        LITERAL_UNIT = 0,
        LITERAL_BOOL = 1,
        LITERAL_NUMBER = 2,
        LITERAL_STRING = 3,
    } tag;
    union AST_LiteralUnion {
        bool boolean;
        double number;
        StringBuf string;
    } value;
} AST_Literal;

// A singular let binding (variable definition) in 'AST_LetIn'
typedef struct AST_LetBind {
    String ident;
    ASTIndex value;
} AST_LetBind;

// clang-format off
DECL_VEC_HEADER(AST_LetBind, AST_LetBindVec)

// An expression which is equivalent to 'body', given the variables defined
// in 'bindings'
typedef struct AST_LetIn {
    AST_LetBindVec bindings;
    ASTIndex body;
    // clang-format on
} AST_LetIn;

// An anonymous function with an argument 'argument' and a body 'body'
typedef struct AST_Abstraction {
    String argument;
    ASTIndex body;
} AST_Abstraction;

// An expression where 'function' is called with the argument 'argument'
typedef struct AST_Application {
    ASTIndex function;
    ASTIndex argument;
} AST_Application;

// An expression that when evaluated, prints 'expr' and returns the value that
// was passed to it, akin to the 'dump' function from the 'batteries' OCaml
// library
typedef struct AST_Print {
    ASTIndex expr;
} AST_Print;

// An if-else expression, operating on the condition 'condition', with a
// consequence 'then' and an alternative 'else_'
typedef struct AST_IfElse {
    ASTIndex condition;
    ASTIndex then;
    ASTIndex else_;
} AST_IfElse;

// A binary operation, with a value matching that of the corresponding
// enumeration in 'VM_Op' so we can safely cast to it (doesn't match with
// 'TokenKind' because 'TK_SUB' is used by 'BINOP_SUB')
typedef enum AST_UnOp {
    AST_UNOP_NOT = 28,
    AST_UNOP_NEGATE = 40,
} AST_UnOp;

// A unary operation `op` on the node referenced by `operand`
typedef struct AST_UnaryOp {
    Span op_span;
    AST_UnOp op;
    ASTIndex operand;
} AST_UnaryOp;

// A binary operation, with a value matching that of the corresponding
// enumeration in 'TokenKind' so we can safely cast from it
typedef enum AST_BinOp {
    BINOP_FNPIPE = 22,

    BINOP_ADD = 23,
    BINOP_SUB = 24,
    BINOP_MUL = 25,
    BINOP_DIV = 26,
    BINOP_MOD = 27,

    BINOP_AND = 29,
    BINOP_OR = 30,

    BINOP_LT = 31,
    BINOP_LEQ = 32,
    BINOP_GT = 33,
    BINOP_GEQ = 34,
    BINOP_EQ = 35,
    BINOP_NEQ = 36,
} AST_BinOp;

// A binary operation 'op' on the nodes referenced by 'lhs' and 'rhs'
typedef struct AST_BinaryOp {
    Span op_span;
    AST_BinOp op;
    ASTIndex lhs;
    ASTIndex rhs;
} AST_BinaryOp;

// A list index expression
typedef struct AST_ListIndex {
    ASTIndex list;
    ASTIndex index;
} AST_ListIndex;

// The Abstract Syntax Tree
typedef struct AST {
    enum ASTTag {
        AST_LITERAL,
        AST_IDENT,
        AST_LIST,
        AST_LET_IN,
        AST_ABSTRACTION,
        AST_APPLICATION,
        AST_PRINT,
        AST_IF_ELSE,
        AST_UNARY_OP,
        AST_BINARY_OP,
        AST_LIST_INDEX,
    } tag;
    union ASTUnion {
        AST_Literal literal;
        String ident;
        AST_List list;
        AST_LetIn let_in;
        AST_Abstraction abstraction;
        AST_Application application;
        AST_Print print;
        AST_IfElse if_else;
        AST_UnaryOp unary_op;
        AST_BinaryOp binary_op;
        AST_ListIndex list_index;
    } value;
    Span span;
} AST;

// clang-format off
DECL_VEC_HEADER(AST, ASTVec)

StringBuf format_ast(ASTVec *arena, size_t index);

#endif
