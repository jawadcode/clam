#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "vec.h"

#define META_CMD(cmd, rest, fallback_label, ...)                               \
    if (strcmp(cmd + 1, rest "\n") == 0) {                                     \
        __VA_ARGS__                                                            \
        break;                                                                 \
    } else {                                                                   \
        goto fallback_label;                                                   \
    }

void print_help() {
    puts("Commands:\n"
         "  :exit - Exit the REPL\n"
         "  :help - Display this help message\n"
         "  :quit - Quit the REPL\n");
}

void run_cmd(const char *cmd) {
    switch (cmd[0]) {
    case 'e':
        META_CMD(cmd, "xit", UNKNOWN_COMMAND, puts("Bye bye...\n"), exit(0);)
    case 'h':
        META_CMD(cmd, "elp", UNKNOWN_COMMAND, print_help();)
    case 'q':
        META_CMD(cmd, "uit", UNKNOWN_COMMAND, puts("Bye bye...\n"), exit(0);)
    UNKNOWN_COMMAND:
    default:
        fputs("Error: Unknown command ", stderr);
        puts(cmd);
        print_help();
        break;
    }
}

void run(const char *source) {
    Parser parser = new_parser(source);

    puts("Lexer output:");
    while (true) {
        Token token = next_token(&parser.lexer);
        print_token(source, token);
        if (token.kind == TK_EOF)
            break;
    }

    // Rewind the lexer
    parser.lexer = new_lexer(source);
    ParseResult result = parse_expr(&parser);
    switch (result.tag) {
    case RESULT_OK:
        puts("\nParser output:");
        String_print(format_ast(&parser));
        puts("");
        break;
    case RESULT_ERR: {
        SyntaxError error = result.value.err;
        switch (error.tag) {
        case ERROR_INVALID_ESC_SEQ: {
            SyntaxError_InvalidEscSeq err = error.error.invalid_esc_seq;
            puts("SyntaxError: Invalid escape sequence\nstring:");
            String_print((String){
                .buffer = source + err.string.start,
                .length = err.string.end - err.string.start,
            });
            puts("\nescape sequence:");
            String_print((String){
                .buffer = source + err.escape_sequence.start,
                .length = err.string.end - err.string.start,
            });
            break;
        }
        case ERROR_UNEXPECTED_TOKEN: {
            SyntaxError_UnexpectedToken err = error.error.unexpected_token;
            puts("SyntaxError: Unexpected token\nexpected:");
            puts(err.expected.buffer);
            puts("got:");
            print_token(source, err.got);
            break;
        }
        }
        break;
    }
    }
}

void repl() {
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
        puts("Llambda Interpreter:\n");
        repl();
    }
    return 0;
}
