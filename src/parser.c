#include "parser.h"
#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "memory.h"
#include "result.h"
#include "vec.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

DEF_VEC_T(String, StringVec)

DEF_VEC(AST_LetBind, AST_LetBindVec)
DEF_VEC(ASTIndex, AST_List)

DEF_RESULT(String, SyntaxError, ParseString);
DEF_RESULT(Token, SyntaxError, Token);
DEF_RESULT(AST, SyntaxError, AST);

#define TK_EXPR_LEN 12
static const TokenKind TK_EXPR[TK_EXPR_LEN] = {
    TK_UNIT,   TK_TRUE,    TK_FALSE, TK_NUMBER, TK_STRING, TK_IDENT,
    TK_LPAREN, TK_LSQUARE, TK_LET,   TK_FUN,    TK_SUB,    TK_NOT};

#define TK_BINOPS_LEN 15
static const TokenKind TK_BINOPS[TK_BINOPS_LEN] = {
    TK_FNPIPE, TK_ADD, TK_SUB, TK_MUL, TK_DIV, TK_MOD, TK_NOT, TK_AND,
    TK_OR,     TK_LT,  TK_LEQ, TK_GT,  TK_GEQ, TK_EQ,  TK_NEQ};

#define TK_EXPR_TERMINATORS_LEN 5
static const TokenKind TK_EXPR_TERMINATORS[TK_EXPR_TERMINATORS_LEN] = {
    TK_RPAREN, TK_RSQUARE, TK_COMMA, TK_IN, TK_EOF};

#define BINOPS_AND_EXPR_TERMINATORS_LEN 26
static const TokenKind
    BINOPS_AND_EXPR_TERMINATORS[BINOPS_AND_EXPR_TERMINATORS_LEN] = {
        TK_FNPIPE, TK_ADD,     TK_SUB,     TK_MUL, TK_DIV, TK_MOD, TK_NOT,
        TK_AND,    TK_OR,      TK_LT,      TK_LEQ, TK_GT,  TK_GEQ, TK_EQ,
        TK_NEQ,    TK_LPAREN,  TK_LSQUARE, TK_LET, TK_FUN, TK_SUB, TK_NOT,
        TK_RPAREN, TK_RSQUARE, TK_COMMA,   TK_IN,  TK_EOF};

Parser new_parser(const char *source) {
    // clang-format on
    return (Parser){
        .source = source,
        .lexer = new_lexer(source),
        .ast_arena = ASTVec_new(),
    };
}

static inline Token *peek(Parser *self) { return peek_token(&self->lexer); }

static inline Token next(Parser *self) { return next_token(&self->lexer); }

static bool in(TokenKind tk, const TokenKind list[], size_t len) {
    bool contains = false;
    for (size_t i = 0; i < len; i++) {
        if (list[i] == tk)
            contains = true;
    }
    return contains;
}

static double parse_number(Parser *self, Span span) {
    const char *num = self->lexer.source + span.start;

    double value = 0.0;
    while (*num >= '0' && *num <= '9') {
        value = value * 10.0 + (*num++ - '0');
    }

    if (*num == '.')
        num++;

    double power = 1.0;
    while (*num >= '0' && *num <= '9') {
        value = value * 10.0 + (*num++ - '0');
        power *= 10.0;
    }

    return value / power;
}

