#include <limits.h>
#include <string.h>

#include "include/pw.h"
#include "include/pw_parse.h"
#include "src/pw_charptr_internal.h"
#include "src/pw_array_internal.h"
#include "src/pw_string_internal.h"
#include "src/pw_struct_internal.h"

[[noreturn]]
static void panic_status()
{
    pw_panic("Array cannot contain Status values");
}

static void array_fini(PwValuePtr self)
{
    _PwArray* array_data = get_array_data_ptr(self);
    pw_assert(array_data->itercount == 0);
    _pw_destroy_array(self->type_id, array_data, self);
}

static PwResult array_init(PwValuePtr self, void* ctor_args)
{
    // XXX not using ctor_args for now

    PwValue status = _pw_alloc_array(self->type_id, get_array_data_ptr(self), PWARRAY_INITIAL_CAPACITY);
    if (pw_error(&status)) {
        array_fini(self);
    }
    return pw_move(&status);
}

static void array_hash(PwValuePtr self, PwHashContext* ctx)
{
    _pw_hash_uint64(ctx, self->type_id);
    _PwArray* array_data = get_array_data_ptr(self);
    PwValuePtr item_ptr = array_data->items;
    for (unsigned n = array_data->length; n; n--, item_ptr++) {
        _pw_call_hash(item_ptr, ctx);
    }
}

static PwResult array_deepcopy(PwValuePtr self)
{
    PwValue dest = pw_create(self->type_id);
    pw_return_if_error(&dest);

    _PwArray* src_array = get_array_data_ptr(self);
    _PwArray* dest_array = get_array_data_ptr(&dest);

    pw_expect_ok( _pw_array_resize(dest.type_id, dest_array, src_array->length) );

    PwValuePtr src_item_ptr = src_array->items;
    PwValuePtr dest_item_ptr = dest_array->items;
    for (unsigned i = 0; i < src_array->length; i++) {
        *dest_item_ptr = pw_deepcopy(src_item_ptr);
        pw_return_if_error(dest_item_ptr);
        _pw_embrace(&dest, dest_item_ptr);
        src_item_ptr++;
        dest_item_ptr++;
        dest_array->length++;
    }
    return pw_move(&dest);
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

    _PwArray* array_data = get_array_data_ptr(self);
    fprintf(fp, "%u items, capacity=%u\n", array_data->length, array_data->capacity);

    next_indent += 4;
    PwValuePtr item_ptr = array_data->items;
    for (unsigned n = array_data->length; n; n--, item_ptr++) {
        _pw_call_dump(fp, item_ptr, next_indent, next_indent, &this_link);
    }
}

static PwResult array_to_string(PwValuePtr self)
{
    return PwError(PW_ERROR_NOT_IMPLEMENTED);
}

static bool array_is_true(PwValuePtr self)
{
    return get_array_data_ptr(self)->length;
}

static bool array_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    return _pw_array_eq(get_array_data_ptr(self), get_array_data_ptr(other));
}

