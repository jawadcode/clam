#ifndef LLAMBDA_VEC_H
#define LLAMBDA_VEC_H

#include "common.h"
#include "memory.h"

#define create_vec(T)                                                          \
    typedef struct T##Vec {                                                    \
        T *buffer;                                                             \
        size_t capacity;                                                       \
        size_t length;                                                         \
    } T##Vec;

#define vec_new_sig(T) T##Vec T##Vec_new();
#define vec_new(T)                                                             \
    T##Vec T##Vec_new() {                                                      \
        return (T##Vec){                                                       \
            .buffer = NULL,                                                    \
            .capacity = 0,                                                     \
            .length = 0,                                                       \
        };                                                                     \
    }

#define vec_push_sig(T) size_t T##Vec_push(T##Vec *array, T value);
#define vec_push(T)                                                            \
    size_t T##Vec_push(T##Vec *array, T value) {                               \
        if (array->capacity < array->length + 1) {                             \
            int old_capacity = array->capacity;                                \
            array->capacity = GROW_CAPACITY(old_capacity);                     \
            array->buffer = GROW_ARRAY(T, array->buffer, array->capacity);     \
        }                                                                      \
                                                                               \
        array->buffer[array->length] = value;                                  \
        return array->length++;                                                \
    }

#define vec_free_sig(T) void T##Vec_free(T##Vec *array);
#define vec_free(T)                                                            \
    void T##Vec_free(T##Vec *array) {                                          \
        FREE_ARRAY(T, array->buffer);                                          \
        array->buffer = NULL;                                                  \
        array->capacity = 0;                                                   \
        array->length = 0;                                                     \
    }

#define VecHeader(T)                                                           \
    create_vec(T) vec_new_sig(T) vec_push_sig(T) vec_free_sig(T)

#define Vec(T) create_vec(T) vec_new(T) vec_push(T) vec_free(T)

#define VecImpl(T) vec_new(T) vec_push(T) vec_free(T)

#endif
