#include "include/pw.h"
#include "src/string/pw_string_internal.h"

[[nodiscard]] bool pw_substr(PwValuePtr str, unsigned start_pos, unsigned end_pos, PwValuePtr result)
{
    pw_assert_string(str);

    unsigned length;
    uint8_t* start_ptr = _pw_string_start_length(str, &length);

    if (end_pos > length) {
        end_pos = length;
    }
    if (start_pos >= end_pos) {
        length = 0;
    } else {
        length = end_pos - start_pos;
    }
    if (!_pw_make_empty_string(str->type_id, length, 1, result)) {
        return false;
    }
    if (length) {
        _pw_string_set_length(result, length);
        uint8_t char_size = str->char_size;
        StrAppend fn_append = _pw_str_append_variants[1][char_size];
        if (!fn_append(result, 0, start_ptr + start_pos * char_size, start_ptr + end_pos * char_size)) {
            return false;
        }
    }
    return true;
}
