#ifndef CLAM_COMMON_H
#define CLAM_COMMON_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "vec.h"

#define DEBUG_MODE

#ifdef DEBUG_MODE
#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define ASSERT(assertion, message)                                             \
    do {                                                                       \
        if (!(assertion)) {                                                    \
            puts("\x1b[31;1mAssertion Failed\x1b[0m \x1b[37m@ " __FILE__       \
                 ":" STRINGIZE(__LINE__) "\x1b[0m = \"" message "\"");         \
            exit(1);                                                           \
        }                                                                      \
    } while (0)
#else
#define ASSERT(assertion, message) (void)0
#endif

// I hate this language, all of its compilers and all of their stupid
// idiosyncrasies
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

// A string which consists of a 'buffer' which is not necessarily
// null-terminated and its length in bytes
typedef struct {
    const char *buffer;
    size_t length;
} String;

// ONLY WORKS FOR STRING LITERALS
#define STR(x)                                                                 \
    (String) { .buffer = (x), .length = sizeof(x) - 1 }

void String_print(String string);

void String_write(String string, FILE *file);

bool String_eq(String a, String b);

DECL_VEC_HEADER(char, StringBuf)

VEC_WITH_CAP_SIG(char, StringBuf)

#define BUF_TO_STR(x)                                                          \
    (String) { .buffer = (x).buffer, .length = (x).length }

void StringBuf_push_string(StringBuf *buf, String string);

void StringBuf_print(StringBuf buf);

typedef struct {
    size_t start;
    size_t end;
} Span;

#endif