static bool array_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        if (t == PwTypeId_Array) {
            return _pw_array_eq(get_array_data_ptr(self), get_array_data_ptr(other));
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

PwResult _pw_array_create(...)
{
    va_list ap;
    va_start(ap);
    PwValue array = pw_create(PwTypeId_Array);
    if (pw_error(&array)) {
        // pw_array_append_ap destroys args on exit, we must do the same
        _pw_destroy_args(ap);
        return pw_move(&array);
    }
    PwValue status = pw_array_append_ap(&array, ap);
    va_end(ap);
    pw_return_if_error(&status);
    return pw_move(&array);
}

PwResult _pw_alloc_array(PwTypeId type_id, _PwArray* array_data, unsigned capacity)
{
    if (capacity >= UINT_MAX / sizeof(_PwValue)) {
        return PwError(PW_ERROR_DATA_SIZE_TOO_BIG);
    }

    array_data->length = 0;
    array_data->capacity = round_capacity(capacity);

    unsigned memsize = array_data->capacity * sizeof(_PwValue);
    array_data->items = _pw_types[type_id]->allocator->allocate(memsize, true);

    pw_return_ok_or_oom( array_data->items );
}

void _pw_destroy_array(PwTypeId type_id, _PwArray* array_data, PwValuePtr parent)
{
    if (array_data->items) {
        PwValuePtr item_ptr = array_data->items;
        for (unsigned n = array_data->length; n; n--, item_ptr++) {
            if (pw_is_compound(item_ptr)) {
                _pw_abandon(parent, item_ptr);
            }
            pw_destroy(item_ptr);
        }
        unsigned memsize = array_data->capacity * sizeof(_PwValue);
        _pw_types[type_id]->allocator->release((void**) &array_data->items, memsize);
    }
}

PwResult _pw_array_resize(PwTypeId type_id, _PwArray* array_data, unsigned desired_capacity)
{
    if (desired_capacity < array_data->length) {
        desired_capacity = array_data->length;
    } else if (desired_capacity >= UINT_MAX / sizeof(_PwValue)) {
        return PwError(PW_ERROR_DATA_SIZE_TOO_BIG);
    }
    unsigned new_capacity = round_capacity(desired_capacity);

    Allocator* allocator = _pw_types[type_id]->allocator;

    unsigned old_memsize = array_data->capacity * sizeof(_PwValue);
    unsigned new_memsize = new_capacity * sizeof(_PwValue);

    if (!allocator->reallocate((void**) &array_data->items, old_memsize, new_memsize, true, nullptr)) {
        return PwOOM();
    }
    array_data->capacity = new_capacity;
    return PwOK();
}

PwResult pw_array_resize(PwValuePtr array, unsigned desired_capacity)
{
    pw_assert_array(array);
    _PwArray* array_data = get_array_data_ptr(array);
    if (array_data->itercount) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }
    return _pw_array_resize(array->type_id, array_data, desired_capacity);
}

unsigned pw_array_length(PwValuePtr array)
{
    pw_assert_array(array);
    return _pw_array_length(get_array_data_ptr(array));
}

bool _pw_array_eq(_PwArray* a, _PwArray* b)
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

PwResult _pw_array_append(PwValuePtr array, PwValuePtr item)
// XXX this will be an interface method, _pwi_array_append
{
    pw_assert_array(array);
    _PwArray* array_data = get_array_data_ptr(array);
    if (array_data->itercount) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }
    PwValue v = pw_clone(item);
    return _pw_array_append_item(array->type_id, array_data, &v, array);
}

static PwResult grow_array(PwTypeId type_id, _PwArray* array_data)
{
    pw_assert(array_data->length <= array_data->capacity);

    if (array_data->length == array_data->capacity) {
        unsigned new_capacity;
        if (array_data->capacity <= PWARRAY_CAPACITY_INCREMENT) {
            new_capacity = array_data->capacity + PWARRAY_INITIAL_CAPACITY;
        } else {
            new_capacity = array_data->capacity + PWARRAY_CAPACITY_INCREMENT;
        }
        pw_expect_ok( _pw_array_resize(type_id, array_data, new_capacity) );
    }
    return PwOK();
}

PwResult _pw_array_append_item(PwTypeId type_id, _PwArray* array_data, PwValuePtr item, PwValuePtr parent)
{
    if (pw_is_status(item)) {
        // prohibit appending Status values
        panic_status();
    }
    pw_expect_ok( grow_array(type_id, array_data) );
    _pw_embrace(parent, item);
    array_data->items[array_data->length] = pw_move(item);
    array_data->length++;
    return PwOK();
}

PwResult _pw_array_append_va(PwValuePtr array, ...)
{
    va_list ap;
    va_start(ap);
    PwValue result = pw_array_append_ap(array, ap);
    va_end(ap);
    return pw_move(&result);
}

PwResult pw_array_append_ap(PwValuePtr dest, va_list ap)
{
    pw_assert_array(dest);
    _PwArray* array_data = get_array_data_ptr(dest);
    if (array_data->itercount) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }
    PwTypeId type_id = dest->type_id;
    unsigned num_appended = 0;
    PwValue error = PwOOM();  // default error is OOM unless some arg is a status
    for(;;) {{
        PwValue arg = va_arg(ap, _PwValue);
        if (pw_is_status(&arg)) {
            if (pw_va_end(&arg)) {
                return PwOK();
            }
            pw_destroy(&error);
            error = pw_move(&arg);
            goto failure;
        }
        if (!pw_charptr_to_string_inplace(&arg)) {
            goto failure;
        }
        PwValue status = _pw_array_append_item(type_id, array_data, &arg, dest);
        if (pw_error(&status)) {
            pw_destroy(&error);
            error = pw_move(&status);
            goto failure;
        }
        num_appended++;
    }}

