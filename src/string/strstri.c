#include "include/pw.h"
#include "src/string/pw_string_internal.h"

bool _pw_strstri(PwValuePtr str, PwValuePtr substr, unsigned start_pos, unsigned* pos)
{
    pw_assert_string(str);
    pw_assert_string(substr);

    uint8_t  substr_char_size = substr->char_size;
    uint8_t* substr_end_ptr;
    uint8_t* substr_start_ptr = _pw_string_start_end(substr, &substr_end_ptr);
    if (_pw_unlikely(substr_start_ptr >= substr_end_ptr)) {
        // empty substring
        return false;
    }

    uint8_t  str_char_size = str->char_size;
    uint8_t* str_end_ptr;
    uint8_t* str_start_ptr = _pw_string_start_end(str, &str_end_ptr);
    str_start_ptr += start_pos * str_char_size;

    char32_t start_codepoint = _pw_get_char(substr_start_ptr, substr_char_size);
    if (_pw_unlikely(start_codepoint >= (1ULL << (8 * str_char_size)))) {
        return false;
    }

    StrChr   fn_strchri   = _pw_strchri_variants[str_char_size];
    SubstrEq fn_substreqi = _pw_substreqi_variants[str_char_size][substr_char_size];

    str_end_ptr -= substr_end_ptr - substr_start_ptr - 1;  // reduce by strlen(substr) - 1
    for (uint8_t* str_ptr = str_start_ptr; str_start_ptr < str_end_ptr; ) {

        uint8_t* maybe_substr = fn_strchri(str_ptr, str_end_ptr, start_codepoint);
        if (!maybe_substr) {
            break;
        }
        if (fn_substreqi(maybe_substr, substr_start_ptr, substr_end_ptr)) {
            if (pos) {
                *pos = start_pos + (maybe_substr - str_start_ptr) / str_char_size;
            }
            return true;
        }
        str_ptr = maybe_substr + str_char_size;
    }
    return false;
}
