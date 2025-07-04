#pragma once

#include "include/pw.h"

#ifdef __cplusplus
extern "C" {
#endif

extern PwType _pw_struct_type;

[[nodiscard]] bool _pw_struct_alloc(PwValuePtr self, void* ctor_args);
/*
 * Allocate struct_data and call init method.
 */

void _pw_struct_release(PwValuePtr self);
/*
 * Call init method, release struct_data and reset type of self to Null.
 */


/*
 * methods for use in statically defined descendant types
 */
[[nodiscard]] bool _pw_struct_create(PwTypeId type_id, void* ctor_args, PwValuePtr result);
              void _pw_struct_destroy(PwValuePtr self);
              void _pw_struct_clone(PwValuePtr self);
              void _pw_struct_hash(PwValuePtr self, PwHashContext* ctx);
[[nodiscard]] bool _pw_struct_deepcopy(PwValuePtr self, PwValuePtr result);
[[nodiscard]] bool _pw_struct_to_string(PwValuePtr self, PwValuePtr result);
[[nodiscard]] bool _pw_struct_is_true(PwValuePtr self);
[[nodiscard]] bool _pw_struct_equal_sametype(PwValuePtr self, PwValuePtr other);
[[nodiscard]] bool _pw_struct_equal(PwValuePtr self, PwValuePtr other);

#ifdef __cplusplus
}
#endif
