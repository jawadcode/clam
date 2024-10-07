#include "string.h"
#include <locale.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "result.h"
#include "vec.h"

DEF_VEC_T(String, StringVec)

DEF_VEC(AST_LetBind, AST_LetBindVec)
DEF_VEC(ASTIndex, AST_List)

DEF_RESULT(StringBuf, SyntaxError, ParseString);
DEF_RESULT(Token, SyntaxError, Token);
DEF_RESULT(AST, SyntaxError, AST);

Parser Parser_new(const String file_name, const String source) {
    return (Parser){
        .file_name = file_name,
        .source = source,
        .lexer = Lexer_new(source),
        .ast_arena = ASTVec_new(),
    };
}

static inline Token *peek(Parser *self) {
    return Lexer_peek_token(&self->lexer);
}

static inline Token next(Parser *self) {
    return Lexer_next_token(&self->lexer);
}

static int32_t parse_int(Parser *self, Span span) {
    const char *num_str = self->lexer.source.buffer + span.start;

    int32_t value = 0;
    while (num_str < self->source.buffer + span.end && *num_str >= 0 &&
           *num_str <= '9')
        value = value * 10 + (*num_str++ - '0');

    return value;
}

static double parse_float(Parser *self, Span span) {
    const char *num_str = self->lexer.source.buffer + span.start;

    double value = 0.0;
    while (num_str < self->source.buffer + self->source.length &&
           *num_str >= '0' && *num_str <= '9') {
        value = value * 10.0 + (*num_str++ - '0');
    }

    if (*num_str == '.')
        num_str++;

    double power = 1.0;
    while (*num_str >= '0' && *num_str <= '9') {
        value = value * 10.0 + (*num_str++ - '0');
        power *= 10.0;
    }

    return value / power;
}

static ParseStringResult parse_string(Parser *self, Span span) {
    SyntaxError error;
    const char *str = self->lexer.source.buffer + span.start;
    // The parsed string will always be less than or equal to the length - 2
    // (for the quotes)
    StringBuf buffer = StringBuf_with_capacity((span.end - span.start) - 2);

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
                size_t start = str + index - self->lexer.source.buffer - 1;
                size_t end = start + 2;
                error = (SyntaxError){
                    ERROR_INVALID_ESC_SEQ,
                    {
                        .invalid_esc_seq =
                            {
                                .string = span,
                                .escape_sequence = {start, end},
                            },
                    },
                };
                goto FAILURE;
            }
            }
            escaped = false;
        } else if (str[index] == '\\') {
            escaped = true;
        } else {
            StringBuf_push(&buffer, str[index]);
        }
    }
    return (ParseStringResult){
        .tag = RESULT_OK,
        .value = {.ok = buffer},
    };
FAILURE:
    StringBuf_free(&buffer);
    return (ParseStringResult){
        .tag = RESULT_ERR,
        .value = {.err = error},
    };
}

