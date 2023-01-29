#ifndef LLAMBDA_VEC_H
#define LLAMBDA_VEC_H

#include "memory.h"

#define CREATE_VEC(T, Name)                                                    \
    typedef struct {                                                           \
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

#define VEC_PUSH_SIG(T, Name) size_t Name##_push(Name *array, T value);
#define VEC_PUSH(T, Name)                                                      \
    size_t Name##_push(Name *array, T value) {                                 \
        if (array->capacity < array->length + 1) {                             \
            int old_capacity = array->capacity;                                \
            array->capacity = old_capacity < 8 ? 8 : old_capacity * 2;         \
            array->buffer =                                                    \
                (T *)reallocate(array->buffer, sizeof(T) * array->capacity);   \
        }                                                                      \
                                                                               \
        array->buffer[array->length] = value;                                  \
        return array->length++;                                                \
    }

#define VEC_FREE_SIG(T, Name) void Name##_free(Name *array);
#define VEC_FREE(T, Name)                                                      \
    void Name##_free(Name *array) {                                            \
        free(array->buffer);                                                   \
        array->buffer = NULL;                                                  \
        array->capacity = 0;                                                   \
        array->length = 0;                                                     \
    }

#define DECL_VEC_HEADER(T, Name)                                               \
    CREATE_VEC(T, Name)                                                        \
    VEC_NEW_SIG(T, Name) VEC_PUSH_SIG(T, Name) VEC_FREE_SIG(T, Name)

#define DEF_VEC_T(T, Name)                                                     \
    CREATE_VEC(T, Name) VEC_NEW(T, Name) VEC_PUSH(T, Name) VEC_FREE(T, Name)

#define DEF_VEC(T, Name) VEC_NEW(T, Name) VEC_PUSH(T, Name) VEC_FREE(T, Name)

#endif
