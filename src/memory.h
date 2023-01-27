#ifndef LLAMBDA_MEMORY_H
#define LLAMBDA_MEMORY_H

#include <stdlib.h>

#define ALLOCATE(type, count) (type *)reallocate(NULL, sizeof(type) * (count))

#define FREE(type, pointer) (type *)reallocate(pointer, 0)

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity)*2)

#define GROW_ARRAY(type, pointer, new_count)                                   \
    (type *)reallocate(pointer, sizeof(type) * (new_count))

void *reallocate(void *pointer, size_t new_size);

#endif
