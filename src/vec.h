#ifndef CLAM_VEC_H
#define CLAM_VEC_H

#include <stddef.h>

#include "memory.h"

#define CREATE_VEC(T, Name)                                                    \
    typedef struct Name {                                                      \
        T *buffer;                                                             \
        size_t capacity;                                                       \
        size_t length;                                                         \
    } Name;

#define VEC_NEW_SIG(T, Name) Name Name##_new(void);
#define VEC_NEW(T, Name)                                                       \
    Name Name##_new(void) {                                                    \
        return (Name){                                                         \
            .buffer = NULL,                                                    \
            .capacity = 0,                                                     \
            .length = 0,                                                       \
        };                                                                     \
    }

#define VEC_WITH_CAP_SIG(T, Name) Name Name##_with_capacity(size_t capacity);
#define VEC_WITH_CAP(T, Name)                                                  \
    Name Name##_with_capacity(size_t capacity) {                               \
        return (Name){.buffer = (T *)reallocate(NULL, sizeof(T) * capacity),   \
                      .length = 0,                                             \
                      .capacity = capacity};                                   \
    }

#define VEC_PUSH_SIG(T, Name) size_t Name##_push(Name *vec, T value);
#define VEC_PUSH(T, Name)                                                      \
    size_t Name##_push(Name *vec, T value) {                                   \
        if (vec->capacity < vec->length + 1) {                                 \
            size_t old_capacity = vec->capacity;                               \
            vec->capacity = grow_allocation(old_capacity);                     \
            vec->buffer =                                                      \
                (T *)reallocate(vec->buffer, sizeof(T) * vec->capacity);       \
        }                                                                      \
                                                                               \
        vec->buffer[vec->length] = value;                                      \
        return vec->length++;                                                  \
    }

#define VEC_FREE_SIG(T, Name) void Name##_free(Name *vec);
#define VEC_FREE(T, Name)                                                      \
    void Name##_free(Name *vec) {                                              \
        free(vec->buffer);                                                     \
        vec->buffer = NULL;                                                    \
        vec->capacity = 0;                                                     \
        vec->length = 0;                                                       \
    }

#define DECL_VEC_HEADER(T, Name)                                               \
    CREATE_VEC(T, Name)                                                        \
    VEC_NEW_SIG(T, Name) VEC_PUSH_SIG(T, Name) VEC_FREE_SIG(T, Name)

#define DEF_VEC_T(T, Name)                                                     \
    CREATE_VEC(T, Name)                                                        \
    static VEC_NEW(T, Name) static VEC_PUSH(T, Name) static VEC_FREE(T, Name)

#define DEF_VEC(T, Name) VEC_NEW(T, Name) VEC_PUSH(T, Name) VEC_FREE(T, Name)

#endif
