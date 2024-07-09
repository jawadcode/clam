#ifndef CLAM_MAYBE_H
#define CLAM_MAYBE_H

#include "common.h"

typedef enum MaybeTag {
    MAYBE_SOME,
    MAYBE_NONE,
} MaybeTag;

#define CREATE_MAYBE(T, Name)                                                  \
    typedef struct Name {                                                      \
        MaybeTag tag;                                                          \
        T some;                                                                \
    } Name

#define MAYBE_UNWRAP_SIG(T, Name)                                              \
    T Name##_unwrap(Name maybe, const char *file, const int line)

#define MAYBE_UNWRAP_DEF(T, Name)                                              \
    T Name##_unwrap(Name maybe, const char *file, const int line) {            \
        ASSERT_CALLEE(maybe.tag == MAYBE_SOME, "Unwrapped an empty Maybe",     \
                      file, line);                                             \
        return maybe.some;                                                     \
    }

#define UNWRAP(Name, x) Name##_unwrap(x, __FILE__, __LINE__)

#endif
