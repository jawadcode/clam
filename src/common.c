#include "common.h"

void String_print(String string) {
    fwrite(string.buffer, 1, (int)string.length, stdout);
}
