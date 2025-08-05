#include "include/pw.h"
#include "src/string/pw_string_internal.h"

// XXX optimize

[[ nodiscard]] bool pw_string_lower(PwValuePtr str)
{
    pw_assert_string(str);

    if (str->embedded) {
        // embedded strings never need a copy on write
        PW_STRING_ITER(src, str);
        while (!PW_STRING_ITER_DONE(src)) {
            char32_t c = PW_STRING_ITER_GETCHAR(src);
            char32_t lower_c = pw_char_lower(c);
            if (lower_c != c ) {
                PW_STRING_ITER_PUTCHAR(src, lower_c);
            }
            PW_STRING_ITER_INC(src);
        };
        return true;
    }

    unsigned start_pos = 0;
    PW_STRING_ITER(src, str);
    while (!PW_STRING_ITER_DONE(src)) {
        char32_t c = PW_STRING_ITER_GETCHAR(src);
        char32_t lower_c = pw_char_lower(c);
        if (lower_c != c ) {

            // copy string if necessary and proceed

            if (!_pw_string_copy_on_write(str)) {
                return false;
            }
            PW_STRING_ITER_RESET_FROM(src, str, start_pos);
            PW_STRING_ITER_PUTCHAR(src, lower_c);
            PW_STRING_ITER_INC(src);
            while (!PW_STRING_ITER_DONE(src)) {
                c = PW_STRING_ITER_GETCHAR(src);
                lower_c = pw_char_lower(c);
                if (lower_c != c ) {
                    PW_STRING_ITER_PUTCHAR(src, lower_c);
                }
                PW_STRING_ITER_INC(src);
            }
            return true;
        }
        start_pos++;
        PW_STRING_ITER_INC(src);
    }
    return true;
}

[[ nodiscard]] bool pw_string_upper(PwValuePtr str)
{
    pw_assert_string(str);

    if (str->embedded) {
        // embedded strings never need a copy on write
        PW_STRING_ITER(src, str);
        while (!PW_STRING_ITER_DONE(src)) {
            char32_t c = PW_STRING_ITER_GETCHAR(src);
            char32_t upper_c = pw_char_upper(c);
            if (upper_c != c ) {
                PW_STRING_ITER_PUTCHAR(src, upper_c);
            }
            PW_STRING_ITER_INC(src);
        };
        return true;
    }

    unsigned start_pos = 0;
    PW_STRING_ITER(src, str);
    while (!PW_STRING_ITER_DONE(src)) {
        char32_t c = PW_STRING_ITER_GETCHAR(src);
        char32_t upper_c = pw_char_upper(c);
        if (upper_c != c ) {

            // copy string if necessary and proceed

            if (!_pw_string_copy_on_write(str)) {
                return false;
            }
            PW_STRING_ITER_RESET_FROM(src, str, start_pos);
            PW_STRING_ITER_PUTCHAR(src, upper_c);
            PW_STRING_ITER_INC(src);
            while (!PW_STRING_ITER_DONE(src)) {
                c = PW_STRING_ITER_GETCHAR(src);
                upper_c = pw_char_upper(c);
                if (upper_c != c ) {
                    PW_STRING_ITER_PUTCHAR(src, upper_c);
                }
                PW_STRING_ITER_INC(src);
            }
            return true;
        }
        start_pos++;
        PW_STRING_ITER_INC(src);
    }
    return true;
}