static ASTResult parse_literal(Parser *self) {
    SyntaxError error;
    Token current = next(self);
    AST_Literal lit;

    switch (current.kind) {
    case TK_UNIT:
        lit = (AST_Literal){.tag = LITERAL_UNIT};
        break;
    case TK_TRUE:
        lit = (AST_Literal){.tag = LITERAL_BOOL, .value = {.boolean = true}};
        break;
    case TK_FALSE:
        lit = (AST_Literal){.tag = LITERAL_BOOL, .value = {.boolean = false}};
        break;
    case TK_INT:
        lit =
            (AST_Literal){.tag = LITERAL_INT,
                          .value = {.integer = parse_int(self, current.span)}};
        break;
    case TK_FLOAT:
        lit =
            (AST_Literal){.tag = LITERAL_FLOAT,
                          .value = {.floate = parse_float(self, current.span)}};
        break;
    case TK_STRING: {
        StringBuf string;
        RET_ERR_ASSIGN(string, ParseStringResult,
                       parse_string(self, current.span));
        lit = (AST_Literal){
            .tag = LITERAL_STRING,
            .value = {.string = string},
        };
        break;
    }
    default:
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
FAILURE:
    return (ASTResult){.tag = RESULT_ERR, .value = {.err = error}};
}

static AST parse_ident(Parser *self) {
    Span curr_span = next(self).span;

    return (AST){
        .tag = AST_IDENT,
        .value =
            {
                .ident =
                    {
                        .buffer = self->lexer.source.buffer + curr_span.start,
                        .length = curr_span.end - curr_span.start,
                    },
            },
        .span = curr_span};
}

static inline Span current_span(Parser *self) {
    return (Span){
        .start = self->lexer.start,
        .end = self->lexer.current,
    };
}

static TokenResult expect(Parser *self, TokenKind kind) {
    Token token = next(self);
    String kind_string = TK_to_string(kind);

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

static inline bool at(Parser *self, TokenKind kind) {
    return peek(self)->kind == kind;
}

static bool at_any(Parser *self, const TokenKind list[], size_t length) {
    Token *current = peek(self);

    for (size_t i = 0; i < length; i++)
        if (current->kind == list[i])
            return true;

    return false;
}

static ParseResult parse_expr_bp(Parser *self, uint8_t binding_power);

static ParseResult parse_expr(Parser *self);

static inline size_t get_end(Parser *self, ASTIndex node) {
    return self->ast_arena.buffer[node].span.end;
}

static ParseResult parse_abstraction(Parser *self) {
    SyntaxError error;
    Token fun_token = next(self);
    StringVec args = StringVec_new();
    Token first_param_token;
    RET_ERR_ASSIGN(first_param_token, TokenResult, expect(self, TK_IDENT));
    StringVec_push(&args, Token_to_string(self->source, first_param_token));

    while (at(self, TK_IDENT)) {
        Token arg_token = next(self);
        StringVec_push(&args, Token_to_string(self->source, arg_token));
    }
    RET_ERR(TokenResult, expect(self, TK_ARROW));

    ASTIndex abs;
    RET_ERR_ASSIGN(abs, ParseResult, parse_expr(self));
    Span abs_span = {.start = fun_token.span.start, .end = get_end(self, abs)};
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
FAILURE:
    StringVec_free(&args);
    return (ParseResult){.tag = RESULT_ERR, .value = {.err = error}};
}

static ASTResult parse_print(Parser *self) {
    SyntaxError error;
    Span span = {.start = next(self).span.start};
    ASTIndex expr;
    RET_ERR_ASSIGN(expr, ParseResult, parse_expr(self));
    span.end = get_end(self, expr);
    AST print = {
        .tag = AST_PRINT,
        .value = {.print =
                      {
                          .expr = expr,
                      }},
        .span = span,
    };
    return (ASTResult){.tag = RESULT_OK, .value = {.ok = print}};
FAILURE:
    return (ASTResult){.tag = RESULT_ERR, .value = {.err = error}};
}

static ASTResult parse_if_then(Parser *self) {
    SyntaxError error;
    Span span = (Span){.start = next(self).span.start};
    ASTIndex cond;
    RET_ERR_ASSIGN(cond, ParseResult, parse_expr(self));
    RET_ERR(TokenResult, expect(self, TK_THEN));
    ASTIndex then;
    RET_ERR_ASSIGN(then, ParseResult, parse_expr(self));
    RET_ERR(TokenResult, expect(self, TK_ELSE));
    ASTIndex else_;
    RET_ERR_ASSIGN(else_, ParseResult, parse_expr(self));
    span.end = get_end(self, else_);
    AST if_then = {
        .tag = AST_IF_ELSE,
        .value = {.if_else =
                      {
                          .condition = cond,
                          .then = then,
                          .else_ = else_,
                      }},
        .span = span,
    };
    return (ASTResult){.tag = RESULT_OK, .value = {.ok = if_then}};
FAILURE:
    return (ASTResult){.tag = RESULT_ERR, .value = {.err = error}};
}

static ASTResult parse_let_binding(Parser *self) {
    SyntaxError error;
    Span span = (Span){.start = next(self).span.start};
    AST_LetBindVec binds = AST_LetBindVec_new();
    while (at(self, TK_IDENT)) {
        Span ident_span = next(self).span;
        String ident = {.buffer = self->source.buffer + ident_span.start,
                        .length = ident_span.end - ident_span.start};
        RET_ERR(TokenResult, expect(self, TK_ASSIGN));
        ASTIndex value;
        RET_ERR_ASSIGN(value, ParseResult, parse_expr(self));
        AST_LetBindVec_push(&binds, (AST_LetBind){ident_span, ident, value});
        Token *peeked = peek(self);
        if (peeked->kind == TK_COMMA) {
            next(self);
        }
    }
    RET_ERR(TokenResult, expect(self, TK_IN));
    ASTIndex body;
    RET_ERR_ASSIGN(body, ParseResult, parse_expr(self));
    span.end = get_end(self, body);
    AST let_in = (AST){
        .tag = AST_LET_IN,
        .value = {.let_in = (AST_LetIn){binds, body}},
        .span = span,
    };
    return (ASTResult){.tag = RESULT_OK, .value = {.ok = let_in}};
FAILURE:
    AST_LetBindVec_free(&binds);
    return (ASTResult){.tag = RESULT_ERR, .value = {.err = error}};
}

static ParseResult parse_term(Parser *self);

// This is guaranteed to be fed a valid tokenkind, hence why it just returns an
// int
static uint8_t prefix_binding_power(AST_UnOp op) {
    switch (op) {
    case UNOP_NEGATE:
        return 18;
    case UNOP_NOT:
        /* Typically the binding power of `!` is stronger than logical
           connectives but weaker than all other operators so that `!` can be
           easily used on variables but not interfere with those other
           operators, however since we are using `not`, I have chosen the
           weakest binding power, as it corresponds better to natural language,
           and makes negated if conditions cleaner, e.g `if not (A and B) then
           ...` vs `if not A and B then ...` (idea stolen from python). */
        return 1;
    default:
        UNREACHABLE;
    }
}

// Returns false for failure, otherwise writes to `left` and `right` (and
// returns true)
static bool infix_binding_power(TokenKind op, uint8_t *left, uint8_t *right) {
    switch (op) {
    case TK_OR: {
        *left = 2;
        *right = 3;
        break;
    }
    case TK_AND: {
        *left = 4;
        *right = 5;
        break;
    }
    case TK_EQ:
    case TK_NEQ:
    case TK_LT:
    case TK_GT:
    case TK_LEQ:
    case TK_GEQ:
    case TK_FNPIPE: {
        // Changed to left associative because `|>` requires it and it doesn't
        // affect relational operators as you can't chain comparisons anyways,
        // this isn't python smh.
        // Also grouped together equality and comparison operators because I
        // have no idea where `|>` would slot in were they separate (I'm copying
        // OCaml here)
        *left = 7;
        *right = 6;
        break;
    }
    case TK_APPEND: {
        // Left associative because it constructs a Snoc and not a Cons list.
        // Not entirely sure if this logic is sound but I guess we will see.
        *left = 9;
        *right = 8;
        break;
    }
    case TK_ADD:
    case TK_SUB: {
        *left = 10;
        *right = 11;
        break;
    }
    case TK_MUL:
    case TK_DIV:
    case TK_MOD: {
        *left = 12;
        *right = 13;
        break;
    }
    case TK_CONCAT: {
        *left = 14;
        *right = 15;
    }
        // clang-format off
	/* For completeness' sake:
	case TK_APP: {
		*left = 16;
		*right = 17;
		break;
	} */
        // clang-format on
    default:
        return false;
    }
    return true;
}

static ASTResult parse_prefix_op(Parser *self) {
    SyntaxError error;
    Token op_token = next(self);
    Span op_span = op_token.span;
    AST_UnOp op = op_token.kind == TK_NOT ? UNOP_NOT : UNOP_NEGATE;

    uint8_t right_binding_power = prefix_binding_power(op);

    ASTIndex operand;
    RET_ERR_ASSIGN(operand, ParseResult,
                   parse_expr_bp(self, right_binding_power));

    AST unop =
        (AST){.tag = AST_UNARY_OP,
              .value = {.unary_op = (AST_UnaryOp){op_span, op, operand}},
              .span = {.start = op_span.start, .end = get_end(self, operand)}};
    return (ASTResult){.tag = RESULT_OK, .value = {.ok = unop}};
FAILURE:
    return (ASTResult){.tag = RESULT_ERR, .value = {.err = error}};
}

static ASTResult parse_list(Parser *self) {
    SyntaxError error;
    Span list_span = (Span){.start = next(self).span.start};
    AST_List items = AST_List_new();
    while (!at_any(self, (TokenKind[]){TK_COMMA, TK_RCURLY}, 2)) {
        ASTIndex item;
        RET_ERR_ASSIGN(item, ParseResult, parse_expr(self));
        AST_List_push(&items, item);
        if (at(self, TK_COMMA)) {
            next(self);
        } else if (at(self, TK_RCURLY)) {
            break;
        } else {
            error = (SyntaxError){
                .tag = ERROR_UNEXPECTED_TOKEN,
                .error = {.unexpected_token = {.expected = STR("',' or '}'"),
                                               .got = next(self),
                                               .span = current_span(self)}}};
            goto FAILURE;
        }
    }
    list_span.end = next(self).span.end;
    return (ASTResult){.tag = RESULT_OK,
                       .value = {.ok = (AST){.tag = AST_LIST,
                                             .value = {.list = items},
                                             .span = list_span}}};
FAILURE:
    AST_List_free(&items);
    return (ASTResult){.tag = RESULT_ERR, .value = {.err = error}};
}

static ParseResult parse_grouping(Parser *self) {
    SyntaxError error;
    size_t span_start = next(self).span.start;
    ASTIndex grouped;
    RET_ERR_ASSIGN(grouped, ParseResult, parse_expr(self));
    Token rparen;
    RET_ERR_ASSIGN(rparen, TokenResult, expect(self, TK_RPAREN));
    size_t span_end = rparen.span.end;
    // Icky but I want to keep `parse_expr` as it is
    self->ast_arena.buffer[grouped].span =
        (Span){.start = span_start, .end = span_end};
    return (ParseResult){.tag = RESULT_OK, .value = {.ok = grouped}};
FAILURE:
    return (ParseResult){.tag = RESULT_ERR, .value = {.err = error}};
}

static ParseResult parse_term(Parser *self) {
    SyntaxError error;
    AST term_ast;
    TokenKind peeked = peek(self)->kind;
    switch (peeked) {
    case TK_UNIT:
    case TK_TRUE:
    case TK_FALSE:
    case TK_INT:
    case TK_FLOAT:
    case TK_STRING:
        RET_ERR_ASSIGN(term_ast, ASTResult, parse_literal(self));
        break;
    case TK_LCURLY:
        RET_ERR_ASSIGN(term_ast, ASTResult, parse_list(self));
        break;
    case TK_PRINT:
        RET_ERR_ASSIGN(term_ast, ASTResult, parse_print(self));
        break;
    case TK_FUN:
        return parse_abstraction(self);
    case TK_IF:
        RET_ERR_ASSIGN(term_ast, ASTResult, parse_if_then(self));
        break;
    case TK_LET:
        RET_ERR_ASSIGN(term_ast, ASTResult, parse_let_binding(self));
        break;
    case TK_IDENT:
        term_ast = parse_ident(self);
        break;
    case TK_SUB:
    case TK_NOT:
        RET_ERR_ASSIGN(term_ast, ASTResult, parse_prefix_op(self));
        break;
    case TK_LPAREN:
        return parse_grouping(self);
    default: {
        Token err_tok = next(self);
        error = (SyntaxError){
            .tag = ERROR_UNEXPECTED_TOKEN,
            .error = {.unexpected_token = {.expected = STR("expression"),
                                           .got = err_tok,
                                           .span = err_tok.span}}};
        goto FAILURE;
    }
    }
    ASTIndex term = ASTVec_push(&self->ast_arena, term_ast);
    return (ParseResult){.tag = RESULT_OK, .value = {.ok = term}};
FAILURE:
    return (ParseResult){.tag = RESULT_ERR, .value = {.err = error}};
}

#define BINOP_TOKENS_LEN 17
static const TokenKind BINOP_TOKENS[BINOP_TOKENS_LEN] = {
    TK_FNPIPE, TK_APPEND, TK_CONCAT, TK_ADD, TK_SUB, TK_MUL,
    TK_DIV,    TK_MOD,    TK_NOT,    TK_AND, TK_OR,  TK_LT,
    TK_LEQ,    TK_GT,     TK_GEQ,    TK_EQ,  TK_NEQ,
};

#define EXPR_TERMINATORS_LEN 8
static const TokenKind EXPR_TERMINATORS[EXPR_TERMINATORS_LEN] = {
    TK_IN, TK_THEN, TK_ELSE, TK_RPAREN, TK_RSQUARE, TK_RCURLY, TK_COMMA, TK_EOF,
};

#define TERM_TOKENS_LEN 9
static const TokenKind TERM_TOKENS[TERM_TOKENS_LEN] = {
    TK_UNIT,   TK_TRUE,   TK_FALSE, TK_INT,   TK_FLOAT,
    TK_STRING, TK_LCURLY, TK_IDENT, TK_LPAREN};

static ParseResult parse_expr_bp(Parser *self, uint8_t binding_power) {
    SyntaxError error;
    ASTIndex lhs;
    RET_ERR_ASSIGN(lhs, ParseResult, parse_term(self));

    while (true) {
        TokenKind op = peek(self)->kind;
        if (at_any(self, BINOP_TOKENS, BINOP_TOKENS_LEN) ||
            at_any(self, TERM_TOKENS, TERM_TOKENS_LEN)) {
            // noop
        } else if (at_any(self, EXPR_TERMINATORS, EXPR_TERMINATORS_LEN)) {
            break;
        } else {
            Token tok = next(self);
            return (ParseResult){
                .tag = RESULT_ERR,
                .value = {
                    .err = {
                        .tag = ERROR_UNEXPECTED_TOKEN,
                        .error = {.unexpected_token = {
                                      .expected = STR(
                                          "operator or expression terminator"),
                                      .got = tok,
                                      .span = tok.span,
                                  }}}}};
        }

        uint8_t left_binding_power, right_binding_power;
        if (infix_binding_power(op, &left_binding_power,
                                &right_binding_power)) {
            if (left_binding_power < binding_power)
                break;

            Token op_token = next(self);

            ASTIndex rhs;
            RET_ERR_ASSIGN(rhs, ParseResult,
                           parse_expr_bp(self, right_binding_power));

            AST ast = {.tag = AST_BINARY_OP,
                       .value = {.binary_op =
                                     {
                                         .op = (AST_BinOp)op_token.kind,
                                         .op_span = op_token.span,
                                         .lhs = lhs,
                                         .rhs = rhs,
                                     }},
                       .span = {.start = self->ast_arena.buffer[lhs].span.start,
                                .end = get_end(self, rhs)}};
            lhs = ASTVec_push(&self->ast_arena, ast);
        }
        if (at_any(self, TERM_TOKENS, TERM_TOKENS_LEN)) {
            if (16 < binding_power)
                break;

            ASTIndex arg;
            RET_ERR_ASSIGN(arg, ParseResult, parse_expr_bp(self, 17));

            AST ast = {
                .tag = AST_APPLICATION,
                .value = {.application = {.function = lhs, .argument = arg}},
                .span = {.start = self->ast_arena.buffer[lhs].span.start,
                         .end = get_end(self, arg)}};
            lhs = ASTVec_push(&self->ast_arena, ast);
        }
    }

    return (ParseResult){
        .tag = RESULT_OK,
        .value = {.ok = lhs},
    };
FAILURE:
    return (ParseResult){.tag = RESULT_ERR, .value = {.err = error}};
}

static inline ParseResult parse_expr(Parser *self) {
    return parse_expr_bp(self, 0);
}

ParseResult Parser_parse_expr(Parser *self) {
    SyntaxError error;
    ASTIndex ast;
    RET_ERR_ASSIGN(ast, ParseResult, parse_expr(self));
    RET_ERR(TokenResult, expect(self, TK_EOF));
    return (ParseResult){.tag = RESULT_OK, .value = {.ok = ast}};
FAILURE:
    return (ParseResult){.tag = RESULT_ERR, .value = {.err = error}};
}

void Parser_free(Parser *self) {
    for (size_t index = 0; index < self->ast_arena.length; index++) {
        AST item = self->ast_arena.buffer[index];
        switch (item.tag) {
        case AST_LIST:
            AST_List_free(&item.value.list);
            break;
        case AST_LET_IN:
            AST_LetBindVec_free(&item.value.let_in.bindings);
            break;
        case AST_LITERAL: {
            AST_Literal lit = item.value.literal;
            switch (lit.tag) {
            case LITERAL_STRING:
                StringBuf_free(&lit.value.string);
            default:
                (void)0;
            }
            break;
        }
        case AST_IDENT:
        case AST_ABSTRACTION:
        case AST_APPLICATION:
        case AST_PRINT:
        case AST_IF_ELSE:
        case AST_UNARY_OP:
        case AST_BINARY_OP:
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
        } else if (c == '\0') {
            if (on_line)
                result.line_end = i;
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
    fputc((int)n % 10 + '0', stream);
}

void Parser_print_diag(Parser *self, SyntaxError error, FILE *stream) {
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
    default:
        // To get MSVC to stfu
        UNREACHABLE;
    }
    LineInfo line_info = get_line_nums(
        (String){.buffer = self->source.buffer, .length = self->source.length},
        span.start);
    size_t num_digits = (size_t)(log10((double)line_info.line_num) + 1.0);
    write_repeat(' ', num_digits + 2, stream);
    fputs("┌─[", stream);
    String_write(self->file_name, stream);
    fputc(':', stream);
    write_num(line_info.line_num, stream);
    fputc(':', stream);
    write_num(span.start - line_info.line_start, stream);
    fputs("]\n", stream);
    write_repeat(' ', num_digits + 2, stream);
    fputs("│\n", stream);
    fputc(' ', stream);
    write_num(line_info.line_num, stream);
    fputs(" │ ", stream);
    String_write((String){.buffer = self->source.buffer + line_info.line_start,
                          .length = span.start - line_info.line_start},
                 stream);
    String_write((String){.buffer = self->source.buffer + span.start,
                          .length = span.end - span.start},
                 stream);
    String_write((String){.buffer = self->source.buffer + span.end,
                          .length = line_info.line_end - span.end},
                 stream);
    fputc('\n', stream);
    write_repeat(' ', num_digits + 2, stream);
    fputs("│", stream);
    write_repeat(' ', span.start - line_info.line_start + 1, stream);
    write_repeat('^', span.end - span.start, stream);
    fputc('\n', stream);
    write_repeat(' ', num_digits + 4 + span.start - line_info.line_start,
                 stream);
    switch (error.tag) {
    case ERROR_INVALID_ESC_SEQ: {
        SyntaxError_InvalidEscSeq ies = error.error.invalid_esc_seq;
        fputs("invalid escape sequence '", stream);
        String_write(
            (String){.buffer = self->source.buffer + ies.escape_sequence.start,
                     .length =
                         ies.escape_sequence.end - ies.escape_sequence.start},
            stderr);
        fputc('\'', stream);
        break;
    }
    case ERROR_UNEXPECTED_TOKEN: {
        SyntaxError_UnexpectedToken ut = error.error.unexpected_token;
        fputs("expected ", stream);
        String_write(ut.expected, stream);
        fputs(", got '", stream);
        String_write(TK_to_string(ut.got.kind), stream);
        fputc('\'', stream);
        break;
    }
    }
    fputc('\n', stream);
}
