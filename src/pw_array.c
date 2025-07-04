#include <limits.h>
#include <string.h>

#include "include/pw.h"
#include "include/pw_parse.h"
#include "src/pw_alloc.h"
#include "src/pw_charptr_internal.h"
#include "src/pw_array_internal.h"
#include "src/pw_string_internal.h"
#include "src/pw_struct_internal.h"

static void array_fini(PwValuePtr self)
{
    _PwArray* array = get_array_struct_ptr(self);
    pw_assert(array->itercount == 0);
    _pw_destroy_array(self->type_id, array, self);
}

[[nodiscard]] static bool array_init(PwValuePtr self, void* ctor_args)
{
    // XXX not using ctor_args for now
    // XXX array constructor arg: initial size

    if (_pw_alloc_array(self->type_id, get_array_struct_ptr(self), PWARRAY_INITIAL_CAPACITY)) {
        return true;
    } else {
        array_fini(self);
        return false;
    }
}

static void array_hash(PwValuePtr self, PwHashContext* ctx)
{
    _pw_hash_uint64(ctx, self->type_id);
    _PwArray* array = get_array_struct_ptr(self);
    PwValuePtr item_ptr = array->items;
    for (unsigned n = array->length; n; n--, item_ptr++) {
        _pw_call_hash(item_ptr, ctx);
    }
}

[[nodiscard]] static bool array_deepcopy(PwValuePtr self, PwValuePtr result)
{
    if (!pw_create(self->type_id, result)) {
        return false;
    }

    _PwArray* src_array = get_array_struct_ptr(self);
    _PwArray* dest_array = get_array_struct_ptr(result);

    if (!_pw_array_resize(self->type_id, dest_array, src_array->length)) {
        pw_destroy(result);
        return false;
    }

    PwValuePtr src_item_ptr = src_array->items;
    PwValuePtr dest_item_ptr = dest_array->items;
    for (unsigned i = 0; i < src_array->length; i++) {
        if (!pw_deepcopy(src_item_ptr, dest_item_ptr)) {
            pw_destroy(result);
            return false;
        }
        if (!_pw_embrace(result, dest_item_ptr)) {
            pw_destroy(result);
            return false;
        };
        src_item_ptr++;
        dest_item_ptr++;
        dest_array->length++;
    }
    return true;
}

static void array_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    _pw_dump_struct_data(fp, self);
    _pw_dump_compound_data(fp, self, next_indent);
    _pw_print_indent(fp, next_indent);

    PwValuePtr value_seen = _pw_on_chain(self, tail);
    if (value_seen) {
        fprintf(fp, "already dumped: %p, data=%p\n", value_seen, value_seen->struct_data);
        return;
    }

    _PwCompoundChain this_link = {
        .prev = tail,
        .value = self
    };

    _PwArray* array = get_array_struct_ptr(self);
    fprintf(fp, "%u items, capacity=%u\n", array->length, array->capacity);

    next_indent += 4;
    PwValuePtr item_ptr = array->items;
    for (unsigned n = array->length; n; n--, item_ptr++) {
        _pw_call_dump(fp, item_ptr, next_indent, next_indent, &this_link);
    }
}

[[nodiscard]] static bool array_to_string(PwValuePtr self, PwValuePtr result)
{
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
}

[[nodiscard]] static bool array_is_true(PwValuePtr self)
{
    return get_array_struct_ptr(self)->length;
}

[[nodiscard]] static bool array_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    return _pw_array_eq(get_array_struct_ptr(self), get_array_struct_ptr(other));
}

[[nodiscard]] static bool array_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        if (t == PwTypeId_Array) {
            return _pw_array_eq(get_array_struct_ptr(self), get_array_struct_ptr(other));
        }
        // check base type
        t = _pw_types[t]->ancestor_id;
        if (t == PwTypeId_Null) {
            return false;
        }
    }
}

static PwInterface_RandomAccess random_access_interface;  // forward declaration

static _PwInterface array_interfaces[] = {
    {
        .interface_id      = PwInterfaceId_RandomAccess,
        .interface_methods = (void**) &random_access_interface
    }
    // PwInterfaceId_Array
};

