#include "ast.h"
#include "common.h"

String binop_to_string(AST_BinOp op) {
    switch (op) {
    case BINOP_FNPIPE:
        return (String){"|>", 2};
    case BINOP_ADD:
        return (String){"+", 1};
    case BINOP_SUB:
        return (String){"-", 1};
    case BINOP_MUL:
        return (String){"*", 1};
    case BINOP_DIV:
        return (String){"/", 1};
    case BINOP_MOD:
        return (String){"%", 1};
    case BINOP_AND:
        return (String){"and", 3};
    case BINOP_OR:
        return (String){"or", 2};
    case BINOP_LT:
        return (String){"<", 1};
    case BINOP_LEQ:
        return (String){"<=", 2};
    case BINOP_GT:
        return (String){">", 1};
    case BINOP_GEQ:
        return (String){">=", 2};
    case BINOP_EQ:
        return (String){"==", 2};
    case BINOP_NEQ:
        return (String){"!=", 2};
    }
}

DEF_VEC(AST, ASTVec)
