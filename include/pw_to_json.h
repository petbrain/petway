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

PwResult pw_to_json_file(PwValuePtr value, unsigned indent, PwValuePtr file);
/*
 * Convert `value` to JSON and write to `file` in UTF-8 encoding.
 *
 * If `indent` is nonzero, the result is formatted with indentation.
 *
 * Return status.
 */

#ifdef __cplusplus
}
#endif
