#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 * Assertions and panic
 *
 * Unlike assertions from standard library they cannot be
 * turned off and can be used for input parameters validation.
 */

#define pw_assert(condition) \
    __extension__  \
    ({  \
        if (_pw_unlikely( !(condition) )) {  \
            pw_panic("PW assertion failed at %s:%s:%d: " #condition "\n", __FILE__, __func__, __LINE__);  \
        }  \
    })

[[noreturn]]
void pw_panic(char* fmt, ...);

#ifdef __cplusplus
}
#endif
