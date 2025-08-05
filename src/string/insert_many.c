#include "include/pw.h"
#include "src/string/pw_string_internal.h"

[[ nodiscard]] bool _pw_string_insert_many_c32(PwValuePtr str, unsigned position, char32_t chr, unsigned n)
{
    if (n == 0) {
        return true;
    }
    pw_assert(position <= pw_strlen(str));

    if (!_pw_expand_string(str, n, calc_char_size(chr))) {
        return false;
    }
    unsigned len = _pw_string_inc_length(str, n);
    uint8_t char_size = str->char_size;
    uint8_t* insertion_ptr = _pw_string_start(str) + position * char_size;

    if (position < len) {
        memmove(insertion_ptr + n * char_size, insertion_ptr, (len - position) * char_size);
    }
    // XXX optimize
    for (unsigned i = 0; i < n; i++) {
        _pw_put_char(insertion_ptr, chr, char_size);
        insertion_ptr += char_size;
    }
    return true;
}
