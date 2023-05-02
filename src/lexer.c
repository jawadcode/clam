#include <alloca.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"

// Create a new 'Lexer' which lexes 'source'
inline Lexer new_lexer(const String source) {
    return (Lexer){
        .source = source,
        .start = 0,
        .current = 0,
        .peeked = {MAYBE_NONE, {.none = NULL}},
    };
}

static inline bool is_ident(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static inline bool is_digit(char c) { return c >= '0' && c <= '9'; }

static inline char next_char(Lexer *lexer) {
    return lexer->source.buffer[lexer->current];
}

static inline bool is_at_end(Lexer *lexer) {
    return (size_t)lexer->current + 1 == lexer->source.length;
}

static inline char peek(Lexer *lexer) {
    return lexer->source.buffer[lexer->current];
}

static inline void skip(Lexer *lexer) { lexer->current++; }

static inline char advance(Lexer *lexer) {
    return lexer->source.buffer[lexer->current++];
}

static inline char safe_advance(Lexer *lexer) {
    if (is_at_end(lexer))
        return '\0';
    else
        return lexer->source.buffer[lexer->current + 1];
}

static inline bool match(Lexer *lexer, char expected) {
    if (is_at_end(lexer) || next_char(lexer) != expected)
        return false;
    else {
        skip(lexer);
        return true;
    }
}

void skip_whitespace(Lexer *lexer) {
    for (;;) {
        switch (peek(lexer)) {
        case '\n':
        case '\r':
        case '\t':
        case ' ':
            skip(lexer);
            break;
        case '#':
            skip(lexer);
            while (peek(lexer) != '\n' && !is_at_end(lexer))
                advance(lexer);
            break;
        default:
            return;
        }
    }
}

static TokenKind check_kw(Lexer *lexer, size_t start, size_t length,
                          const char *rest, TokenKind kind) {
    if (lexer->current - lexer->start == start + length &&
        memcmp(lexer->source.buffer + lexer->start + start, rest, length) == 0)
        return kind;
    else
        return TK_IDENT;
}

static TokenKind ident_type(Lexer *lexer) {
    switch (lexer->source.buffer[lexer->start]) {
    case 'a':
        return check_kw(lexer, 1, 2, "nd", TK_AND);
    case 'e':
        return check_kw(lexer, 1, 3, "lse", TK_ELSE);
    case 'f':
        if (lexer->current - lexer->start > 1) {
            switch (lexer->source.buffer[lexer->start + 1]) {
            case 'a':
                return check_kw(lexer, 2, 3, "lse", TK_FALSE);
            case 'u':
                return check_kw(lexer, 2, 1, "n", TK_FUN);
            }
        }
        break;
    case 'i':
        return check_kw(lexer, 1, 1, "n", TK_IN);
    case 'l':
        return check_kw(lexer, 1, 2, "et", TK_LET);
    case 'n':
        return check_kw(lexer, 1, 2, "ot", TK_NOT);
    case 'o':
        return check_kw(lexer, 1, 1, "r", TK_OR);
    case 'p':
        return check_kw(lexer, 1, 4, "rint", TK_PRINT);
    case 't':
        return check_kw(lexer, 1, 3, "hen", TK_THEN);
    case 'u':
        return check_kw(lexer, 1, 3, "nit", TK_UNIT);
    }

    return TK_IDENT;
}

static TokenKind ident(Lexer *lexer) {
    while (is_ident(peek(lexer)) || is_digit(peek(lexer)))
        skip(lexer);

    return ident_type(lexer);
}

static TokenKind number(Lexer *lexer) {
    while (is_digit(peek(lexer)))
        skip(lexer);

    // Look for a fractional part
    if (peek(lexer) == '.' && is_digit(safe_advance(lexer))) {
        // Consume the "."
        skip(lexer);

        while (is_digit(peek(lexer)))
            skip(lexer);
    }

    return TK_NUMBER;
}

static TokenKind string(Lexer *lexer) {
    bool escaped = false;

    while (!is_at_end(lexer)) {
        if (peek(lexer) == '\\') {
            skip(lexer);
            escaped = true;
            continue;
        } else if (escaped) {
            skip(lexer);
            escaped = false;
            continue;
        } else if (peek(lexer) == '"') {
            break;
        }

        skip(lexer);
    }

    if (is_at_end(lexer))
        return TK_INVALID;

    skip(lexer); // closing quote
    return TK_STRING;
}

static TokenKind next_kind(Lexer *lexer) {
    if (is_at_end(lexer))
        return TK_EOF;

    char c = advance(lexer);
    if (is_ident(c))
        return ident(lexer);
    else if (is_digit(c))
        return number(lexer);

    switch (c) {
    case '(':
        return TK_LPAREN;
    case ')':
        return TK_RPAREN;
    case '[':
        return TK_LSQUARE;
    case ']':
        return TK_RSQUARE;
    case ',':
        return TK_COMMA;
    case '+':
        return TK_ADD;
    case '-':
        return TK_SUB;
    case '*':
        return TK_MUL;
    case '/':
        return TK_DIV;
    case '%':
        return TK_MOD;
    case '!':
        return match(lexer, '=') ? TK_NEQ : TK_INVALID;
    case '=':
        return match(lexer, '=')   ? TK_EQ
               : match(lexer, '>') ? TK_ARROW
                                   : TK_ASSIGN;
    case '<':
        return match(lexer, '=') ? TK_LEQ : TK_LT;
    case '>':
        return match(lexer, '=') ? TK_GEQ : TK_GT;
    case '"':
        return string(lexer);
    default:
        return TK_INVALID;
    }
}

static inline MaybeToken take(MaybeToken *self) {
    MaybeToken temp = *self;
    *self = (MaybeToken){MAYBE_NONE, {.none = NULL}};
    return temp;
}

// TODO: Remove the need for a null terminator at the end of 'lexer->source'
Token next_token(Lexer *lexer) {
    MaybeToken peeked = take(&lexer->peeked);
    switch (peeked.tag) {
    case MAYBE_SOME:
        return peeked.value.some;
    case MAYBE_NONE: {
        skip_whitespace(lexer);

        lexer->start = lexer->current;
        return (Token){.kind = next_kind(lexer),
                       .span = {
                           .start = lexer->start,
                           .end = lexer->current,
                       }};
    }
    }
}

Token *peek_token(Lexer *lexer) {
    if (lexer->peeked.tag == MAYBE_NONE) {
        lexer->peeked = (MaybeToken){
            .tag = MAYBE_SOME,
            .value = {.some = next_token(lexer)},
        };
    }

    return &lexer->peeked.value.some;
}

String token_kind_to_string(TokenKind kind) {
    switch (kind) {
    case TK_LET:
        return STR("let");
    case TK_IN:
        return STR("in");
    case TK_FUN:
        return STR("fun");
    case TK_IF:
        return STR("if");
    case TK_THEN:
        return STR("then");
    case TK_ELSE:
        return STR("else");
    case TK_PRINT:
        return STR("print");
    case TK_TRUE:
        return STR("true");
    case TK_FALSE:
        return STR("false");
    case TK_UNIT:
        return STR("unit");
    case TK_NUMBER:
        return STR("numeric literal");
    case TK_STRING:
        return STR("string literal");
    case TK_IDENT:
        return STR("identifier");
    case TK_ASSIGN:
        return STR("=");
    case TK_ARROW:
        return STR("=>");
    case TK_LPAREN:
        return STR("(");
    case TK_RPAREN:
        return STR(")");
    case TK_LSQUARE:
        return STR("[");
    case TK_RSQUARE:
        return STR("]");
    case TK_COMMA:
        return STR(",");
    case TK_FNPIPE:
        return STR("|>");
    case TK_ADD:
        return STR("+");
    case TK_SUB:
        return STR("-");
    case TK_MUL:
        return STR("*");
    case TK_DIV:
        return STR("/");
    case TK_MOD:
        return STR("%");
    case TK_NOT:
        return STR("!");
    case TK_AND:
        return STR("and");
    case TK_OR:
        return STR("or");
    case TK_LT:
        return STR("<");
    case TK_LEQ:
        return STR("<=");
    case TK_GT:
        return STR(">");
    case TK_GEQ:
        return STR(">=");
    case TK_EQ:
        return STR("==");
    case TK_NEQ:
        return STR("!=");
    case TK_INVALID:
        return STR("invalid token");
    case TK_EOF:
        return STR("EOF");
    }
}

String token_to_string(Lexer lexer, Token token) {
    return (String){
        .buffer = lexer.source.buffer + token.span.start,
        .length = token.span.end - token.span.start,
    };
}

void print_token(const String source, Token token) {
    String kind = token_kind_to_string(token.kind);

    size_t length = token.span.end - token.span.start;
    const char *start = source.buffer + token.span.start;

    char *text = (char *)alloca((length + 1) * sizeof(char));
    text = (char *)memcpy(text, start, length);
    text[length] = '\0';

    String_print(kind);
    printf("%*c @ %zu..%zu \"%s\"\n", 16 - (int)kind.length, ' ',
           token.span.start, token.span.end, text);
}

DEF_VEC(Token, TokenVec)
