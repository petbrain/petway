#include "include/pw.h"
#include "src/pw_alloc.h"
#include "src/string/pw_string_internal.h"

// lookup table to validate capacity

#define _header_size  offsetof(_PwStringData, data)

static unsigned _max_capacity[5] = {
    0,
    0xFFFF'FFFF - _header_size,
    (0xFFFF'FFFF - _header_size) / 2,
    (0xFFFF'FFFF - _header_size) / 3,
    (0xFFFF'FFFF - _header_size) / 4
};

unsigned _pw_calc_string_data_size(uint8_t char_size, unsigned desired_capacity, unsigned* real_capacity)
{
    unsigned size = offsetof(_PwStringData, data) + char_size * desired_capacity + PWSTRING_BLOCK_SIZE - 1;
    size &= ~(PWSTRING_BLOCK_SIZE - 1);
    if (real_capacity) {
        *real_capacity = (size - offsetof(_PwStringData, data)) / char_size;
    }
    return size;
}

[[ nodiscard]] bool _pw_make_empty_string(PwTypeId type_id, unsigned capacity, uint8_t char_size, PwValuePtr result)
{
    pw_assert(1 <= char_size && char_size <= 4);

    pw_destroy(result);
    result->type_id = type_id;
    result->char_size = char_size;

    // check if string can be embedded into result

    if (capacity <= embedded_capacity[char_size]) {
        result->allocated = 0;
        result->embedded = 1;
        result->embedded_length = 0;
        result->str_4[0] = 0;
        result->str_4[1] = 0;
        result->str_4[2] = 0;
        return true;
    }

    if(capacity > _max_capacity[char_size]) {
        pw_set_status(PwStatus(PW_ERROR_STRING_TOO_LONG));
        return false;
    }

    result->embedded = 0;
    result->length = 0;

    // allocate string

    unsigned real_capacity;
    unsigned memsize = _pw_calc_string_data_size(char_size, capacity, &real_capacity);

    _PwStringData* string_data = _pw_alloc(result->type_id, memsize, false);
    if (!string_data) {
        return false;
    }
    string_data->refcount = 1;
    string_data->capacity = real_capacity;
    result->string_data = string_data;
    result->embedded = 0;
    result->allocated = 1;
    return true;
}

[[ nodiscard]] bool _pw_string_do_copy_on_write(PwValuePtr str)
{
    unsigned length = str->length;
    uint8_t char_size = str->char_size;

    // allocate string
    PwValue s = PW_NULL;
    if (!_pw_make_empty_string(str->type_id, length, char_size, &s)) {
        return false;
    }
    uint8_t* char_ptr;
    if (str->allocated) {
        _PwStringData* orig_sdata = str->string_data;
        char_ptr = orig_sdata->data;
    } else {
        // static string here, embedded case is filtered out by _pw_string_need_copy_on_write
        char_ptr = str->char_ptr;
    }
    // copy original string to new string
    memcpy(_pw_string_start(&s), char_ptr, length * char_size);
    _pw_string_set_length(&s, length);
    pw_move(&s, str);
    return true;
}

[[ nodiscard]] bool _pw_expand_string(PwValuePtr str, unsigned increment, uint8_t new_char_size)
{
    uint8_t char_size = str->char_size;
    if (_pw_unlikely(new_char_size < char_size)) {
        // current char_size is greater than new one, use current as new:
        new_char_size = char_size;
    }

    unsigned old_capacity = 0;

    if (str->embedded) {
        unsigned new_length = str->embedded_length + increment;
        if (_pw_likely(new_length <= embedded_capacity[new_char_size])) {
            // no need to expand
            if (_pw_unlikely(new_char_size > char_size)) {
                // but need to make existing chars wider
                _PwValue orig_str = *str;
                str->char_size = new_char_size;

                StrCopy fn_copy = _pw_strcopy_variants[new_char_size][char_size];
                uint8_t* src_end_ptr;
                uint8_t* src_start_ptr = _pw_string_start_end(&orig_str, &src_end_ptr);
                fn_copy(_pw_string_start(str), src_start_ptr, src_end_ptr);
            }
            return true;
        }
        // increased capacity is beyond embedded capacity; go copy
        // !! not necessary: old_capacity = embedded_capacity[char_size];

    } else if (str->allocated) {

        old_capacity = str->string_data->capacity;

        if (new_char_size == char_size) {
            unsigned new_capacity = str->length + increment;

            if (_pw_likely(new_capacity <= str->string_data->capacity)) {
                // no need to expand
                return true;
            }
            if (_pw_likely(str->string_data->refcount == 1)) {

                // expand string in-place

                if (_pw_unlikely(increment > _max_capacity[char_size] - str->length)) {
                    pw_set_status(PwStatus(PW_ERROR_STRING_TOO_LONG));
                    return false;
                }
                unsigned orig_memsize = _pw_allocated_string_data_size(str);
                unsigned new_memsize = _pw_calc_string_data_size(char_size, new_capacity, &str->string_data->capacity);

                // reallocate data
                return _pw_realloc(str->type_id, (void**) &str->string_data, orig_memsize, new_memsize, false);
            }
        }
    }

    // make a copy of string

    unsigned length = pw_strlen(str);
    unsigned new_capacity = length + increment;

    // preserve memory size
    if (new_capacity * new_char_size < old_capacity * char_size) {
        new_capacity = old_capacity * char_size / new_char_size;
        pw_assert(new_capacity >= length + increment);
    }

    if (_pw_unlikely(increment > _max_capacity[new_char_size] - length)) {
        pw_set_status(PwStatus(PW_ERROR_STRING_TOO_LONG));
        return false;
    }

    // allocate string
    _PwValue orig_str = *str;
    str->type_id = PwTypeId_Null;
    if (!_pw_make_empty_string(orig_str.type_id, new_capacity, new_char_size, str)) {
        return false;
    }
    // copy original string to new string
    StrCopy fn_copy = _pw_strcopy_variants[new_char_size][char_size];
    uint8_t* src_end_ptr;
    uint8_t* src_start_ptr = _pw_string_start_end(&orig_str, &src_end_ptr);
    fn_copy(_pw_string_start(str), src_start_ptr, src_end_ptr);
    _pw_string_set_length(str, length);

    _pw_string_destroy(&orig_str);

    return true;
}

/****************************************************************
 * Constructors
 */

[[nodiscard]] bool pw_create_empty_string(unsigned capacity, uint8_t char_size, PwValuePtr result)
{
    return _pw_make_empty_string(PwTypeId_String, capacity, char_size, result);
}

[[nodiscard]] bool _pw_create_string(PwValuePtr initializer, PwValuePtr result)
{
    if (!pw_create_empty_string(0, 1, result)) {
        return false;
    }
    return pw_string_append(result, initializer);
}

[[nodiscard]] bool _pw_create_string_ascii(char* initializer, PwValuePtr result)
{
    if (!pw_create_empty_string(0, 1, result)) {
        return false;
    }
    return pw_string_append(result, initializer, nullptr);
}

[[nodiscard]] bool _pw_create_string_utf8(char8_t* initializer, PwValuePtr result)
{
    if (!pw_create_empty_string(0, 1, result)) {
        return false;
    }
    return pw_string_append(result, initializer, nullptr);
}

[[nodiscard]] bool _pw_create_string_utf32(char32_t* initializer, PwValuePtr result)
{
    if (!pw_create_empty_string(0, 1, result)) {
        return false;
    }
    return pw_string_append(result, initializer, nullptr);
}
