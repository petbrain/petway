#include "include/pw.h"
#include "src/string/pw_string_internal.h"

[[ nodiscard]] bool pw_string_erase(PwValuePtr str, unsigned start_pos, unsigned end_pos)
{
    unsigned length = pw_strlen(str);
    if (start_pos >= length || start_pos >= end_pos) {
        return true;
    }
    if (!_pw_string_copy_on_write(str)) {
        return false;
    }
    if (end_pos >= length) {
        // truncate
        length = start_pos;
    } else {
        // erase in-place (simpler approach)
        // XXX or make substring by concatenating parts?
        uint8_t* start_ptr = _pw_string_start(str);
        uint8_t char_size = str->char_size;
        memmove(start_ptr + start_pos * char_size, start_ptr + end_pos * char_size, (length - end_pos) * char_size);
        length -= end_pos - start_pos;
    }
    _pw_string_set_length(str, length);

    if (str->embedded) {
        // clean remainder for fast comparison to work
        unsigned i = str->char_size * length;
        while (i < sizeof(str->str_1)) {
            str->str_1[i++] = 0;
        }
    }
    return true;
}
