#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define PW_LENGTH(array)  (sizeof(array) / sizeof((array)[0]))
/*
 * Get array length
 */

#define _pw_likely(x)    __builtin_expect(!!(x), 1)
#define _pw_unlikely(x)  __builtin_expect(!!(x), 0)
/*
 * Branch optimization hints
 */

#ifdef __cplusplus
}
#endif
