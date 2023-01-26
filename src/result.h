#ifndef LLAMBDA_RESULT_H
#define LLAMBDA_RESULT_H

#include <stdio.h>

typedef enum {
    RESULT_OK,
    RESULT_ERR,
} ResultTag;

#define CreateResult(T, E, name)                                               \
    typedef struct {                                                           \
        ResultTag tag;                                                         \
        union name##Union {                                                    \
            T ok;                                                              \
            E err;                                                             \
        } value;                                                               \
    } name##Result

#define RetErr(ResultType, result, ReturnType)                                 \
    do {                                                                       \
        ResultType r = result;                                                 \
        if (r.tag == RESULT_ERR)                                               \
            return (ReturnType){RESULT_ERR, {.err = r.value.err}};             \
    } while (0)

#define RetErrAssign(ident, result, ResultType, ReturnType)                    \
    do {                                                                       \
        ResultType r = result;                                                 \
        if (r.tag == RESULT_ERR)                                               \
            return (ReturnType){RESULT_ERR, {.err = r.value.err}};             \
        else                                                                   \
            ident = r.value.ok;                                                \
    } while (0)

#define RetOk(ResultType, result, ReturnType)                                  \
    do {                                                                       \
        ResultType r = result;                                                 \
        if (r.tag == RESULT_ERR)                                               \
            return (ReturnType){RESULT_OK, {.ok = r.value.ok}};                \
    } while (0)

#endif
