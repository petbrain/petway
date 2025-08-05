#include "include/pw.h"
#include "src/string/pw_string_internal.h"

[[ nodiscard]] bool pw_string_truncate(PwValuePtr str, unsigned position)
{
    if (position >= pw_strlen(str)) {
        return true;
    }
    if (!_pw_string_copy_on_write(str)) {
        return false;
    }
    _pw_string_set_length(str, position);

    if (str->embedded) {
        // clean remainder for fast comparison to work
        unsigned i = str->char_size * position;
        while (i < sizeof(str->str_1)) {
            str->str_1[i++] = 0;
        }
    }
    return true;
}
