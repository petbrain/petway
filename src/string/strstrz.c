#include "include/pw.h"
#include "src/string/pw_string_internal.h"

bool _pw_strstr_z(PwValuePtr str, void* substr, uint8_t substr_char_size, unsigned start_pos, unsigned* pos)
{
    unsigned str_len = pw_strlen(str);
    if (_pw_unlikely(start_pos >= str_len)) {
        return false;
    }
    uint8_t  str_char_size = str->char_size;
    uint8_t* str_end_ptr;
    uint8_t* str_start_ptr = _pw_string_start_end(str, &str_end_ptr);
    str_start_ptr += start_pos * str_char_size;
    str_len -= start_pos;

    char32_t start_codepoint = _pw_get_char((uint8_t*) substr, substr_char_size);
    if (_pw_unlikely(start_codepoint == 0 || start_codepoint >= (1ULL << (8 * str_char_size)))) {
        return false;
    }

    StrChr    fn_strchr = _pw_strchr_variants[str_char_size];
    StrEqualZ fn_equalz = _pw_str_equalz_variants[str_char_size][substr_char_size];

    for (uint8_t* str_ptr = str_start_ptr; str_start_ptr < str_end_ptr; ) {

        uint8_t* maybe_substr = fn_strchr(str_ptr, str_end_ptr, start_codepoint);
        if (!maybe_substr) {
            break;
        }
        unsigned offset = (maybe_substr - str_start_ptr) / str_char_size;
        if (fn_equalz(maybe_substr, str_len - offset, (uint8_t*) substr) != PW_NEQ) {  // partial and full matches are ok
            if (pos) {
                *pos = start_pos + offset;
            }
            return true;
        }
        str_ptr = maybe_substr + str_char_size;
        str_len--;
    }
    return false;
}
