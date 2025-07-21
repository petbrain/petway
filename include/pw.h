#pragma once

/*
 * Single include file.
 */

#include <pw_types.h>
#include <pw_interfaces.h>

#include <pw_args.h>
#include <pw_dump.h>
#include <pw_hash.h>
#include <pw_status.h>

#include <pw_array.h>
#include <pw_datetime.h>
#include <pw_iterator.h>
#include <pw_map.h>
#include <pw_string.h>
#include <pw_file.h>
#include <pw_string_io.h>
#include <pw_utf.h>


/*
 * Miscellaneous helpers
 */

#define pwva(func, ...)  \
    ({  \
        _PwValue result = PW_NULL;  \
        if (!(func)(__VA_ARGS__ __VA_OPT__(,) &result)) {  \
            pw_clone2(&current_task->status, &result);  \
        }  \
        result;  \
    })
/*
 * Call `func` and return its result on success or current_task->status on error.
 */

#define pw_get(result, container, ...) _pw_get((result), (container) __VA_OPT__(,) __VA_ARGS__, nullptr)
bool _pw_get(PwValuePtr result, PwValuePtr container, ...);

#define pw_set(value, container, ...) _pw_set((value), (container) __VA_OPT__(,) __VA_ARGS__, nullptr)
bool _pw_set(PwValuePtr value, PwValuePtr container, ...);
/*
 * Get value from container object / set value.
 * Variadic arguments are path to value and all must have char* type because
 * there's no simple way to distinguish types of variadic args.
 * For maps arguments are used as keys in UTF-8 encoding.
 * For lists arguments are converted to integer index.
 */

bool pw_read_environment(PwValuePtr result);
/*
 * Return map containing environment variables.
 */
