#pragma once

#include <pw_types.h>

#ifdef __cplusplus
extern "C" {
#endif

[[nodiscard]] bool pw_monotonic(PwValuePtr result);
/*
 * Return current timestamp.
 */

[[nodiscard]] _PwValue pw_timestamp_sum(PwValuePtr a, PwValuePtr b);
/*
 * Calculate a + b
 */

[[nodiscard]] _PwValue pw_timestamp_diff(PwValuePtr a, PwValuePtr b);
/*
 * Calculate  a - b
 */

#ifdef __cplusplus
}
#endif
