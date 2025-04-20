#pragma once

/*
 * Array internals.
 */

#include "include/pw_types.h"
#include "src/pw_compound_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

// the following constants must be power of two:
#define PWARRAY_INITIAL_CAPACITY    4
#define PWARRAY_CAPACITY_INCREMENT  16

typedef struct {
    PwValuePtr items;
    unsigned length;
    unsigned capacity;
    unsigned itercount;  // number of iterations in progress
} _PwArray;

#define get_array_data_ptr(value)  ((_PwArray*) _pw_get_data_ptr((value), PwTypeId_Array))

extern PwType _pw_array_type;

/****************************************************************
 * Helpers
 */

static inline unsigned _pw_array_length(_PwArray* array_data)
{
    return array_data->length;
}

static inline unsigned _pw_array_capacity(_PwArray* array_data)
{
    return array_data->capacity;
}

PwResult _pw_alloc_array(PwTypeId type_id, _PwArray* array_data, unsigned capacity);
/*
 * - allocate array items
 * - set array->length = 0
 * - set array->capacity = rounded capacity
 *
 * Return status.
 */

PwResult _pw_array_resize(PwTypeId type_id, _PwArray* array_data, unsigned desired_capacity);
/*
 * Reallocate array.
 */

void _pw_destroy_array(PwTypeId type_id, _PwArray* array_data, PwValuePtr parent);
/*
 * Call destructor for all items and free the array items.
 * For compound values call _pw_abandon before the destructor.
 */

bool _pw_array_eq(_PwArray* a, _PwArray* b);
/*
 * Compare for equality.
 */

PwResult _pw_array_append_item(PwTypeId type_id, _PwArray* array_data, PwValuePtr item, PwValuePtr parent);
/*
 * Append: move `item` on the array using pw_move() and call _pw_embrace(parent, item)
 */

PwResult _pw_array_pop(_PwArray* array_data);
/*
 * Pop item from array.
 */

void _pw_array_del(_PwArray* array_data, unsigned start_index, unsigned end_index);
/*
 * Delete items from array.
 */

#ifdef __cplusplus
}
#endif
