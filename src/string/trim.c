#include "include/pw.h"
#include "src/string/pw_string_internal.h"

[[ nodiscard]] bool pw_string_ltrim(PwValuePtr str)
{
    pw_assert_string(str);

    uint8_t char_size = str->char_size;
    uint8_t* end_ptr;
    uint8_t* start_ptr = _pw_string_start_end(str, &end_ptr);
    StrSkipSpaces fn_skip_spaces = _pw_skip_spaces_variants[char_size];
    uint8_t* nonspace_ptr = fn_skip_spaces(start_ptr, end_ptr);

    if (nonspace_ptr == start_ptr) {
        return true;
    }
    if ( nonspace_ptr == end_ptr) {
        pw_destroy(str);
        *str = PwString("");
        return true;
    }
    ptrdiff_t n = end_ptr - nonspace_ptr;
    unsigned new_length = n / char_size;
    if (!_pw_string_need_copy_on_write(str)) {
        memmove(start_ptr, nonspace_ptr, n);
        _pw_string_set_length(str, new_length);
        return true;
    }
    // make substring
    pw_destroy(str);
    if (!_pw_make_empty_string(str->type_id, new_length, char_size, str)) {
        return false;
    }
    if (new_length) {
        _pw_string_set_length(str, new_length);
        StrAppend fn_append = _pw_str_append_variants[char_size][char_size];
        if (!fn_append(str, 0, nonspace_ptr, end_ptr)) {
            return false;
        }
        if (str->embedded) {
            // clean remainder for fast comparison to work
            unsigned i = char_size * new_length;
            while (i < sizeof(str->str_1)) {
                str->str_1[i++] = 0;
            }
        }
    }
    return true;
}

[[ nodiscard]] bool pw_string_rtrim(PwValuePtr str)
{
    pw_assert_string(str);

    uint8_t char_size = str->char_size;
    uint8_t* end_ptr;
    uint8_t* start_ptr = _pw_string_start_end(str, &end_ptr);

    if (start_ptr == end_ptr) {
        return true;
    }
    while (start_ptr < end_ptr) {
        uint8_t* prev_end_ptr = end_ptr;
        char32_t c = _pw_get_char_reverse(&end_ptr, char_size);
        if (!pw_isspace(c)) {
            end_ptr = prev_end_ptr;
            break;
        }
    }
    if (start_ptr == end_ptr) {
        pw_destroy(str);
        *str = PwString("");
        return true;
    }
    return pw_string_truncate(str, (end_ptr - start_ptr) / char_size);
}

[[ nodiscard]] bool pw_string_trim(PwValuePtr str)
{
    return pw_string_rtrim(str) && pw_string_ltrim(str);
}
