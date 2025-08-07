#include "include/pw.h"
#include "src/string/pw_string_internal.h"

#define TRANSFORM(TFUNC, CHAR_TYPE, CHAR_SIZE)  \
    static bool str_##TFUNC##_##CHAR_SIZE(PwValuePtr str)  \
    {  \
        uint8_t* end_ptr;  \
        uint8_t* start_ptr = _pw_string_start_end(str, &end_ptr);  \
        if (str->embedded) {  \
            while (start_ptr < end_ptr) {  \
                *((CHAR_TYPE*) start_ptr) = pw_char_##TFUNC(*(CHAR_TYPE*) start_ptr);  \
                start_ptr += CHAR_SIZE;  \
            }  \
            return true;  \
        }  \
        unsigned offset = 0;  \
        while (start_ptr < end_ptr) {  \
            CHAR_TYPE c = *(CHAR_TYPE*) start_ptr;  \
            char32_t new_c = pw_char_##TFUNC(c);  \
            if (new_c != c) {  \
                if (!_pw_string_copy_on_write(str)) {  \
                    return false;  \
                }  \
                start_ptr = _pw_string_start_end(str, &end_ptr) + offset;  \
                for (;;) {  \
                    *((CHAR_TYPE*) start_ptr) = new_c;  \
                    start_ptr += CHAR_SIZE;  \
                    if (start_ptr >= end_ptr) {  \
                        return true;  \
                    }  \
                    c = *(CHAR_TYPE*) start_ptr;  \
                    new_c = pw_char_##TFUNC(c);  \
                }  \
            }  \
            start_ptr += CHAR_SIZE;  \
            offset += CHAR_SIZE;  \
        }  \
        return true;  \
    }
TRANSFORM(upper, uint8_t,  1)
TRANSFORM(lower, uint8_t,  1)
TRANSFORM(upper, uint16_t, 2)
TRANSFORM(lower, uint16_t, 2)
TRANSFORM(upper, char32_t, 4)
TRANSFORM(lower, char32_t, 4)

#define TRANSFORM3(TFUNC)  \
    static bool str_##TFUNC##_3(PwValuePtr str)  \
    {  \
        uint8_t* end_ptr;  \
        uint8_t* start_ptr = _pw_string_start_end(str, &end_ptr);  \
        if (str->embedded) {  \
            while (start_ptr < end_ptr) {  \
                char32_t c = *start_ptr++;  \
                c |= (*start_ptr++) << 8;  \
                c |= (*start_ptr++) << 16;  \
                char32_t new_c = pw_char_##TFUNC(c);  \
                if (new_c != c ) {  \
                    start_ptr -= 3;  \
                    *start_ptr++ = new_c; new_c >>= 8;  \
                    *start_ptr++ = new_c; new_c >>= 8;  \
                    *start_ptr++ = new_c;  \
                }  \
            }  \
            return true;  \
        }  \
        unsigned offset = 0;  \
        while (start_ptr < end_ptr) {  \
            char32_t c = *start_ptr++;  \
            c |= (*start_ptr++) << 8;  \
            c |= (*start_ptr++) << 16;  \
            char32_t new_c = pw_char_##TFUNC(c);  \
            if (new_c != c) {  \
                if (!_pw_string_copy_on_write(str)) {  \
                    return false;  \
                }  \
                start_ptr = _pw_string_start_end(str, &end_ptr) + offset;  \
                for (;;) {  \
                    *start_ptr++ = new_c; new_c >>= 8;  \
                    *start_ptr++ = new_c; new_c >>= 8;  \
                    *start_ptr++ = new_c;  \
                    if (start_ptr >= end_ptr) {  \
                        return true;  \
                    }  \
                    start_ptr -= 3;  \
                    c = *start_ptr++;  \
                    c |= (*start_ptr++) << 8;  \
                    c |= (*start_ptr++) << 16;  \
                    new_c = pw_char_##TFUNC(c);  \
                }  \
            }  \
            start_ptr += 3;  \
            offset += 3;  \
        }  \
        return true;  \
    }
TRANSFORM3(upper)
TRANSFORM3(lower)

typedef bool (*StrTransform)(PwValuePtr str);

static StrTransform _pw_strupper_variants[5] = {
    nullptr,
    str_upper_1,
    str_upper_2,
    str_upper_3,
    str_upper_4
};

static StrTransform _pw_strlower_variants[5] = {
    nullptr,
    str_lower_1,
    str_lower_2,
    str_lower_3,
    str_lower_4
};

[[ nodiscard]] bool pw_string_lower(PwValuePtr str)
{
    pw_assert_string(str);
    StrTransform fn_lower = _pw_strlower_variants[str->char_size];
    return fn_lower(str);
}

[[ nodiscard]] bool pw_string_upper(PwValuePtr str)
{
    pw_assert_string(str);
    StrTransform fn_upper = _pw_strupper_variants[str->char_size];
    return fn_upper(str);
}
