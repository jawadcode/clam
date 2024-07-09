#ifndef CLAM_COMMON_H
#define CLAM_COMMON_H

#include <stdbool.h>

#define DEBUG_MODE

// I hate this language, all of its compilers and all of their stupid
// idiosyncrasies, anyways, this expression should signal to the MSVC, GCC and
// Clang optimisers that the branch is unreachable, and should trigger a runtime
// error when AddressSanitizer is enabled.
#if defined(__GNUC__) // Includes clang as most versions support GNU C
                      // extensions
#define UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
#define UNREACHABLE __assume(0)
#else
// I have no idea if this does anything meaningful in other compilers but it was
// the only portable substitute I could find, this stuff should really be
// standardised.
#define UNREACHABLE ((void)0)
#endif

#ifdef DEBUG_MODE
#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define ASSERT(assertion, message)                                             \
    do {                                                                       \
        if (!(assertion)) {                                                    \
            puts("\x1b[31;1mAssertion Failed\x1b[0m @\x1b[37;3m " __FILE__     \
                 ":" STRINGIZE(__LINE__) "\x1b[0m => '" STRINGIZE(assertion) "'\n    info: \"" message "\"");               \
            exit(1);                                                           \
        }                                                                      \
    } while (0)
#define ASSERT_CALLEE(assertion, message, file, line)                          \
    do {                                                                       \
        if (!(assertion)) {                                                    \
            printf("\x1b[31;1mAssertion Failed\x1b[0m @\x1b[37;3m "            \
                   "%s:%d\x1b[0m => '" STRINGIZE(assertion) "'\n    info: \"" message "\"", file, line); \
            exit(1);                                                           \
        }                                                                      \
    } while (0)
#else
#define ASSERT(assertion, message)                                             \
    do {                                                                       \
        if (!(assertion)) {                                                    \
            UNREACHABLE;                                                       \
        }                                                                      \
    } while (0)
#endif

typedef struct {
    size_t start;
    size_t end;
} Span;

#endif
