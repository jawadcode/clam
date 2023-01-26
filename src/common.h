#ifndef LLAMBDA_COMMON_H
#define LLAMBDA_COMMON_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG_MODE

#define ASSERT(assertion, message)                                             \
    _assert(assertion, "Assertion failed: " message)

static inline void _assert(bool assertion, const char *message) {
#ifdef DEBUG_MODE
    if (!assertion) {
        puts(message);
        exit(1);
    }
#endif
}

// A string which consists of a 'buffer' which is not null-terminated and its
// length in bytes
typedef struct {
    const char *buffer;
    size_t length;
} String;

#define STR(x)                                                                 \
    (String) { .buffer = x, .length = sizeof(x) - 1 }

void String_print(String string);

typedef struct {
    size_t start;
    size_t end;
} Span;

#endif
