#ifndef CLAM_MEMORY_H
#define CLAM_MEMORY_H

#include <stdlib.h>

void *reallocate(void *pointer, size_t new_size);

size_t grow_allocation(size_t old_capacity);

#endif
