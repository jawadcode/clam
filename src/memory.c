#include "memory.h"
#include <stdio.h>

void *reallocate(void *pointer, size_t new_size) {
    // `realloc` isn't guaranteed to free the pointer if the supplied size is 0
    if (new_size == 0) {
        free(pointer);
        return NULL;
    }

    void *result = realloc(pointer, new_size);
    if (result == NULL) {
        puts("Fatal Error: Out of Memory");
        exit(1);
    }
    return result;
}

// Hopefully this will be inlined
size_t grow_allocation(size_t old_capacity) {
    return old_capacity < 8 ? 8 : old_capacity * 2;
}
