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

Parser new_parser(const char *source) {
    // clang-format on
    return (Parser){
        .source = source,
        .lexer = new_lexer(source),
        .ast_arena = ASTVec_new(),
    };
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

DEF_RESULT(char *, SyntaxError, ParseString);

static ParseStringResult parse_string(Parser *self, Span span) {
    const char *str = self->lexer.source + span.start;

    // Malloc enough space for the contents (i.e. minus the start and end
    // quotes, plus the null terminator) of the string in the source, which
    // means we might allocate in excess if has escape codes
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
        .value = {.ok = buffer},
    };
}

DEF_RESULT(AST, SyntaxError, AST);

#ifdef _MSC_VER
#define UNREACHABLE                                                            \
    default:                                                                   \
        __assume(0)
#elif defined(__GNUC__) || defined(__clang__)
#define UNREACHABLE                                                            \
    default:                                                                   \
        __builtin_unreachable()
#else
#define UNREACHABLE                                                            \
    default:                                                                   \
        ((void)0)
#endif

static ASTResult parse_literal(Parser *self) {
    Token current = next_token(&self->lexer);
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
        char *string;
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
    Token current = next_token(&self->lexer);

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

#define TK_EXPR_LENGTH 8
const TokenKind TK_EXPR[TK_EXPR_LENGTH] = {TK_UNIT,   TK_TRUE,   TK_FALSE,
                                           TK_NUMBER, TK_STRING, TK_IDENT,
                                           TK_LPAREN, TK_FUN};

DEF_RESULT(Token, SyntaxError, Token);

static TokenResult expect(Parser *self, TokenKind kind) {
    Token token = next_token(&self->lexer);
    const char *kind_string = token_kind_to_string(kind);

    if (token.kind != kind) {
        return (TokenResult){
            .tag = RESULT_ERR,
            .value = {.err = {.tag = ERROR_UNEXPECTED_TOKEN,
                              .error = {.unexpected_token =
                                            {.expected = {.buffer = kind_string,
                                                          .length = strlen(
                                                              kind_string)}}}}},
        };
    } else {
        return (TokenResult){
            .tag = RESULT_OK,
            .value = {.ok = token},
        };
    }
}

static bool at(Parser *self, TokenKind kind) {
    return peek_token(&self->lexer)->kind == kind;
}

static bool at_any(Parser *self, const TokenKind *list, size_t length) {
    Token *current = peek_token(&self->lexer);

    for (size_t i = 0; i < length; i++)
        if (current->kind == list[i])
            return true;

    return false;
}

// clang-format off
DEF_VEC_T(String, StringVec)

static ASTResult parse_abstraction(Parser *self) {
    // clang-format on
    Token fun_token = next_token(&self->lexer);

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
        Token arg = next_token(&self->lexer);
        String arg_string = (String){
            .buffer = self->lexer.source + arg.span.start,
            .length = arg.span.end - arg.span.start,
        };
        StringVec_push(&args, arg_string);
    }

    RET_ERR(TokenResult, expect(self, TK_ARROW), ASTResult);

    SpannedIndex body;
    RET_ERR_ASSIGN(body, parse_expr(self), ParseResult, ASTResult);

    AST abs = (AST){.tag = AST_ABSTRACTION,
                    .value =
                        {
                            .abstraction =
                                (AST_Abstraction){
                                    .argument = args.buffer[args.length - 1],
                                    .body = body.index,
                                },
                        },
                    .span = {
                        .start = fun_token.span.start,
                        .end = body.span.end,
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
                .span = {fun_token.span.start, body.span.end},
            };
            abs_index = ASTVec_push(&self->ast_arena, abs);
        }
    }

    return (ASTResult){
        .tag = RESULT_OK,
        .value = {.ok = abs},
    };
}