static ParseStringResult parse_string(Parser *self, Span span) {
    const char *str = self->lexer.source + span.start;

    // Malloc enough space for the contents (i.e. minus the start and end
    // quotes, plus the null terminator) of the string in the source, which
    // means we might allocate in excess if has escape codes, but who cares
    char *buffer = malloc(span.end - span.start - 1);
    size_t index = 0;
    bool escaped = false;

    // Skips the opening quote
    while (str[++index] != '"' || (escaped && str[index] == '"')) {
        if (escaped) {
            switch (str[index]) {
            case 'n':
                buffer[index] = '\n';
                break;
            case 'r':
                buffer[index] = '\r';
                break;
            case 't':
                buffer[index] = '\t';
                break;
            case '0':
                buffer[index] = '\0';
                break;
            case '"':
            case '\\':
                buffer[index] = str[index];
                break;
            default: {
                size_t start = str + index - self->lexer.source - 1;
                size_t end = start + 2;
                return (ParseStringResult){
                    .tag = RESULT_ERR,
                    .value =
                        {
                            .err =
                                (SyntaxError){
                                    ERROR_INVALID_ESC_SEQ,
                                    {
                                        .invalid_esc_seq =
                                            {
                                                .string = span,
                                                .escape_sequence = {start, end},
                                            },
                                    },
                                },
                        },
                };
            }
            }
        } else if (str[index] == '\\') {
            escaped = true;
        } else {
            buffer[index] = str[index];
        }
    }
    buffer = (char *)realloc(buffer, sizeof(char) * (index + 1));
    buffer[index] = '\0';

    return (ParseStringResult){
        .tag = RESULT_OK,
        .value = {.ok = {.buffer = buffer, .length = strlen(buffer)}},
    };
}

static ASTResult parse_literal(Parser *self) {
    Token current = next(self);
    AST_Literal lit;

    switch (current.kind) {
    case TK_UNIT:
        lit = (AST_Literal){LITERAL_UNIT, {.unit = NULL}};
        break;
    case TK_TRUE:
        lit = (AST_Literal){LITERAL_BOOL, {.boolean = true}};
        break;
    case TK_FALSE:
        lit = (AST_Literal){LITERAL_BOOL, {.boolean = false}};
        break;
    case TK_NUMBER:
        lit = (AST_Literal){LITERAL_NUMBER,
                            {.number = parse_number(self, current.span)}};
        break;
    case TK_STRING: {
        String string;
        RET_ERR_ASSIGN(string, parse_string(self, current.span),
                       ParseStringResult, ASTResult);

        lit = (AST_Literal){
            .tag = LITERAL_STRING,
            .value = {.string = string},
        };
        break;
    }
        // Any other value should be unreachable
        UNREACHABLE;
    }

    return (ASTResult){
        .tag = RESULT_OK,
        .value =
            {
                .ok =
                    (AST){
                        .tag = AST_LITERAL,
                        .value = {.literal = lit},
                        .span = current.span,
                    },
            },
    };
}

static AST parse_ident(Parser *self) {
    Token current = next(self);

    return (AST){
        .tag = AST_IDENT,
        .value =
            {
                .ident =
                    {
                        .buffer = self->lexer.source + current.span.start,
                        .length = current.span.end - current.span.start,
                    },
            },
        .span = current.span};
}

static TokenResult expect(Parser *self, TokenKind kind) {
    Token token = next(self);
    String kind_string = token_kind_to_string(kind);

    if (token.kind != kind) {
        return (TokenResult){
            .tag = RESULT_ERR,
            .value = {.err = {.tag = ERROR_UNEXPECTED_TOKEN,
                              .error = {.unexpected_token = {.expected =
                                                                 kind_string,
                                                             .got = token}}}},
        };
    } else {
        return (TokenResult){
            .tag = RESULT_OK,
            .value = {.ok = token},
        };
    }
}

static bool at(Parser *self, TokenKind kind) {
    return peek(self)->kind == kind;
}

static bool at_any(Parser *self, const TokenKind *list, size_t length) {
    Token *current = peek(self);

    for (size_t i = 0; i < length; i++)
        if (current->kind == list[i])
            return true;

    return false;
}