PwType _pw_array_type = {
    .id             = PwTypeId_Array,
    .ancestor_id    = PwTypeId_Compound,
    .name           = "Array",
    .allocator      = &default_allocator,

    .create         = _pw_struct_create,
    .destroy        = _pw_compound_destroy,
    .clone          = _pw_struct_clone,
    .hash           = array_hash,
    .deepcopy       = array_deepcopy,
    .dump           = array_dump,
    .to_string      = array_to_string,
    .is_true        = array_is_true,
    .equal_sametype = array_equal_sametype,
    .equal          = array_equal,

    .data_offset    = sizeof(_PwCompoundData),
    .data_size      = sizeof(_PwArray),

    .init           = array_init,
    .fini           = array_fini,

    .num_interfaces = PW_LENGTH(array_interfaces),
    .interfaces     = array_interfaces
};

// make sure _PwCompoundData has correct padding
static_assert((sizeof(_PwCompoundData) & (alignof(_PwArray) - 1)) == 0);


static unsigned round_capacity(unsigned capacity)
{
    if (capacity <= PWARRAY_CAPACITY_INCREMENT) {
        return align_unsigned(capacity, PWARRAY_INITIAL_CAPACITY);
    } else {
        return align_unsigned(capacity, PWARRAY_CAPACITY_INCREMENT);
    }
}

[[nodiscard]] bool _pw_array_va(PwValuePtr result, ...)
{
    va_list ap;
    va_start(ap);
    if (!pw_create_array(result)) {
        _pw_destroy_args(ap);
        va_end(ap);
        return false;
    }
    bool ret = pw_array_append_ap(result, ap);
    if (!ret) {
        pw_destroy(result);
    }
    va_end(ap);
    return ret;
}

[[nodiscard]] bool _pw_alloc_array(PwTypeId type_id, _PwArray* array, unsigned capacity)
{
    if (capacity >= UINT_MAX / sizeof(_PwValue)) {
        pw_set_status(PwStatus(PW_ERROR_DATA_SIZE_TOO_BIG));
        return false;
    }

    array->length = 0;
    array->capacity = round_capacity(capacity);

    unsigned memsize = array->capacity * sizeof(_PwValue);
    array->items = _pw_alloc(type_id, memsize, true);
    return (bool) array->items;
}

[[nodiscard]] static bool _pw_array_pop(PwValuePtr array_value, PwValuePtr result)
{
    _PwArray* array = get_array_struct_ptr(array_value);
    if (array->length == 0) {
        pw_set_status(PwStatus(PW_ERROR_EXTRACT_FROM_EMPTY_ARRAY));
        return false;
    }
    array->length--;
    PwValuePtr item_ptr = &array->items[array->length];
    if (pw_is_compound(item_ptr)) {
        _pw_abandon(array_value, item_ptr);
    }
    pw_destroy(item_ptr);
   return true;
}

static void destroy_items(_PwArray* array, unsigned start_index, unsigned end_index, PwValuePtr parent)
{
    PwValuePtr item_ptr = &array->items[start_index];
    for (unsigned i = start_index; i < end_index; i++, item_ptr++) {
        if (pw_is_compound(item_ptr)) {
            _pw_abandon(parent, item_ptr);
        }
        pw_destroy(item_ptr);
    }
}

void _pw_destroy_array(PwTypeId type_id, _PwArray* array, PwValuePtr parent)
{
    if (array->items) {
        destroy_items(array, 0, array->length, parent);
        unsigned memsize = array->capacity * sizeof(_PwValue);
        _pw_free(type_id, (void**) &array->items, memsize);
    }
}

[[nodiscard]] bool _pw_array_resize(PwTypeId type_id, _PwArray* array, unsigned desired_capacity)
{
    if (desired_capacity < array->length) {
        desired_capacity = array->length;
    } else if (desired_capacity >= UINT_MAX / sizeof(_PwValue)) {
        pw_set_status(PwStatus(PW_ERROR_DATA_SIZE_TOO_BIG));
        return false;
    }
    unsigned new_capacity = round_capacity(desired_capacity);

    unsigned old_memsize = array->capacity * sizeof(_PwValue);
    unsigned new_memsize = new_capacity * sizeof(_PwValue);

    if (!_pw_realloc(type_id, (void**) &array->items, old_memsize, new_memsize, true)) {
        return false;
    }
    array->capacity = new_capacity;
    return true;
}

[[nodiscard]] bool pw_array_resize(PwValuePtr array_value, unsigned desired_capacity)
{
    pw_assert_array(array_value);
    _PwArray* array = get_array_struct_ptr(array_value);
    if (array->itercount) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }
    return _pw_array_resize(array_value->type_id, array, desired_capacity);
}

