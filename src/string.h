#ifndef CLAM_STRING_H
#define CLAM_STRING_H

#include <stdbool.h>
#include <stdio.h>

#include "vec.h"

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

void StringBuf_push_string(StringBuf *dest_buf, String src_str);

void StringBuf_print(StringBuf buf);

#endif
