#include "ast.h"
#include "common.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

DEF_VEC(AST, ASTVec)

static String binop_to_string(AST_BinOp op) {
    switch (op) {
    case BINOP_FNPIPE:
        return STR("|>");
    case BINOP_ADD:
        return STR("+");
    case BINOP_SUB:
        return STR("-");
    case BINOP_MUL:
        return STR("*");
    case BINOP_DIV:
        return STR("/");
    case BINOP_MOD:
        return STR("%");
    case BINOP_AND:
        return STR("and");
    case BINOP_OR:
        return STR("or");
    case BINOP_LT:
        return STR("<");
    case BINOP_LEQ:
        return STR("<=");
    case BINOP_GT:
        return STR(">");
    case BINOP_GEQ:
        return STR(">=");
    case BINOP_EQ:
        return STR("==");
    case BINOP_NEQ:
        return STR("!=");
    }
}

static String unop_to_string(AST_UnOp op) {
    switch (op) {
    case AST_UNOP_NOT:
        return STR("!");
    case AST_UNOP_NEGATE:
        return STR("-");
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
            char num[20];
            // I hate that I have to use a printf-style function but needs must
            snprintf(num, 20, "%f", literal->value.number);
            StringBuf_push_string(buf,
                                  (String){.buffer = num,
                                           // 20 isn't necessarily the length of
                                           // 'num', it may well use less
                                           .length = strlen(num)});
            break;
        }
        case LITERAL_STRING: {
            StringBuf str = literal->value.string;
            StringBuf_push(buf, '"');
            StringBuf_push_string(buf, (String){str.buffer, str.length});
            StringBuf_push(buf, '"');
            break;
        }
        }
        break;
    }
    case AST_IDENT:
        StringBuf_push_string(buf, node->value.ident);
        break;
    case AST_LIST: {
        AST_List *list = &node->value.list;
        StringBuf_push_string(buf, STR("(list"));
        for (size_t i = 0; i < list->length; i++) {
            StringBuf_push(buf, ' ');
            ASTIndex item = list->buffer[i];
            format_ast_node(arena, item, buf);
        }
        StringBuf_push(buf, ')');
        break;
    }
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
    case AST_UNARY_OP: {
        AST_UnaryOp *unop = &node->value.unary_op;
        StringBuf_push(buf, '(');
        StringBuf_push_string(buf, unop_to_string(unop->op));
        StringBuf_push(buf, ' ');
        format_ast_node(arena, unop->operand, buf);
        StringBuf_push(buf, ')');
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
        // I hate writing switch statements
        break;
    }
    case AST_APPLICATION: {
        AST_Application *app = &node->value.application;
        StringBuf_push_string(buf, STR("(app "));
        format_ast_node(arena, app->function, buf);
        StringBuf_push(buf, ' ');
        format_ast_node(arena, app->argument, buf);
        StringBuf_push(buf, ')');
        break;
    }
    case AST_IF_ELSE: {
        AST_IfElse *if_else = &node->value.if_else;
        StringBuf_push_string(buf, STR("(if "));
        format_ast_node(arena, if_else->condition, buf);
        StringBuf_push_string(buf, STR(" :then "));
        format_ast_node(arena, if_else->then, buf);
        StringBuf_push_string(buf, STR(" :else "));
        format_ast_node(arena, if_else->else_, buf);
        StringBuf_push(buf, ')');
        break;
    }
    case AST_LIST_INDEX: {
        AST_ListIndex *list_index = &node->value.list_index;
        StringBuf_push_string(buf, STR("(get "));
        format_ast_node(arena, list_index->list, buf);
        StringBuf_push(buf, ' ');
        format_ast_node(arena, list_index->index, buf);
        StringBuf_push(buf, ')');
        break;
    }
    }
}

StringBuf format_ast(ASTVec *arena, size_t index) {
    StringBuf buf = StringBuf_new();
    format_ast_node(arena, index, &buf);
    return buf;
}
