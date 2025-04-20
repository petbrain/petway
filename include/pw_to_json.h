#pragma once

#include <pw.h>

#ifdef __cplusplus
extern "C" {
#endif

PwResult pw_to_json(PwValuePtr value, unsigned indent);
/*
 * Convert `value` to JSON string.
 *
 * If `indent` is nonzero, the result is formatted with indentation.
 */

#ifdef __cplusplus
}
#endif
