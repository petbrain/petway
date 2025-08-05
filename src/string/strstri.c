#include "include/pw.h"
#include "src/string/pw_string_internal.h"

// XXX optimize

bool _pw_strstri(PwValuePtr str, PwValuePtr substr, unsigned start_pos, unsigned* pos)
{
    pw_assert_string(str);
    pw_assert_string(substr);
    PW_STRING_ITER(isub, substr);
    if (PW_STRING_ITER_DONE(isub)) {
        // zero length substr
        return false;
    }
    char32_t start_codepoint = PW_STRING_ITER_NEXT(isub);
    PW_STRING_ITER_FROM(istr, str, start_pos);
    while (!PW_STRING_ITER_DONE(istr)) {
        char32_t codepoint = PW_STRING_ITER_NEXT(istr);
        if (pw_char_lower(codepoint) == pw_char_lower(start_codepoint)) {
            for (;;) {
                bool str_done = PW_STRING_ITER_DONE(istr);
                bool sub_done = PW_STRING_ITER_DONE(isub);
                if (sub_done) {
                    if (pos) {
                        *pos = start_pos;
                    }
                    return true;
                }
                if (str_done) {
            not_a_match:
                    PW_STRING_ITER_RESET_FROM(isub, substr, 1);
                    PW_STRING_ITER_RESET_FROM(istr, str, start_pos);
                    break;
                }
                char32_t a = PW_STRING_ITER_NEXT(istr);
                char32_t b = PW_STRING_ITER_NEXT(isub);
                if (pw_char_lower(a) != pw_char_lower(b)) {
                    goto not_a_match;
                }
            }
        }
        start_pos++;
    }
    return false;
}