// clang-format off
static ASTResult parse_abstraction(Parser *self) {
    // clang-format on
    Token fun_token = next(self);
    StringVec args = StringVec_new();

    // Mandatory first argument
    Token first_arg;
    RET_ERR_ASSIGN(first_arg, expect(self, TK_IDENT), TokenResult, ASTResult);
    String first_arg_string = (String){
        .buffer = self->lexer.source + first_arg.span.start,
        .length = first_arg.span.end - first_arg.span.start,
    };
    StringVec_push(&args, first_arg_string);

    while (at(self, TK_IDENT)) {
        Token arg = next(self);
        String arg_string = (String){
            .buffer = self->lexer.source + arg.span.start,
            .length = arg.span.end - arg.span.start,
        };
        StringVec_push(&args, arg_string);
    }

    RET_ERR(TokenResult, expect(self, TK_ARROW), ASTResult);

    ASTIndex body;
    RET_ERR_ASSIGN(body, parse_expr(self), ParseResult, ASTResult);

    AST abs = (AST){.tag = AST_ABSTRACTION,
                    .value =
                        {
                            .abstraction =
                                (AST_Abstraction){
                                    .argument = args.buffer[args.length - 1],
                                    .body = body,
                                },
                        },
                    .span = {
                        .start = fun_token.span.start,
                        .end = self->ast_arena.buffer[body].span.end,
                    }};
    ASTIndex abs_index = ASTVec_push(&self->ast_arena, abs);
    // We do a little bit of currying
    if (args.length > 1) {
        for (size_t i = args.length - 2; i <= 0; i--) {
            abs = (AST){
                .tag = AST_ABSTRACTION,
                .value =
                    {
                        .abstraction =
                            (AST_Abstraction){
                                .argument = args.buffer[i],
                                .body = abs_index,
                            },
                    },
                .span = {fun_token.span.start,
                         self->ast_arena.buffer[body].span.end},
            };
            abs_index = ASTVec_push(&self->ast_arena, abs);
        }
    }

    return (ASTResult){
        .tag = RESULT_OK,
        .value = {.ok = abs},
    };
}

static ASTResult parse_let_binding(Parser *self) {
    Span span = (Span){.start = next(self).span.start};
    AST_LetBindVec bindings = AST_LetBindVec_new();
    while (at(self, TK_IDENT)) {
        Span ident_span = next(self).span;
        String ident = {.buffer = self->source + ident_span.start,
                        .length = ident_span.end - ident_span.start};
        RET_ERR(TokenResult, expect(self, TK_ASSIGN), ASTResult);
        ASTIndex value;
        RET_ERR_ASSIGN(value, parse_expr(self), ParseResult, ASTResult);
        AST_LetBindVec_push(&bindings, (AST_LetBind){ident, value});
        Token *peeked = peek(self);
        if (peeked->kind == TK_COMMA) {
            next(self);
        }
    }
    RET_ERR(TokenResult, expect(self, TK_IN), ASTResult);
    ASTIndex body;
    RET_ERR_ASSIGN(body, parse_expr(self), ParseResult, ASTResult);
    span.end = self->ast_arena.buffer[body].span.end;
    AST let_in = (AST){
        .tag = AST_LET_IN,
        .value = {.let_in = (AST_LetIn){bindings, body}},
        .span = span,
    };
    return (ASTResult){.tag = RESULT_OK, .value = {.ok = let_in}};
}

static ASTResult parse_unop(Parser *self) {
    Token op_token = next(self);
    Span op_span = op_token.span;
    // The tokenkind is checked before calling so we can safely cast
    AST_UnOp op = (AST_UnOp)op_token.kind;
    ASTIndex operand;
    RET_ERR_ASSIGN(operand, parse_expr(self), ParseResult, ASTResult);
    AST unop =
        (AST){.tag = AST_UNARY_OP,
              .value = {.unary_op = (AST_UnaryOp){op_span, op, operand}}};
    return (ASTResult){.tag = RESULT_OK, .value = {.ok = unop}};
}

static ASTResult parse_list(Parser *self) {
    Span list_span = (Span){.start = next(self).span.start};
    AST_List items = AST_List_new();
    while (!at_any(self, (TokenKind[]){TK_COMMA, TK_RPAREN}, 2)) {
        ASTIndex item;
        RET_ERR_ASSIGN(item, parse_expr(self), ParseResult, ASTResult);
        AST_List_push(&items, item);
        TokenKind peeked = peek(self)->kind;
        if (peeked == TK_COMMA) {
            next(self);
        } else if (peeked == TK_RPAREN) {
            break;
        }
    }
    list_span.end = next(self).span.end;
    return (ASTResult){
        .tag = RESULT_OK,
        .value = {.ok = (AST){.tag = AST_LIST, .value = {.list = items}}}};
}