[[nodiscard]] unsigned pw_array_length(PwValuePtr array_value)
{
    pw_assert_array(array_value);
    return _pw_array_length(get_array_struct_ptr(array_value));
}

[[nodiscard]] bool _pw_array_eq(_PwArray* a, _PwArray* b)
{
    unsigned n = a->length;
    if (b->length != n) {
        // arrays have different lengths
        return false;
    }

    PwValuePtr a_ptr = a->items;
    PwValuePtr b_ptr = b->items;
    while (n) {
        if (!_pw_equal(a_ptr, b_ptr)) {
            return false;
        }
        n--;
        a_ptr++;
        b_ptr++;
    }
    return true;
}

[[nodiscard]] bool _pw_array_append(PwValuePtr array_value, PwValuePtr item)
// XXX this will be an interface method, _pwi_array_append
{
    pw_assert_array(array_value);
    _PwArray* array = get_array_struct_ptr(array_value);
    if (array->itercount) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }
    PwValue cloned_item = pw_clone(item);
    return _pw_array_append_item(array_value->type_id, array, &cloned_item, array_value);
}

[[nodiscard]] static bool grow_array(PwTypeId type_id, _PwArray* array)
{
    pw_assert(array->length <= array->capacity);

    if (array->length == array->capacity) {
        unsigned new_capacity;
        if (array->capacity <= PWARRAY_CAPACITY_INCREMENT) {
            new_capacity = array->capacity + PWARRAY_INITIAL_CAPACITY;
        } else {
            new_capacity = array->capacity + PWARRAY_CAPACITY_INCREMENT;
        }
        if (!_pw_array_resize(type_id, array, new_capacity)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool _pw_array_append_item(PwTypeId type_id, _PwArray* array, PwValuePtr item, PwValuePtr parent)
{
    if (! grow_array(type_id, array)) {
        return false;
    }
    if (!_pw_embrace(parent, item)) {
        return false;
    }
    pw_move(item, &array->items[array->length]);
    array->length++;
    return true;
}

[[nodiscard]] bool _pw_array_append_va(PwValuePtr array, ...)
{
    va_list ap;
    va_start(ap);
    bool ret = pw_array_append_ap(array, ap);
    va_end(ap);
    return ret;
}

[[nodiscard]] bool pw_array_append_ap(PwValuePtr dest, va_list ap)
{
    pw_assert_array(dest);
    _PwArray* array = get_array_struct_ptr(dest);
    if (array->itercount) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }
    PwTypeId type_id = dest->type_id;
    unsigned num_appended = 0;
    for(;;) {{
        PwValue arg = va_arg(ap, _PwValue);
        if (pw_is_status(&arg)) {
            if (pw_is_va_end(&arg)) {
                return true;
            }
            pw_set_status(pw_clone(&arg));
            goto failure;
        }
        if (!pw_charptr_to_string_inplace(&arg)) {
            goto failure;
        }
        if (!_pw_array_append_item(type_id, array, &arg, dest)) {
            goto failure;
        }
        num_appended++;
    }}

failure:
    // rollback
    while (num_appended--) {
        PwValue v = PW_NULL;
        if (_pw_array_pop(dest, &v)) {
            pw_destroy(&v);
        }
    }
    // consume args
    _pw_destroy_args(ap);
    return true;
}

[[nodiscard]] bool _pw_array_insert(PwValuePtr array_value, unsigned index, PwValuePtr item)
{
    pw_assert_array(array_value);
    _PwArray* array = get_array_struct_ptr(array_value);
    if (array->itercount) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }
    if (index > array->length) {
        pw_set_status(PwStatus(PW_ERROR_INDEX_OUT_OF_RANGE));
        return false;
    }
    if (!grow_array(array_value->type_id, array)) {
        return false;
    }
    if (!_pw_embrace(array_value, item)) {
        return false;
    }
    if (index < array->length) {
        memmove(&array->items[index + 1], &array->items[index], (array->length - index) * sizeof(_PwValue));
    }
    __pw_clone(item, &array->items[index]);  // destination contains garbage, so use __pw_clone here
    array->length++;
    return true;
}

[[nodiscard]] bool _pw_array_item_signed(PwValuePtr array_value, ssize_t index, PwValuePtr result)
{
    pw_assert_array(array_value);

    _PwArray* array = get_array_struct_ptr(array_value);

    if (index < 0) {
        index = array->length + index;
        if (index < 0) {
            pw_set_status(PwStatus(PW_ERROR_INDEX_OUT_OF_RANGE));
            return false;
        }
    } else if (index >= array->length) {
        pw_set_status(PwStatus(PW_ERROR_INDEX_OUT_OF_RANGE));
        return false;
    }
    pw_clone2(&array->items[index], result);
    return true;
}

[[nodiscard]] bool _pw_array_item(PwValuePtr array_value, unsigned index, PwValuePtr result)
{
    pw_assert_array(array_value);
    _PwArray* array = get_array_struct_ptr(array_value);
    if (index < array->length) {
        pw_destroy(result);
        pw_clone2(&array->items[index], result);
        return true;
    } else {
        pw_set_status(PwStatus(PW_ERROR_INDEX_OUT_OF_RANGE));
        return false;
    }
}

[[nodiscard]] bool _pw_array_set_item_signed(PwValuePtr array_value, ssize_t index, PwValuePtr item)
{
    pw_assert_array(array_value);
    _PwArray* array = get_array_struct_ptr(array_value);
    if (array->itercount) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }
    if (index < 0) {
        index = array->length + index;
        if (index < 0) {
            pw_set_status(PwStatus(PW_ERROR_INDEX_OUT_OF_RANGE));
            return false;
        }
    } else if (index >= array->length) {
        pw_set_status(PwStatus(PW_ERROR_INDEX_OUT_OF_RANGE));
        return false;
    }
    pw_clone2(item, &array->items[index]);
    return true;
}