failure:
    // rollback
    while (num_appended--) {
        PwValue v = _pw_array_pop(array_data);
        pw_destroy(&v);
    }
    // consume args
    _pw_destroy_args(ap);
    return pw_move(&error);
}

PwResult _pw_array_insert(PwValuePtr array, unsigned index, PwValuePtr item)
{
    if (pw_is_status(item)) {
        // prohibit appending Status values
        panic_status();
    }
    pw_assert_array(array);
    _PwArray* array_data = get_array_data_ptr(array);
    if (array_data->itercount) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }
    if (index > array_data->length) {
        return PwError(PW_ERROR_INDEX_OUT_OF_RANGE);
    }
    pw_expect_ok( grow_array(array->type_id, array_data) );

    _pw_embrace(array, item);
    if (index < array_data->length) {
        memmove(&array_data->items[index + 1], &array_data->items[index], (array_data->length - index) * sizeof(_PwValue));
    }
    array_data->items[index] = pw_clone(item);
    array_data->length++;
    return PwOK();
}

PwResult _pw_array_item_signed(PwValuePtr array, ssize_t index)
{
    pw_assert_array(array);

    _PwArray* array_data = get_array_data_ptr(array);

    if (index < 0) {
        index = array_data->length + index;
        if (index < 0) {
            return PwError(PW_ERROR_INDEX_OUT_OF_RANGE);
        }
    } else if (index >= array_data->length) {
        return PwError(PW_ERROR_INDEX_OUT_OF_RANGE);
    }
    return pw_clone(&array_data->items[index]);
}

PwResult _pw_array_item(PwValuePtr array, unsigned index)
{
    pw_assert_array(array);
    _PwArray* array_data = get_array_data_ptr(array);
    if (index < array_data->length) {
        return pw_clone(&array_data->items[index]);
    } else {
        return PwError(PW_ERROR_INDEX_OUT_OF_RANGE);
    }
}

PwResult _pw_array_set_item_signed(PwValuePtr array, ssize_t index, PwValuePtr item)
{
    pw_assert_array(array);
    _PwArray* array_data = get_array_data_ptr(array);
    if (array_data->itercount) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }
    if (index < 0) {
        index = array_data->length + index;
        if (index < 0) {
            return PwError(PW_ERROR_INDEX_OUT_OF_RANGE);
        }
    } else if (index >= array_data->length) {
        return PwError(PW_ERROR_INDEX_OUT_OF_RANGE);
    }

    pw_destroy(&array_data->items[index]);
    array_data->items[index] = pw_clone(item);
    return PwOK();
}

PwResult _pw_array_set_item(PwValuePtr array, unsigned index, PwValuePtr item)
{
    pw_assert_array(array);
    _PwArray* array_data = get_array_data_ptr(array);
    if (array_data->itercount) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }
    if (index < array_data->length) {
        pw_destroy(&array_data->items[index]);
        array_data->items[index] = pw_clone(item);
        return PwOK();
    } else {
        return PwError(PW_ERROR_INDEX_OUT_OF_RANGE);
    }
}

PwResult pw_array_pull(PwValuePtr array)
{
    pw_assert_array(array);
    _PwArray* array_data = get_array_data_ptr(array);
    if (array_data->itercount) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }
    if (array_data->length == 0) {
        return PwError(PW_ERROR_EXTRACT_FROM_EMPTY_ARRAY);
    }
    _PwValue result = pw_clone(&array_data->items[0]);
    _pw_array_del(array_data, 0, 1);
    return result;
}

PwResult pw_array_pop(PwValuePtr array)
{
    pw_assert_array(array);
    _PwArray* array_data = get_array_data_ptr(array);
    if (array_data->itercount) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }
    return _pw_array_pop(array_data);
}

PwResult _pw_array_pop(_PwArray* array_data)
{
    if (array_data->length == 0) {
        return PwError(PW_ERROR_EXTRACT_FROM_EMPTY_ARRAY);
    }
    array_data->length--;
    return pw_move(&array_data->items[array_data->length]);
}

