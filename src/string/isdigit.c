#include "include/pw.h"
#include "src/string/pw_string_internal.h"

#define is_ascii_digit(c) ('0' <= (c) && (c) <= '9')

static bool is_digit_1(uint8_t* start_ptr, uint8_t* end_ptr)
{
    while (start_ptr < end_ptr) {
        uint8_t c = *start_ptr++;
        if (!is_ascii_digit(c)) {
            return false;
        }
    }
    return true;
}

#define IS_DIGIT(CHAR_TYPE, CHAR_SIZE)  \
    static bool is_digit_##CHAR_SIZE(uint8_t* start_ptr, uint8_t* end_ptr)  \
    {  \
        while (start_ptr < end_ptr) {  \
            char32_t c = *(CHAR_TYPE*) start_ptr;  \
            if (!pw_isdigit(c)) {  \
                return false;  \
            }  \
            start_ptr += CHAR_SIZE;  \
        }  \
        return true;  \
    }
IS_DIGIT(uint16_t, 2)
IS_DIGIT(char32_t, 4)

static bool is_digit_3(uint8_t* start_ptr, uint8_t* end_ptr)
{
    while (start_ptr < end_ptr) {
        char32_t c = *start_ptr++;
        c |= (*start_ptr++) << 8;
        c |= (*start_ptr++) << 16;
        if (!pw_isdigit(c)) {
            return false;
        }
    }
    return true;
}

typedef bool (*StrIsDigit)(uint8_t* start_ptr, uint8_t* end_ptr);

static StrIsDigit is_digit_variants[5] = {
    nullptr,
    is_digit_1,
    is_digit_2,
    is_digit_3,
    is_digit_4
};

bool pw_string_isdigit(PwValuePtr str)
{
    pw_assert_string(str);
    uint8_t* end_ptr;
    uint8_t* start_ptr = _pw_string_start_end(str, &end_ptr);
    if (_pw_unlikely(start_ptr == end_ptr)) {
        return false;
    }
    StrIsDigit fn_is_digit = is_digit_variants[str->char_size];
    return fn_is_digit(start_ptr, end_ptr);
}
