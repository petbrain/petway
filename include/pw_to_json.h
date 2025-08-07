#pragma once

#include <pw.h>

#ifdef __cplusplus
extern "C" {
#endif

[[nodiscard]] bool pw_to_json(PwValuePtr value, unsigned indent, PwValuePtr result);
/*
 * Convert `value` to JSON.
 *
 * If `result` is Null, a string is created for it.
 * Otherwise, it should provide Append interface.
 *
 * If `indent` is nonzero, the result is formatted with indentation.
 */

#ifdef __cplusplus
}
#endif
