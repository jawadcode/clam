#include "compiler.h"

MaybeNameError resolve_names(ASTVec arena, ASTIndex root) {
    AST node = arena.buffer[root];
    switch (node.tag) {
    case AST_LITERAL:
    case AST_IDENT:
    case AST_LIST:
    case AST_LET_IN:
    case AST_ABSTRACTION:
    case AST_APPLICATION:
    case AST_PRINT:
    case AST_IF_ELSE:
    case AST_UNARY_OP:
    case AST_BINARY_OP:
        break;
    }
}