void pw_array_del(PwValuePtr array, unsigned start_index, unsigned end_index)
{
    pw_assert_array(array);
    _PwArray* array_data = get_array_data_ptr(array);
    if (array_data->itercount) {
        return; // PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }
    _pw_array_del(array_data, start_index, end_index);
}

void pw_array_clean(PwValuePtr array)
{
    pw_assert_array(array);
    _PwArray* array_data = get_array_data_ptr(array);
    if (array_data->itercount) {
        return; // PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }
    _pw_array_del(array_data, 0, UINT_MAX);
}

void _pw_array_del(_PwArray* array_data, unsigned start_index, unsigned end_index)
{
    if (array_data->length == 0) {
        return;
    }
    if (end_index > array_data->length) {
        end_index = array_data->length;
    }
    if (start_index >= end_index) {
        return;
    }

    PwValuePtr item_ptr = &array_data->items[start_index];
    for (unsigned i = start_index; i < end_index; i++, item_ptr++) {
        pw_destroy(item_ptr);
    }
    unsigned new_length = array_data->length - (end_index - start_index);
    unsigned tail_length = array_data->length - end_index;
    if (tail_length) {
        memmove(&array_data->items[start_index], &array_data->items[end_index], tail_length * sizeof(_PwValue));
        memset(&array_data->items[new_length], 0, (array_data->length - new_length) * sizeof(_PwValue));
    }
    array_data->length = new_length;
}

PwResult pw_array_slice(PwValuePtr array, unsigned start_index, unsigned end_index)
{
    _PwArray* src_array = get_array_data_ptr(array);
    unsigned length = _pw_array_length(src_array);

    if (end_index > length) {
        end_index = length;
    }
    if (start_index >= end_index) {
        // return empty array
        return PwArray();
    }
    unsigned slice_len = end_index - start_index;

    PwValue dest = PwArray();
    pw_return_if_error(&dest);

    pw_expect_ok( pw_array_resize(&dest, slice_len) );

    _PwArray* dest_array = get_array_data_ptr(&dest);

    PwValuePtr src_item_ptr = &src_array->items[start_index];
    PwValuePtr dest_item_ptr = dest_array->items;
    for (unsigned i = start_index; i < end_index; i++) {
        *dest_item_ptr = pw_clone(src_item_ptr);
        src_item_ptr++;
        dest_item_ptr++;
        dest_array->length++;
    }
    return pw_move(&dest);
}

PwResult _pw_array_join_c32(char32_t separator, PwValuePtr array)
{
    char32_t s[2] = {separator, 0};
    PwValue sep = PwChar32Ptr(s);
    return _pw_array_join(&sep, array);
}

PwResult _pw_array_join_u8(char8_t* separator, PwValuePtr array)
{
    PwValue sep = PwCharPtr(separator);
    return _pw_array_join(&sep, array);
}

PwResult _pw_array_join_u32(char32_t*  separator, PwValuePtr array)
{
    PwValue sep = PwChar32Ptr(separator);
    return _pw_array_join(&sep, array);
}

PwResult _pw_array_join(PwValuePtr separator, PwValuePtr array)
{
    unsigned num_items = pw_array_length(array);
    if (num_items == 0) {
        return PwString();
    }
    if (num_items == 1) {
        PwValue item = pw_array_item(array, 0);
        if (pw_is_string(&item)) {
            return pw_move(&item);
        } if (pw_is_charptr(&item)) {
            return pw_charptr_to_string(&item);
        } else {
            // XXX skipping non-string values
            return PwString();
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
        PwValue error = PwError(PW_ERROR_INCOMPATIBLE_TYPE);
        _pw_set_status_desc(&error, "Bad separator type for pw_array_join: %u, %s",
                            separator->type_id, pw_get_type_name(separator->type_id));
        return pw_move(&error);
    }

    // calculate total length and max char width of string items
    unsigned result_len = 0;
    for (unsigned i = 0; i < num_items; i++) {{   // nested scope for autocleaning item
        if (i) {
            result_len += separator_len;
        }
        PwValue item = pw_array_item(array, i);
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
    PwValue result = pw_create_empty_string(result_len, max_char_size);
    pw_return_if_error(&result);

    for (unsigned i = 0; i < num_items; i++) {{   // nested scope for autocleaning item
        PwValue item = pw_array_item(array, i);
        if (pw_is_string(&item) || pw_is_charptr(&item)) {
            if (i) {
                pw_expect_true( _pw_string_append(&result, separator) );
            }
            pw_expect_true( _pw_string_append(&result, &item) );
        }
    }}
    return pw_move(&result);
}

PwResult pw_array_dedent(PwValuePtr lines)
{
    // dedent inplace, so access items directly to avoid cloning
    _PwArray* array_data = get_array_data_ptr(lines);

    if (array_data->itercount) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }

    static char32_t indent_chars[] = {' ', '\t', 0};

    unsigned n = pw_array_length(lines);

    unsigned indent[n];

    // measure indents
    unsigned min_indent = UINT_MAX;
    for (unsigned i = 0; i < n; i++) {
        PwValuePtr line = &array_data->items[i];
        if (pw_is_string(line)) {
            indent[i] = pw_string_skip_chars(line, 0, indent_chars);
            if (_pw_string_length(line) && indent[i] < min_indent) {
                min_indent = indent[i];
            }
        } else {
            indent[i] = 0;
        }
    }
    if (min_indent == UINT_MAX || min_indent == 0) {
        // nothing to dedent
        return PwOK();
    }

    for (unsigned i = 0; i < n; i++) {
        if (indent[i]) {
            PwValuePtr line = &array_data->items[i];
            if (!pw_string_erase(line, 0, min_indent)) {
                return PwOOM();
            }
        }
    }
    return PwOK();
}


/****************************************************************
 * RandomAccess interface
 */

static PwResult key_to_index(PwValuePtr key)
/*
 * Convert key to integer index, either signed or unsigned.
 */
{
    // clone key for two reasons:
    // 1) it's the result;
    // 2) convert CharPtr to String

    PwValue k = pw_clone(key);

    if (pw_is_string(&k)) {
        PwValue index = pw_parse_number(&k);
        pw_return_if_error(&index);
        pw_destroy(&k);
        k = pw_move(&index);
        // continue with bounds checking
    }

    // note bounds checking aren't inclusive because:
    // a) capacity is anyway less by a factor of value size;
    // b) delete needs index + 1

    if (pw_is_unsigned(&k)) {
#       if UINT_MAX < PW_UNSIGNED_MAX
            if (k.unsigned_value < UINT_MAX) {
                return pw_move(&k);
            }
            return PwError(PW_ERROR_INDEX_OUT_OF_RANGE);
#       else
            return pw_move(&k);
#       endif
    }
    if (pw_is_signed(&k)) {
#       if INT_MAX < PW_SIGNED_MAX
            if (INT_MIN < k.signed_value && k.signed_value < INT_MAX) {
                return pw_move(&k);
            }
            return PwError(PW_ERROR_INDEX_OUT_OF_RANGE);
#       else
            return pw_move(&k);
#       endif
    }
    return PwError(PW_ERROR_INCOMPATIBLE_TYPE);
}

static PwResult ra_get_item(PwValuePtr self, PwValuePtr key)
{
    PwValue index = key_to_index(key);
    pw_return_if_error(&index);
    if (pw_is_unsigned(&index)) {
        return _pw_array_item(self, index.unsigned_value);
    } else {
        return _pw_array_item_signed(self, index.signed_value);
    }
}

static PwResult ra_set_item(PwValuePtr self, PwValuePtr key, PwValuePtr value)
{
    PwValue index = key_to_index(key);
    pw_return_if_error(&index);
    if (pw_is_unsigned(&index)) {
        return _pw_array_set_item(self, index.unsigned_value, value);
    } else {
        return _pw_array_set_item_signed(self, index.signed_value, value);
    }
}

static PwResult ra_delete_item(PwValuePtr self, PwValuePtr key)
{
    PwValue index = key_to_index(key);
    pw_return_if_error(&index);

    unsigned i;
    if (pw_is_unsigned(&index)) {
        i = index.unsigned_value;
    } else {
        if (index.signed_value < 0) {
            index.signed_value += pw_array_length(self);
        }
        if (index.signed_value < 0) {
            return PwError(PW_ERROR_INDEX_OUT_OF_RANGE);
        }
        i = index.signed_value;
    }
    pw_array_del(self, i, i + 1);
    return PwOK();
}

static PwInterface_RandomAccess random_access_interface = {
    .length      = pw_array_length,
    .get_item    = ra_get_item,
    .set_item    = ra_set_item,
    .delete_item = ra_delete_item
};