ParseResult parse_expr(Parser *self) {
    Token *current = peek_token(&self->lexer);
    SpannedIndex lhs;

    switch (current->kind) {
    case TK_UNIT:
    case TK_TRUE:
    case TK_FALSE:
    case TK_NUMBER:
    case TK_STRING: {
        AST lhs_ast;
        RET_ERR_ASSIGN(lhs_ast, parse_literal(self), ASTResult, ParseResult);
        lhs = (SpannedIndex){ASTVec_push(&self->ast_arena, lhs_ast),
                             lhs_ast.span};
        break;
    }
    case TK_IDENT: {
        AST lhs_ast = parse_ident(self);
        lhs = (SpannedIndex){ASTVec_push(&self->ast_arena, lhs_ast),
                             lhs_ast.span};
        break;
    }
    case TK_LPAREN:
        // Skip '('
        next_token(&self->lexer);
        RET_ERR_ASSIGN(lhs, parse_expr(self), ParseResult, ParseResult);
        RET_ERR(TokenResult, expect(self, TK_RPAREN), ParseResult);
        break;
    case TK_FUN: {
        AST lhs_ast;
        RET_ERR_ASSIGN(lhs_ast, parse_abstraction(self), ASTResult,
                       ParseResult);
        lhs = (SpannedIndex){ASTVec_push(&self->ast_arena, lhs_ast),
                             lhs_ast.span};
        break;
    }
    default:
        return (ParseResult){
            .tag = RESULT_ERR,
            .value = {
                .err = {.tag = ERROR_UNEXPECTED_TOKEN,
                        .error =
                            {
                                .unexpected_token =
                                    {
                                        .expected =
                                            {
                                                .buffer = "expression",
                                                .length = 10,
                                            },
                                        .got = next_token(&self->lexer),
                                    },

                            }},
            }};
    }

    while (at_any(self, TK_EXPR, TK_EXPR_LENGTH)) {
        SpannedIndex argument;
        RET_ERR_ASSIGN(argument, parse_expr(self), ParseResult, ParseResult);

        AST lhs_ast = (AST){.tag = AST_APPLICATION,
                            .value =
                                {
                                    .application =
                                        (AST_Application){
                                            .function = lhs.index,
                                            .argument = argument.index,
                                        },
                                },
                            .span = {
                                .start = lhs.span.start,
                                .end = argument.span.end,
                            }};
        lhs = (SpannedIndex){ASTVec_push(&self->ast_arena, lhs_ast),
                             lhs_ast.span};
    }

    for (;;) {
        Token *peeked = peek_token(&self->lexer);
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
                                                    {
                                                        .buffer =
                                                            "operator, ')', "
                                                            "'in', or ','",
                                                        .length = 27,
                                                    },
                                                .got = next_token(&self->lexer),
                                            },
                                    },
                            },
                    },
            };
        }

        next_token(&self->lexer);

        SpannedIndex rhs;
        RET_ERR_ASSIGN(rhs, parse_expr(self), ParseResult, ParseResult);

        Span span = {lhs.span.start, rhs.span.end};
        AST lhs_ast = (AST){
            .tag = AST_BINARY_OP,
            .value =
                {
                    .binary_op =
                        {
                            .op = (AST_BinOp)op,
                            .lhs = lhs.index,
                            .rhs = rhs.index,
                        },
                },
            .span = span,
        };
        lhs = (SpannedIndex){ASTVec_push(&self->ast_arena, lhs_ast), span};
    }
DONE:

    return (ParseResult){
        .tag = RESULT_OK,
        .value = {.ok =
                      {
                          .index = lhs.index,
                          .span = lhs.span,
                      }},
    };
}

static void buffer_write(char *buffer, size_t *buffer_length, String source) {
    if (source.length == 0)
        return;
    size_t prev_length = *buffer_length;
    *buffer_length += source.length;
    buffer = (char *)realloc(buffer, sizeof(char) * *buffer_length);
    memcpy(buffer + sizeof(char) * prev_length, source.buffer, source.length);
}

static void format_ast_index(Parser *self, char *buffer, size_t *buffer_length,
                             ASTIndex ast_index, size_t spaces) {
    AST ast = self->ast_arena.buffer[ast_index];
    char *tabs = (char *)malloc(sizeof(char) * spaces);
    if (spaces > 0)
        memset(tabs, ' ', spaces);

    String indent = {.buffer = tabs, .length = spaces};
    String lparen = STR("(");
    String rparen = STR(")");
    String newline = STR("\n");

    switch (ast.tag) {
    case AST_LITERAL:
    case AST_IDENT: {
        String source = {
            .buffer = self->source + ast.span.start,
            .length = ast.span.end - ast.span.start,
        };
        buffer_write(buffer, buffer_length, indent);
        buffer_write(buffer, buffer_length, source);
        break;
    }
    case AST_ABSTRACTION: {
        AST_Abstraction abs = ast.value.abstraction;
        buffer_write(buffer, buffer_length, indent);
        buffer_write(buffer, buffer_length, STR("(fun ["));
        buffer_write(buffer, buffer_length, abs.argument);
        buffer_write(buffer, buffer_length, STR("]\n"));

        format_ast_index(self, buffer, buffer_length, abs.body, spaces + 2);
        buffer_write(buffer, buffer_length, rparen);
        break;
    }
    case AST_APPLICATION: {
        AST_Application app = ast.value.application;
        buffer_write(buffer, buffer_length, indent);
        buffer_write(buffer, buffer_length, lparen);
        format_ast_index(self, buffer, buffer_length, app.function, 0);
        buffer_write(buffer, buffer_length, newline);

        format_ast_index(self, buffer, buffer_length, app.argument, spaces + 2);
        buffer_write(buffer, buffer_length, rparen);
        break;
    }
    case AST_BINARY_OP: {
        AST_BinaryOp binop = ast.value.binary_op;
        String op = binop_to_string(binop.op);
        buffer_write(buffer, buffer_length, indent);
        buffer_write(buffer, buffer_length, lparen);
        buffer_write(buffer, buffer_length, op);
        buffer_write(buffer, buffer_length, newline);

        format_ast_index(self, buffer, buffer_length, binop.lhs, spaces + 2);
        buffer_write(buffer, buffer_length, newline);

        format_ast_index(self, buffer, buffer_length, binop.rhs, spaces + 2);
        buffer_write(buffer, buffer_length, newline);
        buffer_write(buffer, buffer_length, rparen);
        break;
    }
    default: {
        buffer_write(buffer, buffer_length, STR("<unimplemented>"));
        break;
    }
    }
}

String format_ast(Parser *self) {
    char *buffer = (char *)malloc(sizeof(char));
    size_t buffer_length = 0;

    format_ast_index(self, buffer, &buffer_length, self->ast_arena.length - 1,
                     0);
    printf("buffer_length: %lu", buffer_length);
    return (String){buffer, buffer_length};
}
