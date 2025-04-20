#pragma once

/*
 * This file contains both function argument helpers
 * and command line arguments parsing API.
 */

#include <stdarg.h>

#include <pw_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 * Function argument helpers
 */

/*
 * Argument validation macro
 */
#define pw_expect(value_type, arg)  \
    do {  \
        if (!pw_is_subtype(_Generic((arg),  \
                _PwValue:   &(arg), \
                PwValuePtr: (arg)   \
            ), PwTypeId_##value_type)) {  \
            if (pw_error(_Generic((arg),  \
                    _PwValue:   &(arg), \
                    PwValuePtr: (arg)))) {  \
                return _Generic((arg),  \
                    _PwValue:   pw_move, \
                    PwValuePtr: pw_clone  \
                )(_Generic((arg),  \
                    _PwValue:   &(arg), \
                    PwValuePtr: arg  \
                ));  \
            }  \
            return PwError(PW_ERROR_INCOMPATIBLE_TYPE);  \
        }  \
    } while (false)


static inline void _pw_destroy_args(va_list ap)
/*
 * Helper for functions that accept values created on stack during function call.
 * Such values cannot be automatically cleaned on error, the callee
 * should do that.
 * See PwArray(), pw_array_append_va, PwMap(), pw_map_update_va
 */
{
    for (;;) {{
        PwValue arg = va_arg(ap, _PwValue);
        if (pw_va_end(&arg)) {
            break;
        }
    }}
}

/****************************************************************
 * Command line arguments parsing
 */

PwResult pw_parse_kvargs(int argc, char* argv[]);
/*
 * The function returns a mapping.
 *
 * The encoding for arguments is assumed to be UTF-8.
 *
 * Arguments starting from 1 are expected in the form of key=value.
 * If an argument contains no '=', it goes to key and the value is set to null.
 *
 * argv[0] is added to the mapping as is under key 0.
 */

#ifdef __cplusplus
}
#endif
