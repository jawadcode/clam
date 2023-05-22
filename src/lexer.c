#include "lexer.h"
#include <alloca.h>
#include <stddef.h>
#include <string.h>

inline Lexer Lexer_new(const String source) {
    return (Lexer){
        .source = source,
        .start = 0,
        .current = 0,
        .peeked = {.tag = MAYBE_NONE},
    };
}

static inline void skip(Lexer *lexer) { lexer->current++; }

static inline char peek(Lexer *lexer) {
    if (lexer->current == (lexer->source.length - 1))
        return '\0';
    else
        return lexer->source.buffer[lexer->current];
}

static inline char next(Lexer *lexer) {
    return lexer->source.buffer[lexer->current++];
}

static inline bool at_end(Lexer *lexer) {
    return lexer->current == (lexer->source.length - 1);
}

static inline char checked_next(Lexer *lexer) {
    if (at_end(lexer))
        return '\0';
    else
        return lexer->source.buffer[lexer->current + 1];
}

static void skip_whitespace(Lexer *lexer) {
    while (true) {
        switch (peek(lexer)) {
        case '\n':
        case '\r':
        case '\t':
        case ' ':
            skip(lexer);
            break;
        case '#':
            skip(lexer);
            for (char c = peek(lexer); c != '\n' && c != '\0'; c++)
                skip(lexer);
            break;
        default:
            return;
        }
    }
}

static inline bool is_ident(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static inline bool is_digit(char c) { return c >= '0' && c <= '9'; }

static inline MaybeToken take(MaybeToken *self) {
    MaybeToken temp = *self;
    *self = (MaybeToken){.tag = MAYBE_NONE};
    return temp;
}

static TokenKind check_kw(Lexer *lexer, size_t start, const String rest,
                          TokenKind kind) {
    if (lexer->current - lexer->start == start + rest.length &&
        memcmp(lexer->source.buffer + lexer->start + start, rest.buffer,
               rest.length) == 0)
        return kind;
    else
        return TK_IDENT;
}

static TokenKind ident_type(Lexer *lexer) {
    switch (lexer->source.buffer[lexer->start]) {
    case 'a':
        return check_kw(lexer, 1, STR("nd"), TK_AND);
    case 'e':
        return check_kw(lexer, 1, STR("lse"), TK_ELSE);
    case 'f':
        if (lexer->current - lexer->start > 1) {
            switch (lexer->source.buffer[lexer->start + 1]) {
            case 'a':
                return check_kw(lexer, 2, STR("lse"), TK_FALSE);
            case 'u':
                return check_kw(lexer, 2, STR("n"), TK_FUN);
            }
        }
        break;
    case 'i':
        if (lexer->current - lexer->start > 1) {
            switch (lexer->source.buffer[lexer->start + 1]) {
            case 'f':
                return check_kw(lexer, 2, STR(""), TK_IF);
            case 'n':
                return check_kw(lexer, 2, STR(""), TK_IN);
            }
        }
    case 'l':
        return check_kw(lexer, 1, STR("et"), TK_LET);
    case 'n':
        return check_kw(lexer, 1, STR("ot"), TK_NOT);
    case 'o':
        return check_kw(lexer, 1, STR("r"), TK_OR);
    case 'p':
        return check_kw(lexer, 1, STR("rint"), TK_PRINT);
    case 't':
        if (lexer->current - lexer->start > 1) {
            switch (lexer->source.buffer[lexer->start + 1]) {
            case 'h':
                return check_kw(lexer, 2, STR("en"), TK_THEN);
            case 'r':
                return check_kw(lexer, 2, STR("ue"), TK_TRUE);
            }
        }
    case 'u':
        return check_kw(lexer, 1, STR("nit"), TK_UNIT);
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
    if (peek(lexer) == '.' && is_digit(checked_next(lexer))) {
        // Consume the "."
        skip(lexer);

        while (is_digit(peek(lexer)))
            skip(lexer);

        return TK_FLOAT;
    } else {
        return TK_INT;
    }
}

static inline bool match(Lexer *lexer, char expected) {
    if (at_end(lexer) || peek(lexer) != expected)
        return false;
    else {
        skip(lexer);
        return true;
    }
}

static inline TokenKind string(Lexer *lexer) {
    bool escaped = false;

    while (!at_end(lexer)) {
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
        } else
            skip(lexer);
    }

    if (at_end(lexer))
        return TK_INVALID;
    else {
        skip(lexer);
        return TK_STRING;
    }
}

TokenKind next_kind(Lexer *lexer) {
    skip_whitespace(lexer);

    if (at_end(lexer))
        return TK_EOF;

    char c = next(lexer);
    if (is_ident(c))
        return ident(lexer);
    else if (is_digit(c))
        return number(lexer);
    else
        switch (c) {
        case '(':
            return TK_LPAREN;
        case ')':
            return TK_RPAREN;
        case '[':
            return TK_LSQUARE;
        case ']':
            return TK_RSQUARE;
        case '{':
            return TK_LCURLY;
        case '}':
            return TK_RCURLY;
        case ',':
            return TK_COMMA;
        case '|':
            return match(lexer, '>') ? TK_FNPIPE : TK_INVALID;
        case ':':
            return match(lexer, ':') ? TK_APPEND : TK_INVALID;
        case '+':
            return match(lexer, '+') ? TK_CONCAT : TK_ADD;
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

Token Lexer_next_token(Lexer *lexer) {
    MaybeToken peeked = take(&lexer->peeked);
    switch (peeked.tag) {
    case MAYBE_SOME:
        return peeked.some;
    case MAYBE_NONE:
        skip_whitespace(lexer);
        lexer->start = lexer->current;
        return (Token){.kind = next_kind(lexer),
                       .span = {
                           .start = lexer->start,
                           .end = lexer->current,
                       }};
    }
}

Token *Lexer_peek_token(Lexer *lexer) {
    if (lexer->peeked.tag == MAYBE_NONE) {
        lexer->peeked = (MaybeToken){
            .tag = MAYBE_SOME,
            .some = Lexer_next_token(lexer),
        };
    }

    return &lexer->peeked.some;
}

String TK_to_string(TokenKind kind) {
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
    case TK_INT:
        return STR("integer literal");
    case TK_FLOAT:
        return STR("float literal");
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
    case TK_LCURLY:
        return STR("{");
    case TK_RCURLY:
        return STR("}");
    case TK_COMMA:
        return STR(",");
    case TK_FNPIPE:
        return STR("|>");
    case TK_APPEND:
        return STR("::");
    case TK_CONCAT:
        return STR("++");
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
        return STR("not");
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

String Token_to_string(const String source, Token token) {
    return (String){
        .buffer = source.buffer + token.span.start,
        .length = token.span.end - token.span.start,
    };
}

void Token_print(const String source, Token token) {
    String kind = TK_to_string(token.kind);

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
