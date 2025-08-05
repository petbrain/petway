#include "include/pw.h"
#include "src/string/pw_string_internal.h"

[[nodiscard]] bool pw_string_rsplit_chr(PwValuePtr str, char32_t splitter, unsigned maxsplit, PwValuePtr result)
{
    pw_assert_string(str);

    if (!pw_create_array(result)) {
        return false;
    }
    uint8_t str_char_size = str->char_size;
    if (str_char_size < calc_char_size(splitter)) {
        return pw_array_append(result, str);
    }
    uint8_t* str_end_ptr;
    uint8_t* str_start_ptr = _pw_string_start_end(str, &str_end_ptr);

    StrRChr2 fn_strrchr2 = _pw_strrchr2_variants[str_char_size];

    if (!maxsplit) {
        maxsplit = UINT_MAX;
    }
    if (str_start_ptr == str_end_ptr) {
        return pw_array_append(result, str);
    }
    while (str_start_ptr < str_end_ptr) {
        uint8_t* substr_start;
        uint8_t substr_char_size;
        if (maxsplit--) {
            substr_start = fn_strrchr2(str_start_ptr, str_end_ptr, splitter, &substr_char_size);
            if (substr_start) {
                substr_start += str_char_size; // skip splitter
            } else {
                substr_start = str_start_ptr;
            }
        } else {
            substr_start = str_start_ptr;
            substr_char_size = 1;
        }
        // create substring
        unsigned substr_len = (str_end_ptr - substr_start) / str_char_size;
        PwValue substr = PW_NULL;
        if (!_pw_make_empty_string(str->type_id, substr_len, substr_char_size, &substr)) {
            return false;
        }
        if (substr_len) {
            _pw_string_set_length(&substr, substr_len);
            StrAppend fn_append = _pw_str_append_variants[substr_char_size][str_char_size];
            if (!fn_append(&substr, 0, substr_start, str_end_ptr)) {
                return false;
            }
        }
        if (!pw_array_insert(result, 0, &substr)) {
            return false;
        }
        str_end_ptr = substr_start - str_char_size;
    }
    return true;
}