[[nodiscard]] bool _pw_array_set_item(PwValuePtr array_value, unsigned index, PwValuePtr item)
{
    pw_assert_array(array_value);
    _PwArray* array = get_array_struct_ptr(array_value);
    if (array->itercount) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }
    if (index < array->length) {
        pw_clone2(item, &array->items[index]);
        return true;
    } else {
        pw_set_status(PwStatus(PW_ERROR_INDEX_OUT_OF_RANGE));
        return false;
    }
}

[[nodiscard]] bool pw_array_pull(PwValuePtr array_value, PwValuePtr result)
{
    pw_assert_array(array_value);
    _PwArray* array = get_array_struct_ptr(array_value);
    if (array->itercount) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }
    if (array->length == 0) {
        pw_set_status(PwStatus(PW_ERROR_EXTRACT_FROM_EMPTY_ARRAY));
        return false;
    }
    pw_move(&array->items[0], result);
    _pw_array_del(array, 0, 1, array_value);
    return true;
}

[[nodiscard]] bool pw_array_pop(PwValuePtr array_value, PwValuePtr result)
{
    pw_assert_array(array_value);
    _PwArray* array = get_array_struct_ptr(array_value);
    if (array->itercount) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }
    return _pw_array_pop(array_value, result);
}

void pw_array_del(PwValuePtr array_value, unsigned start_index, unsigned end_index)
{
    pw_assert_array(array_value);
    _PwArray* array = get_array_struct_ptr(array_value);
    if (array->itercount) {
        // pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return;
    }
    _pw_array_del(array, start_index, end_index, array_value);
}

void pw_array_clean(PwValuePtr array_value)
{
    pw_assert_array(array_value);
    _PwArray* array = get_array_struct_ptr(array_value);
    if (array->itercount) {
        // pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return;
    }
    _pw_array_del(array, 0, UINT_MAX, array_value);
}

void _pw_array_del(_PwArray* array, unsigned start_index, unsigned end_index, PwValuePtr parent)
{
    if (array->length == 0) {
        return;
    }
    if (end_index > array->length) {
        end_index = array->length;
    }
    if (start_index >= end_index) {
        return;
    }

    destroy_items(array, start_index, end_index, parent);

    unsigned new_length = array->length - (end_index - start_index);
    unsigned tail_length = array->length - end_index;
    if (tail_length) {
        memmove(&array->items[start_index], &array->items[end_index], tail_length * sizeof(_PwValue));
        memset(&array->items[new_length], 0, (array->length - new_length) * sizeof(_PwValue));
    }
    array->length = new_length;
}

