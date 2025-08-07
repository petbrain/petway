#include "include/pw.h"
#include "src/string/pw_string_internal.h"


[[nodiscard]] bool _pw_substring_eqi_z(PwValuePtr a, unsigned start_pos, unsigned end_pos, void* b, uint8_t b_char_size)
{
    pw_assert_string(a);
    unsigned a_length;
    uint8_t* a_ptr = _pw_string_start_length(a, &a_length);

    if (end_pos > a_length) {
        end_pos = a_length;
    }
    if (end_pos < start_pos) {
        return false;
    }
    if (_pw_unlikely(b_char_size == 4)) {
        if (0 == *(char32_t*) b) {
            return start_pos == end_pos;
        }
    } else {
        if (0 == *(char*) b) {;
            return start_pos == end_pos;
        }
    }
    uint8_t char_size = a->char_size;
    StrEqualZ fn_equalzi = _pw_str_equalzi_variants[char_size][b_char_size];
    int match = fn_equalzi(a_ptr + start_pos * char_size, end_pos - start_pos, b);
    return (match == PW_NEQ)? false : true;  // partial match means equal
}

[[nodiscard]] bool _pw_startswithi_z(PwValuePtr str, void* prefix, uint8_t prefix_char_size)
{
    pw_assert_string(str);
    unsigned length;
    uint8_t* str_start_ptr = _pw_string_start_length(str, &length);
    StrEqualZ fn_equalzi = _pw_str_equalzi_variants[str->char_size][prefix_char_size];
    int match = fn_equalzi(str_start_ptr, length, prefix);
    return (match == PW_NEQ)? false : true;  // partial match means equal
}

[[nodiscard]] bool _pw_endswithi_z(PwValuePtr str, void* suffix, uint8_t suffix_char_size)
{
    pw_assert_string(str);
    unsigned suffix_len;
    switch (suffix_char_size) {
        case 0: suffix_len = utf8_strlen(suffix); break;
        case 1: suffix_len = strlen(suffix); break;
        case 4: suffix_len = utf32_strlen(suffix); break;
        default:
            pw_panic("Bad char size for suffix");
    }
    unsigned str_len;
    uint8_t* str_start_ptr = _pw_string_start_length(str, &str_len);
    if (_pw_unlikely(str_len < suffix_len)) {
        return false;
    }
    uint8_t str_char_size = str->char_size;
    StrEqualZ fn_equalzi = _pw_str_equalzi_variants[str_char_size][suffix_char_size];
    int match = fn_equalzi(str_start_ptr + (str_len - suffix_len) * str_char_size, suffix_len, suffix);
    return (match == PW_EQ)? true : false;  // nust be exact match
}
