#pragma once

#include <pw_types.h>

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
    ({  \
        if (_pw_unlikely( !(condition) )) {  \
            pw_panic("PW assertion failed at %s:%s:%d: " #condition "\n", __FILE__, __func__, __LINE__);  \
        }  \
    })

[[noreturn]]
void pw_panic(char* fmt, ...);

[[noreturn]]
void _pw_panic_bad_charptr_subtype(PwValuePtr v);
/*
 * Implemented in src/pw_charptr.c
 */

[[ noreturn ]]
void _pw_panic_no_interface(PwTypeId type_id, unsigned interface_id);
/*
 * Implemented in src/pw_interfaces.c
 */

[[ noreturn ]]
void _pw_panic_bad_char_size(uint8_t char_size);
/*
 * Implemented in src/pw_string.c
 */

#ifdef __cplusplus
}
#endif