[[nodiscard]] bool pw_array_slice(PwValuePtr array_value, unsigned start_index, unsigned end_index, PwValuePtr result)
{
    _PwArray* src_array = get_array_struct_ptr(array_value);
    unsigned length = _pw_array_length(src_array);

    if (end_index > length) {
        end_index = length;
    }
    if (start_index > end_index) {
        start_index = end_index;
    }
    unsigned slice_len = end_index - start_index;
    // XXX array constructor arg: initial size
    if (!pw_create_array(result)) {
        return false;
    }
    if (slice_len == 0) {
        return true;
    }
    if (!pw_array_resize(result, slice_len)) {
        return false;
    }

    _PwArray* dest_array = get_array_struct_ptr(result);

    PwValuePtr src_item_ptr = &src_array->items[start_index];
    PwValuePtr dest_item_ptr = dest_array->items;
    for (unsigned i = start_index; i < end_index; i++) {
        __pw_clone(src_item_ptr, dest_item_ptr);  // no need to destroy dest_item, so use __pw_clone here
        src_item_ptr++;
        dest_item_ptr++;
        dest_array->length++;
    }
    return true;
}

[[nodiscard]] bool _pw_array_join_c32(char32_t separator, PwValuePtr array, PwValuePtr result)
{
    char32_t s[2] = {separator, 0};
    PwValue sep = PwChar32Ptr(s);
    return _pw_array_join(&sep, array, result);
}

[[nodiscard]] bool _pw_array_join_u8(char8_t* separator, PwValuePtr array, PwValuePtr result)
{
    PwValue sep = PwCharPtr(separator);
    return _pw_array_join(&sep, array, result);
}

[[nodiscard]] bool _pw_array_join_u32(char32_t*  separator, PwValuePtr array, PwValuePtr result)
{
    PwValue sep = PwChar32Ptr(separator);
    return _pw_array_join(&sep, array, result);
}

[[nodiscard]] bool _pw_array_join(PwValuePtr separator, PwValuePtr array, PwValuePtr result)
{
    unsigned num_items = pw_array_length(array);
    if (num_items == 0) {
        *result = PwString(0, {});
        return true;
    }
    if (num_items == 1) {
        PwValue item = PW_NULL;
        if (!pw_array_item(array, 0, &item)) {
            return false;
        }
        if (pw_is_string(&item)) {
            pw_move(&item, result);
            return true;
        } if (pw_is_charptr(&item)) {
            return pw_charptr_to_string(&item, result);
        } else {
            // XXX skipping non-string values
            *result = PwString(0, {});
            return true;
        }
    }

    uint8_t max_char_size;
    unsigned separator_len;

    if (pw_is_string(separator)) {
        max_char_size = _pw_string_char_size(separator);
        separator_len = _pw_string_length(separator);

    } if (pw_is_charptr(separator)) {
        separator_len = _pw_charptr_strlen2(separator, &max_char_size);

    } else {
        pw_set_status(PwStatus(PW_ERROR_INCOMPATIBLE_TYPE),
                      "Bad separator type for pw_array_join: %u, %s",
                      separator->type_id, pw_get_type_name(separator->type_id));
        return false;
    }

    // calculate total length and max char width of string items
    unsigned result_len = 0;
    for (unsigned i = 0; i < num_items; i++) {{   // nested scope for autocleaning item
        if (i) {
            result_len += separator_len;
        }
        PwValue item = PW_NULL;
        if (!pw_array_item(array, i, &item)) {
            return false;
        }
        uint8_t char_size;
        if (pw_is_string(&item)) {
            char_size = _pw_string_char_size(&item);
            result_len += _pw_string_length(&item);
        } else if (pw_is_charptr(&item)) {
            result_len += _pw_charptr_strlen2(separator, &char_size);
        } else {
            // XXX skipping non-string values
            continue;
        }
        if (max_char_size < char_size) {
            max_char_size = char_size;
        }
    }}

    // join array items
    if (!pw_create_empty_string(result_len, max_char_size, result)) {
        return false;
    }
    for (unsigned i = 0; i < num_items; i++) {{   // nested scope for autocleaning item
        PwValue item = PW_NULL;
        if (!pw_array_item(array, i, &item)) {
            return false;
        }
        if (pw_is_string(&item) || pw_is_charptr(&item)) {
            if (i) {
                if (!_pw_string_append(result, separator)) {
                    pw_destroy(result);
                    return false;
                }
            }
            if (!_pw_string_append(result, &item)) {
                pw_destroy(result);
                return false;
            }
        }
    }}
    return true;
}

