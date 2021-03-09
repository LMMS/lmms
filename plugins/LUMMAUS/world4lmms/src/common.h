#ifndef _H_COMMON_
#define _H_COMMON_

#define max(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a > _b ? _a : _b;      \
    })

#define min(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a < _b ? _a : _b;      \
    })

#define SAFE_FREE(a) \
    if (a)           \
    {                \
        free(a);     \
        a = NULL;    \
    }

#endif