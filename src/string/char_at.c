#include "include/pw.h"
#include "src/string/pw_string_internal.h"

char32_t pw_char_at(PwValuePtr str, unsigned position)
{
    pw_assert_string(str);

    uint8_t char_size = str->char_size;
    uint8_t* char_ptr = _pw_string_char_ptr(str, position);
    if (char_ptr) {
        return _pw_get_char(char_ptr, char_size);
    } else {
        return 0;
    }
}
