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


/*
 * Miscellaneous helpers
 */

#define pw_get(container, ...) _pw_get((container) __VA_OPT__(,) __VA_ARGS__, nullptr)
PwResult _pw_get(PwValuePtr container, ...);
/*
 * Get value from container object.
 * Variadic arguments are path to value and all must have char* type because
 * there's no simple way to distinguish types of variadic args.
 * For maps arguments are used as keys in UTF-8 encoding.
 * For lists arguments are converted to integer index.
 */