[[nodiscard]] bool pw_array_dedent(PwValuePtr lines)
{
    // dedent inplace, so access items directly to avoid cloning
    _PwArray* array = get_array_struct_ptr(lines);

    if (array->itercount) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }

    static char32_t indent_chars[] = {' ', '\t', 0};

    unsigned n = pw_array_length(lines);

    unsigned indent[n];

    // measure indents
    unsigned min_indent = UINT_MAX;
    for (unsigned i = 0; i < n; i++) {
        PwValuePtr line = &array->items[i];
        indent[i] = 0;
        if (pw_is_string(line)) {
            indent[i] = pw_string_skip_chars(line, 0, indent_chars);
            if (indent[i] && indent[i] < min_indent) {
                min_indent = indent[i];
            }
        }
    }
    if (min_indent == UINT_MAX) {
        // nothing to dedent
        return true;
    }

    // dedent
    for (unsigned i = 0; i < n; i++) {
        if (indent[i]) {
            PwValuePtr line = &array->items[i];
            if (!pw_string_erase(line, 0, min_indent)) {
                return false;
            }
        }
    }
    return true;
}


/****************************************************************
 * RandomAccess interface
 */

[[nodiscard]] static bool key_to_index(PwValuePtr key, PwValuePtr result)
/*
 * Convert key to integer index, either signed or unsigned.
 */
{
    PwValue k = PW_NULL;
    if (pw_is_charptr(key)) {
        if (!pw_charptr_to_string(key, &k)) {
            return false;
        }
    } else {
        pw_clone2(key, &k);
    }
    if (pw_is_string(&k)) {
        PwValue index = PW_NULL;
        if (!pw_parse_number(&k, &index)) {
            return false;
        }
        pw_move(&index, &k);
        // continue with bounds checking
    }

    // note bounds checking aren't inclusive because:
    // a) capacity is anyway less by a factor of value size;
    // b) delete needs index + 1

    if (pw_is_unsigned(&k)) {
#       if UINT_MAX < PW_UNSIGNED_MAX
            if (k.unsigned_value < UINT_MAX) {
                pw_move(&k, result);
                return true;
            }
            pw_set_status(PwStatus(PW_ERROR_INDEX_OUT_OF_RANGE));
            return false;
#       else
            pw_move(&k, result);
            return true;
#       endif
    }
    if (pw_is_signed(&k)) {
#       if INT_MAX < PW_SIGNED_MAX
            if (INT_MIN < k.signed_value && k.signed_value < INT_MAX) {
                pw_move(&k, result);
                return true;
            }
            pw_set_status(PwStatus(PW_ERROR_INDEX_OUT_OF_RANGE));
            return false;
#       else
            pw_move(&k, result);
            return true;
#       endif
    }
    pw_set_status(PwStatus(PW_ERROR_INCOMPATIBLE_TYPE));
    return false;
}

[[nodiscard]] static bool ra_get_item(PwValuePtr self, PwValuePtr key, PwValuePtr result)
{
    PwValue index = PW_NULL;
    if (!key_to_index(key, &index)) {
        return false;
    }
    if (pw_is_unsigned(&index)) {
        return _pw_array_item(self, index.unsigned_value, result);
    } else {
        return _pw_array_item_signed(self, index.signed_value, result);
    }
}

[[nodiscard]] static bool ra_set_item(PwValuePtr self, PwValuePtr key, PwValuePtr value)
{
    PwValue index = PW_NULL;
    if (!key_to_index(key, &index)) {
        return false;
    }
    if (pw_is_unsigned(&index)) {
        return _pw_array_set_item(self, index.unsigned_value, value);
    } else {
        return _pw_array_set_item_signed(self, index.signed_value, value);
    }
}

[[nodiscard]] static bool ra_delete_item(PwValuePtr self, PwValuePtr key)
{
    PwValue index = PW_NULL;
    if (!key_to_index(key, &index)) {
        return false;
    }
    unsigned i;
    if (pw_is_unsigned(&index)) {
        i = index.unsigned_value;
    } else {
        if (index.signed_value < 0) {
            index.signed_value += pw_array_length(self);
        }
        if (index.signed_value < 0) {
            pw_set_status(PwStatus(PW_ERROR_INDEX_OUT_OF_RANGE));
            return false;
        }
        i = index.signed_value;
    }
    pw_array_del(self, i, i + 1);
    return true;
}

static PwInterface_RandomAccess random_access_interface = {
    .length      = pw_array_length,
    .get_item    = ra_get_item,
    .set_item    = ra_set_item,
    .delete_item = ra_delete_item
};
