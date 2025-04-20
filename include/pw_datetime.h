#pragma once

#include <pw_types.h>

#ifdef __cplusplus
extern "C" {
#endif

PwResult pw_monotonic();
/*
 * Return current timestamp.
 */

PwResult pw_timestamp_sum(PwValuePtr a, PwValuePtr b);
/*
 * Calculate a + b
 */

PwResult pw_timestamp_diff(PwValuePtr a, PwValuePtr b);
/*
 * Calculate  a - b
 */

#ifdef __cplusplus
}
#endif
