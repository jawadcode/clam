#include "parser.h"
#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "memory.h"
#include "result.h"
#include "vec.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

DEF_VEC_T(String, StringVec)

DEF_VEC(AST_LetBind, AST_LetBindVec)
DEF_VEC(ASTIndex, AST_List)

DEF_RESULT(StringBuf, SyntaxError, ParseString);
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

Parser new_parser(const char *file_name, const char *source) {
    // clang-format on
    return (Parser){
        .file_name = file_name,
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

    // Alloc enough space for the contents (i.e. minus the start and end
    // quotes, plus the null terminator) of the string in the source, which
    // means we might allocate in excess if has escape codes, but who cares
    StringBuf buffer = StringBuf_new();

    size_t index = 0;
    bool escaped = false;
    while (str[++index] != '"' || (escaped && str[index] == '"')) {
        if (escaped) {
            switch (str[index]) {
            case 'n':
                StringBuf_push(&buffer, '\n');
                break;
            case 'r':
                StringBuf_push(&buffer, '\r');
                break;
            case 't':
                StringBuf_push(&buffer, '\t');
                break;
            case '0':
                StringBuf_push(&buffer, '\0');
                break;
            case '"':
            case '\\':
                StringBuf_push(&buffer, str[index]);
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
            StringBuf_push(&buffer, str[index]);
            buffer.buffer[index] = str[index];
        }
    }
    return (ParseStringResult){
        .tag = RESULT_OK,
        .value = {.ok = buffer},
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
        StringBuf string;
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

static inline Span current_span(Parser *self) {
    return (Span){
        .start = self->lexer.start - self->lexer.source,
        .end = self->lexer.current - self->lexer.source,
    };
}

static TokenResult expect(Parser *self, TokenKind kind) {
    Token token = next(self);
    String kind_string = token_kind_to_string(kind);

    if (token.kind != kind) {
        return (TokenResult){
            .tag = RESULT_ERR,
            .value = {.err = {.tag = ERROR_UNEXPECTED_TOKEN,
                              .error = {.unexpected_token =
                                            {.expected = kind_string,
                                             .got = token,
                                             .span = current_span(self)}}}},
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

static ParseResult parse_abstraction(Parser *self) {
    Token fun_token = next(self);
    StringVec args = StringVec_new();
    Token first_param_token;
    RET_ERR_ASSIGN(first_param_token, expect(self, TK_IDENT), TokenResult,
                   ParseResult);
    StringVec_push(&args, token_to_string(&self->lexer, first_param_token));

    while (at(self, TK_IDENT)) {
        Token arg_token = next(self);
        StringVec_push(&args, token_to_string(&self->lexer, arg_token));
    }
    RET_ERR(TokenResult, expect(self, TK_ARROW), ParseResult);

    ASTIndex abs;
    RET_ERR_ASSIGN(abs, parse_expr(self), ParseResult, ParseResult);
    Span abs_span = {.start = fun_token.span.start,
                     .end = self->ast_arena.buffer[abs].span.end};
    for (size_t i = args.length; i > 0; i--) {
        AST body_ast = {
            .tag = AST_ABSTRACTION,
            .value = {.abstraction =
                          {
                              .argument = args.buffer[i - 1],
                              .body = abs,
                          }},
            .span = abs_span,
        };
        abs = ASTVec_push(&self->ast_arena, body_ast);
    }
    StringVec_free(&args);
    return (ParseResult){
        .tag = RESULT_OK,
        .value =
            {
                .ok = abs,
            },
    };
}

static ASTResult parse_let_binding(Parser *self) {
    Span span = (Span){.start = next(self).span.start};
    AST_LetBindVec binds = AST_LetBindVec_new();
    while (at(self, TK_IDENT)) {
        Span ident_span = next(self).span;
        String ident = {.buffer = self->source + ident_span.start,
                        .length = ident_span.end - ident_span.start};
        RET_ERR(TokenResult, expect(self, TK_ASSIGN), ASTResult);
        ASTIndex value;
        RET_ERR_ASSIGN(value, parse_expr(self), ParseResult, ASTResult);
        AST_LetBindVec_push(&binds, (AST_LetBind){ident, value});
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
        .value = {.let_in = (AST_LetIn){binds, body}},
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
    while (!at_any(self, (TokenKind[]){TK_COMMA, TK_RSQUARE}, 2)) {
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
        RET_ERR_ASSIGN(lhs, parse_abstraction(self), ParseResult, ParseResult);
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
        next(self);
        return (ParseResult){
            .tag = RESULT_ERR,
            .value = {
                .err = (SyntaxError){
                    .tag = ERROR_UNEXPECTED_TOKEN,
                    .error = {.unexpected_token = (SyntaxError_UnexpectedToken){
                                  .expected = STR("expression"),
                                  .got = next(self),
                                  .span = current_span(self),
                              }}}}};
    }
    }
    while (true) {
        if (at_any(self, TK_EXPR, TK_EXPR_LEN)) {
            ASTIndex argument;
            RET_ERR_ASSIGN(argument, parse_expr(self), ParseResult,
                           ParseResult);

            AST lhs_ast = {.tag = AST_APPLICATION,
                           .value =
                               {
                                   .application =
                                       {
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
            next(self);
            return (ParseResult){
                .tag = RESULT_ERR,
                .value = {.err = {.tag = ERROR_UNEXPECTED_TOKEN,
                                  .error = {.unexpected_token = {
                                                .expected = STR("expression"),
                                                .got = next(self),
                                                .span = current_span(self)}}}}};
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

    while (true) {
        Token *peeked = peek(self);
        Token op;
        if (in(peeked->kind, TK_BINOPS, TK_BINOPS_LEN)) {
            op = next(self);
        } else if (in(peeked->kind, TK_EXPR_TERMINATORS,
                      TK_EXPR_TERMINATORS_LEN)) {
            break;
        } else {
            next(self);
            return (ParseResult){
                .tag = RESULT_ERR,
                .value = {
                    .err = {
                        .tag = ERROR_UNEXPECTED_TOKEN,
                        .error = {.unexpected_token = {
                                      .expected = STR(
                                          "operator or expression terminator"),
                                      .got = next(self),
                                      .span = current_span(self)}}}}};
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

void Parser_free(Parser *self) {
    for (size_t index = 0; index < self->ast_arena.length; index++) {
        AST item = self->ast_arena.buffer[index];
        switch (item.tag) {
        case AST_LIST:
            AST_List_free(&item.value.list);
        case AST_LET_IN:
            AST_LetBindVec_free(&item.value.let_in.bindings);
        case AST_LITERAL:
        case AST_IDENT:
        case AST_ABSTRACTION:
        case AST_APPLICATION:
        case AST_IF_ELSE:
        case AST_UNARY_OP:
        case AST_BINARY_OP:
        case AST_LIST_INDEX:
            break;
        }
    }

    ASTVec_free(&self->ast_arena);
}

typedef struct {
    size_t line_num;
    size_t line_start;
    size_t line_end;
} LineInfo;

static LineInfo get_line_nums(String source, size_t span_start) {
    LineInfo result = {.line_num = 1, .line_start = 0};

    bool on_line = false;
    for (size_t i = 0; i < source.length; i++) {
        char c = source.buffer[i];
        if (i == span_start)
            on_line = true;
        else if (c == '\n') {
            if (on_line) {
                result.line_end = i;
                break;
            }
            result.line_num++;
            result.line_start = i + 1;
        }
    }

    return result;
}

static inline void write_repeat(char c, size_t n, FILE *stream) {
    for (size_t i = 0; i < n; i++)
        fputc(c, stream);
}

static inline void write_num(size_t n, FILE *stream) {
    if (n / 10)
        write_num(n / 10, stream);
    fputc(n % 10 + '0', stream);
}

void SyntaxError_print_diag(Parser *self, SyntaxError error, FILE *stream) {
    fputs("\x1b[31;1mError\x1b[0m: ", stream);
    Span span;
    switch (error.tag) {
    case ERROR_INVALID_ESC_SEQ:
        fputs("invalid escape sequence\n", stream);
        span = error.error.invalid_esc_seq.escape_sequence;
        break;
    case ERROR_UNEXPECTED_TOKEN:
        fputs("unexpected token\n", stream);
        span = error.error.unexpected_token.span;
        break;
    }
    LineInfo line_info = get_line_nums(
        (String){.buffer = self->source, .length = strlen(self->source)},
        span.start);
    size_t num_digits = floor(log10((double)line_info.line_num) + 1.0);
    write_repeat(' ', num_digits + 2, stream);
    fputs("\x1b[37m┌─[\x1b[0m", stream);
    String_write(
        (String){.buffer = self->file_name, .length = strlen(self->file_name)},
        stream);
    fputc(':', stream);
    write_num(line_info.line_num, stream);
    fputc(':', stream);
    write_num(span.start - line_info.line_start, stream);
    fputs("\x1b[37m]\n", stream);
    write_repeat(' ', num_digits + 2, stream);
    fputs("│\n", stream);
    fputc(' ', stream);
    write_num(line_info.line_num, stream);
    fputs(" │ ", stream);
    String_write((String){.buffer = self->source + line_info.line_start,
                          .length = span.start - line_info.line_start},
                 stream);
    fputs("\x1b[0m", stream);
    String_write((String){.buffer = self->source + span.start,
                          .length = span.end - span.start},
                 stream);
    fputs("\x1b[37m", stream);
    String_write((String){.buffer = self->source + span.end,
                          .length = line_info.line_end - span.end},
                 stream);
    fputc('\n', stream);
    write_repeat(' ', num_digits + 4 + span.start - line_info.line_start,
                 stream);
    fputs("\x1b[0m", stream);
    write_repeat('^', span.end - span.start, stream);
    fputc('\n', stream);
    write_repeat(' ', num_digits + 4 + span.start - line_info.line_start,
                 stream);
    switch (error.tag) {
    case ERROR_INVALID_ESC_SEQ:
        fputs("invalid escape sequence '\\", stream);
        fputc(*(self->source + span.start), stream);
        fputc('\'', stream);
        break;
    case ERROR_UNEXPECTED_TOKEN: {
        SyntaxError_UnexpectedToken ut = error.error.unexpected_token;
        fputs("expected ", stream);
        String_write(ut.expected, stream);
        fputs(", got '", stream);
        String_write(token_to_string(&self->lexer, ut.got), stream);
        fputc('\'', stream);
        break;
    }
    }
    fputc('\n', stream);
}
