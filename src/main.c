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

#define MATCH_REST(cmd, rest) strcmp((cmd) + 1, rest "\n") == 0

void print_help(void) {
    puts("Commands:\n"
         "  :exit - Exit the REPL\n"
         "  :help - Display this help message\n"
         "  :quit - Quit the REPL\n");
}

void run_cmd(const char *cmd) {
    switch (cmd[0]) {
    case 'e':
        if (MATCH_REST(cmd, "xit")) {
            puts("Bye bye...\n");
            exit(0);
        } else
            goto UNKNOWN_COMMAND;
    case 'h':
        if (MATCH_REST(cmd, "elp")) {
            print_help();
            break;
        } else
            goto UNKNOWN_COMMAND;
    case 'q':
        if (MATCH_REST(cmd, "uit")) {
            puts("Bye bye...\n");
            exit(0);
        } else
            goto UNKNOWN_COMMAND;
    UNKNOWN_COMMAND:
    default:
        fputs("Error: Unknown command ", stderr);
        puts(cmd);
        print_help();
        break;
    }
}

void run(const char *source) {
    Parser parser = new_parser("stdin", source);

    /*  puts("\nLexer Output:");
        while (true) {
            Token token = next_token(&parser.lexer);
            print_token(source, token);
            if (token.kind == TK_EOF)
                break;
        }

        // Rewind the lexer
        parser.lexer = new_lexer(source); */
    ParseResult result = parse_expr(&parser);
    switch (result.tag) {
    case RESULT_OK: {
        StringBuf sexpr = format_ast(&parser.ast_arena, result.value.ok);
        puts("\nParser Output:");
        StringBuf_print(&sexpr);
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

void repl(void) {
    char *buffer;
    size_t length;
    ssize_t len_chars;

    while (true) {
        fputs("$ ", stdout);
        fflush(stdout);
        buffer = NULL;
        len_chars = getline(&buffer, &length, stdin);
        if (len_chars >= 1 && buffer[0] == ':') {
            run_cmd(buffer + 1);
        } else {
            run(buffer);
        }
        free(buffer);
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
    run(buffer);
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
