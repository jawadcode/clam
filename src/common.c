#include "common.h"
#include "memory.h"
#include "vec.h"
#include <stdio.h>
#include <string.h>

void String_print(String string) {
    fwrite(string.buffer, sizeof(char), string.length, stdout);
}

void String_write(String string, FILE *file) {
    fwrite(string.buffer, sizeof(char), string.length, file);
}

bool String_eq(String a, String b) {
    if (a.length != b.length)
        return false;
    else
        return memcmp(a.buffer, b.buffer, a.length) == 0;
}

DEF_VEC(char, StringBuf)
VEC_WITH_CAP(char, StringBuf)

void StringBuf_push_string(StringBuf *dest_buf, String src_str) {
    for (size_t idx = 0; idx < src_str.length; idx++) {
        StringBuf_push(dest_buf, src_str.buffer[idx]);
    }
}

void StringBuf_print(StringBuf string) {
    fwrite(string.buffer, sizeof(char), string.length, stdout);
}