static ParseResult parse_list_index(Parser *self, ASTIndex list) {
    // skip [
    next(self);
    ASTIndex index;
    RET_ERR_ASSIGN(index, parse_expr(self), ParseResult, ParseResult);
    Token rsquare;
    RET_ERR_ASSIGN(rsquare, expect(self, TK_RSQUARE), TokenResult, ParseResult);
    AST expr = {.tag = AST_LIST,
                .value = {.list_index = {list, index}},
                .span = {.start = self->ast_arena.buffer[list].span.start,
                         .end = rsquare.span.end}};
    return (ParseResult){.tag = RESULT_OK,
                         .value = {.ok = ASTVec_push(&self->ast_arena, expr)}};
}

static ParseResult parse_operand(Parser *self) {
    // TODO: Copy the associated code from llamac_parser lmao
    ASTIndex lhs;
    switch (peek(self)->kind) {
    case TK_UNIT:
    case TK_TRUE:
    case TK_FALSE:
    case TK_NUMBER:
    case TK_STRING: {
        AST lhs_ast;
        RET_ERR_ASSIGN(lhs_ast, parse_literal(self), ASTResult, ParseResult);
        lhs = ASTVec_push(&self->ast_arena, lhs_ast);
        break;
    }
    case TK_IDENT: {
        AST lhs_ast = parse_ident(self);
        lhs = ASTVec_push(&self->ast_arena, lhs_ast);
        break;
    }
    case TK_LPAREN:
        // Skip '('
        next(self);
        RET_ERR_ASSIGN(lhs, parse_expr(self), ParseResult, ParseResult);
        RET_ERR(TokenResult, expect(self, TK_RPAREN), ParseResult);
        break;
    case TK_LSQUARE: {
        AST lhs_ast;
        RET_ERR_ASSIGN(lhs_ast, parse_list(self), ASTResult, ParseResult);
        lhs = ASTVec_push(&self->ast_arena, lhs_ast);
        break;
    }
    case TK_LET: {
        AST lhs_ast;
        RET_ERR_ASSIGN(lhs_ast, parse_let_binding(self), ASTResult,
                       ParseResult);
        lhs = ASTVec_push(&self->ast_arena, lhs_ast);
        break;
    }
    case TK_FUN: {
        AST lhs_ast;
        RET_ERR_ASSIGN(lhs_ast, parse_abstraction(self), ASTResult,
                       ParseResult);
        lhs = ASTVec_push(&self->ast_arena, lhs_ast);
        break;
    }
    case TK_SUB:
    case TK_NOT: {
        AST lhs_ast;
        RET_ERR_ASSIGN(lhs_ast, parse_unop(self), ASTResult, ParseResult);
        lhs = ASTVec_push(&self->ast_arena, lhs_ast);
        break;
    }
    default: {
        return (ParseResult){
            .tag = RESULT_ERR,
            .value = {
                .err = (SyntaxError){
                    .tag = ERROR_UNEXPECTED_TOKEN,
                    .error = {.unexpected_token = (SyntaxError_UnexpectedToken){
                                  .expected = STR("expression"),
                                  .got = next(self),
                              }}}}};
    }
    }
    while (true) {
        if (at_any(self, TK_EXPR, TK_EXPR_LEN)) {
            ASTIndex argument;
            RET_ERR_ASSIGN(argument, parse_expr(self), ParseResult,
                           ParseResult);

            AST lhs_ast =
                (AST){.tag = AST_APPLICATION,
                      .value =
                          {
                              .application =
                                  (AST_Application){
                                      .function = lhs,
                                      .argument = argument,
                                  },
                          },
                      .span = {
                          .start = self->ast_arena.buffer[lhs].span.start,
                          .end = self->ast_arena.buffer[argument].span.end,
                      }};
            lhs = ASTVec_push(&self->ast_arena, lhs_ast);
        }
        if (at(self, TK_LSQUARE)) {
            RET_ERR_ASSIGN(lhs, parse_list_index(self, lhs), ParseResult,
                           ParseResult);
        }
        if (at_any(self, BINOPS_AND_EXPR_TERMINATORS,
                   BINOPS_AND_EXPR_TERMINATORS_LEN)) {
            break;
        }
        if (peek(self)->kind == TK_LSQUARE) {
            continue;
        } else {
            return (ParseResult){
                .tag = RESULT_ERR,
                .value = {.err = {.tag = ERROR_UNEXPECTED_TOKEN,
                                  .error = {.unexpected_token = {
                                                .expected = STR("expression,"),
                                                .got = next(self)}}}}};
        }
    }
    return (ParseResult){.tag = RESULT_OK,
                         .value = {
                             .ok = lhs,
                         }};
}

