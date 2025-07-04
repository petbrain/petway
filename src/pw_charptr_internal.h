#pragma once

#include "include/pw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern PwType _pw_charptr_type;

[[nodiscard]] bool pw_charptr_to_string(PwValuePtr self, PwValuePtr result);
[[nodiscard]] bool pw_charptr_to_string_inplace(PwValuePtr v);
/*
 * If `v` is CharPtr, convert it to String in place.
 * Return false if OOM.
 */

[[nodiscard]] unsigned _pw_charptr_strlen2(PwValuePtr charptr, uint8_t* char_size);

[[nodiscard]] bool _pw_charptr_equal_string(PwValuePtr charptr, PwValuePtr str);
// comparison helper

#ifdef __cplusplus
}
#endif
