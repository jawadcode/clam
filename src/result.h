#ifndef CLAM_RESULT_H
#define CLAM_RESULT_H

#include <stdio.h>

typedef enum {
    RESULT_OK,
    RESULT_ERR,
} ResultTag;

#define DEF_RESULT(T, E, Name)                                                 \
    typedef struct Name##Result {                                              \
        ResultTag tag;                                                         \
        union {                                                                \
            T ok;                                                              \
            E err;                                                             \
        } value;                                                               \
    } Name##Result

#define RET_ERR(ResultType, result)                                            \
    do {                                                                       \
        ResultType res = result;                                               \
        if (res.tag == RESULT_ERR) {                                           \
            error = res.value.err;                                             \
            goto FAILURE;                                                      \
        }                                                                      \
    } while (0)

#define RET_ERR_ASSIGN(ident, ResultType, result)                              \
    do {                                                                       \
        ResultType res = result;                                               \
        if (res.tag == RESULT_ERR) {                                           \
            error = res.value.err;                                             \
            goto FAILURE;                                                      \
        } else                                                                 \
            ident = res.value.ok;                                              \
    } while (0)

#define RET_OK(ResultType, result)                                             \
    do {                                                                       \
        ResultType res = result;                                               \
        if (res.tag == RESULT_ERR) {                                           \
            error = res.value.ok;                                              \
            goto FAILURE;                                                      \
        }                                                                      \
    } while (0)

#endif
