#include "ast.h"
#include "common.h"
#include <stdio.h>

DEF_VEC(AST, ASTVec)

static String binop_to_string(AST_BinOp op) {
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

static void format_ast_node(ASTVec *arena, size_t index, StringBuf *buf) {
    AST *node = &arena->buffer[index];
    switch (node->tag) {
    case AST_LITERAL: {
        AST_Literal *literal = &node->value.literal;
        switch (literal->tag) {
        case LITERAL_UNIT:
            StringBuf_push_string(buf, STR("()"));
            break;
        case LITERAL_BOOL:
            StringBuf_push_string(buf, literal->value.boolean ? STR("true")
                                                              : STR("false"));
            break;
        case LITERAL_NUMBER: {
            char num[19];
            snprintf(num, 19, "%f", literal->value.number);
            StringBuf_push_string(buf, STR(num));
            break;
        }
        case LITERAL_STRING:
            StringBuf_push(buf, '"');
            StringBuf_push_string(buf, literal->value.string);
            StringBuf_push(buf, '"');
            break;
        }
        break;
    }
    case AST_IDENT:
        StringBuf_push_string(buf, node->value.ident);
        break;
    case AST_LET_IN: {
        AST_LetIn *let_in = &node->value.let_in;
        StringBuf_push_string(buf, STR("(let ["));
        for (size_t i = 0; i < let_in->bindings.length; i++) {
            StringBuf_push(buf, '(');
            AST_LetBind binding = let_in->bindings.buffer[i];
            StringBuf_push_string(buf, binding.ident);
            StringBuf_push(buf, ' ');
            format_ast_node(arena, binding.value, buf);
            StringBuf_push(buf, ')');
        }
        StringBuf_push_string(buf, STR("] "));
        format_ast_node(arena, let_in->body, buf);
        StringBuf_push(buf, ')');
        break;
    }
    case AST_ABSTRACTION: {
        AST_Abstraction *abs = &node->value.abstraction;
        StringBuf_push_string(buf, STR("(fun ["));
        StringBuf_push_string(buf, abs->argument);
        StringBuf_push_string(buf, STR("] "));
        format_ast_node(arena, abs->body, buf);
        StringBuf_push(buf, ')');
        break;
    }
    case AST_APPLICATION:
    case AST_PRINT:
    case AST_IF_ELSE:
    case AST_UNARY_OP: {
        StringBuf_push_string(buf, STR("UNIMPLEMENTED"));
        break;
    }
    case AST_BINARY_OP: {
        AST_BinaryOp *binop = &node->value.binary_op;
        StringBuf_push(buf, '(');
        StringBuf_push_string(buf, binop_to_string(binop->op));
        StringBuf_push(buf, ' ');
        format_ast_node(arena, binop->lhs, buf);
        StringBuf_push(buf, ' ');
        format_ast_node(arena, binop->rhs, buf);
        StringBuf_push(buf, ')');
    }
    }
}

StringBuf format_ast(ASTVec *arena, size_t index) {
    StringBuf buf = StringBuf_new();
    format_ast_node(arena, index, &buf);
    return buf;
}
