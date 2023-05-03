#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "vec.h"

// Ensure cmd.length > 2
static inline bool match_rest(const String cmd, const char *rest) {
    // not using 'memcmp' because we want to stop at the null-terminator in
    // 'rest'
    return strncmp(cmd.buffer + 1, rest, cmd.length - 1) == 0;
}

void print_help(void) {
    puts("\nCommands:\n"
         "  :exit - Exit the REPL\n"
         "  :help - Display this help message\n"
         "  :quit - Quit the REPL\n");
}

void run_cmd(const String cmd) {
    switch (cmd.buffer[0]) {
    case 'e':
        if (match_rest(cmd, "xit")) {
            puts("Bye bye...\n");
            exit(0);
        } else
            goto UNKNOWN_COMMAND;
    case 'h':
        if (match_rest(cmd, "elp")) {
            print_help();
            break;
        } else
            goto UNKNOWN_COMMAND;
    case 'q':
        if (match_rest(cmd, "uit")) {
            puts("Bye bye...\n");
            exit(0);
        } else
            goto UNKNOWN_COMMAND;
    UNKNOWN_COMMAND:
    default:
        fputs("Error: Unknown command ", stderr);
        String_print(cmd);
        print_help();
        break;
    }
}

void run(const String source) {
    Parser parser = new_parser("stdin", source);
    puts("Lexer Output:");
    for (Token tok = next_token(&parser.lexer); tok.kind != TK_EOF;
         tok = next_token(&parser.lexer))
        print_token(parser.source, tok);
    parser.lexer = new_lexer(parser.source);
    ParseResult result = parse_expr(&parser);
    putchar('\n');
    switch (result.tag) {
    case RESULT_OK: {
        StringBuf sexpr = format_ast(&parser.ast_arena, result.value.ok);
        puts("Parser Output:");
        StringBuf_print(sexpr);
        putchar('\n');
        StringBuf_free(&sexpr);
        break;
    }
    case RESULT_ERR: {
        SyntaxError error = result.value.err;
        SyntaxError_print_diag(&parser, error, stderr);
        break;
    }
    }

    Parser_free(&parser);
}

StringBuf read_line() {
    StringBuf str = StringBuf_new();
    char c = fgetc(stdin);
    while (c != '\n' && c != EOF) {
        StringBuf_push(&str, c);
        c = fgetc(stdin);
    }
    if (c == EOF)
        exit(0);
    // Ideally we could remove this but the lexer depends on it
    StringBuf_push(&str, '\0');
    return str;
}

void repl(void) {
    while (true) {
        fputs("$ ", stdout);
        fflush(stdout);
        StringBuf line = read_line();
        if (line.length > 2 && line.buffer[0] == ':') {
            run_cmd(
                (String){.buffer = line.buffer + 1, .length = line.length - 1});
        } else {
            run((String){.buffer = line.buffer, .length = line.length});
        }
        StringBuf_free(&line);
    }
}

void run_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fputs("Could not access file ", stdout);
        puts(path);
        return;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(file_size + 1);
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    buffer[bytes_read] = '\0';

    fclose(file);
    run((String){.buffer = buffer, .length = file_size});
    free(buffer);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        const char *path = argv[1];
        run_file(path);
    } else {
        puts("Clam Interpreter:\n");
        repl();
    }
    return 0;
}
