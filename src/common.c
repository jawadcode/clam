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

void StringBuf_push_string(StringBuf *buf, String string) {
    size_t old_length = buf->length;
    size_t new_length = buf->length + string.length;
    buf->length = new_length;
    if (new_length > buf->capacity) {
#if __has_builtin(__builtin_clz)
        buf->capacity = 1 << (32 - __builtin_clz(new_length - 1));
#else
#error "Clam requires '__builtin_clz' support"
#endif
        buf->buffer =
            (char *)reallocate(buf->buffer, sizeof(char) * buf->capacity);
    }
    memcpy(buf->buffer + old_length, string.buffer, string.length);
}

void StringBuf_print(StringBuf string) {
    fwrite(string.buffer, sizeof(char), string.length, stdout);
}