ParseResult parse_expr(Parser *self) {
    ASTIndex lhs;
    RET_ERR_ASSIGN(lhs, parse_operand(self), ParseResult, ParseResult);

    /*for (;;) {
        Token *peeked = peek(self);
        TokenKind op;
        switch (peeked->kind) {
        case TK_FNPIPE:
        case TK_ADD:
        case TK_SUB:
        case TK_MUL:
        case TK_DIV:
        case TK_MOD:
        case TK_AND:
        case TK_OR:
        case TK_LT:
        case TK_LEQ:
        case TK_GT:
        case TK_GEQ:
        case TK_EQ:
        case TK_NEQ:
            op = peeked->kind;
            break;
        case TK_RPAREN:
        case TK_IN:
        case TK_COMMA:
        case TK_EOF:
            // Cope seethe mald
            goto DONE;
        default:
            return (ParseResult){
                .tag = RESULT_ERR,
                .value =
                    {
                        .err =
                            {
                                .tag = ERROR_UNEXPECTED_TOKEN,
                                .error =
                                    {
                                        .unexpected_token =
                                            {
                                                .expected =
                                                    STR("operator, ')', "
                                                        "'in', or ','"),
                                                .got = next(self),
                                            },
                                    },
                            },
                    },
            };
        }

        next(self);

        ASTIndex rhs;
        RET_ERR_ASSIGN(rhs, parse_expr(self), ParseResult, ParseResult);

        Span span = {self->ast_arena.buffer[lhs].span.start,
                     self->ast_arena.buffer[rhs].span.end};
        AST lhs_ast = (AST){
            .tag = AST_BINARY_OP,
            .value =
                {
                    .binary_op =
                        {
                            .op = (AST_BinOp)op,
                            .lhs = lhs,
                            .rhs = rhs,
                        },
                },
            .span = span,
        };
        lhs = ASTVec_push(&self->ast_arena, lhs_ast);
    }
DONE:

    return (ParseResult){
        .tag = RESULT_OK,
        .value = {.ok = lhs},
    }; */
    while (true) {
        Token *peeked = peek(self);
        Token op;
        if (in(peeked->kind, TK_BINOPS, TK_BINOPS_LEN)) {
            op = next(self);
        } else if (in(peeked->kind, TK_EXPR_TERMINATORS,
                      TK_EXPR_TERMINATORS_LEN)) {
            break;
        } else {
            return (ParseResult){
                .tag = RESULT_ERR,
                .value = {
                    .err = {
                        .tag = ERROR_UNEXPECTED_TOKEN,
                        .error = {.unexpected_token = {
                                      .expected = STR(
                                          "operator or expression terminator"),
                                      .got = next(self)}}}}};
        }

        ASTIndex rhs;
        RET_ERR_ASSIGN(rhs, parse_operand(self), ParseResult, ParseResult);
        AST lhs_ast = (AST){
            .tag = AST_BINARY_OP,
            .value = {
                .binary_op = (AST_BinaryOp){
                    .op_span = op.span, .op = (AST_BinOp)op.kind, lhs, rhs}}};
        lhs = ASTVec_push(&self->ast_arena, lhs_ast);
    }

    return (ParseResult){
        .tag = RESULT_OK,
        .value = {.ok = lhs},
    };
}
