#ifndef LLAMBDA_RESULT_H
#define LLAMBDA_RESULT_H

#include <stdio.h>

typedef enum {
    RESULT_OK,
    RESULT_ERR,
} ResultTag;

#define DEF_RESULT(T, E, name)                                                 \
    typedef struct {                                                           \
        ResultTag tag;                                                         \
        union name##Union {                                                    \
            T ok;                                                              \
            E err;                                                             \
        } value;                                                               \
    } name##Result

#define RET_ERR(ResultType, result, ReturnType)                                \
    do {                                                                       \
        ResultType r = result;                                                 \
        if (r.tag == RESULT_ERR)                                               \
            return (ReturnType){RESULT_ERR, {.err = r.value.err}};             \
    } while (0)

#define RET_ERR_ASSIGN(ident, result, ResultType, ReturnType)                  \
    do {                                                                       \
        ResultType r = result;                                                 \
        if (r.tag == RESULT_ERR)                                               \
            return (ReturnType){RESULT_ERR, {.err = r.value.err}};             \
        else                                                                   \
            ident = r.value.ok;                                                \
    } while (0)

#define RET_OK(ResultType, result, ReturnType)                                 \
    do {                                                                       \
        ResultType r = result;                                                 \
        if (r.tag == RESULT_ERR)                                               \
            return (ReturnType){RESULT_OK, {.ok = r.value.ok}};                \
    } while (0)

#endif
