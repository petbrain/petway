#include "include/pw.h"
#include "src/string/pw_string_internal.h"

static uint8_t* skip_chars_1(uint8_t* start_ptr, uint8_t* end_ptr, char32_t* skipchars)
{
    while (start_ptr < end_ptr) {
        if (!utf32_strchr(skipchars, *start_ptr)) {
            break;
        }
        start_ptr++;
    }
    return start_ptr;
}

static uint8_t* skip_chars_2(uint8_t* start_ptr, uint8_t* end_ptr, char32_t* skipchars)
{
    while (start_ptr < end_ptr) {
        if (!utf32_strchr(skipchars, *(uint16_t*) start_ptr)) {
            break;
        }
        start_ptr += 2;
    }
    return start_ptr;
}

static uint8_t* skip_chars_3(uint8_t* start_ptr, uint8_t* end_ptr, char32_t* skipchars)
{
    while (start_ptr < end_ptr) {
        char32_t c = *start_ptr++;
        c |= (*start_ptr++) << 8;
        c |= (*start_ptr++) << 16;

        if (!utf32_strchr(skipchars, c)) {
            start_ptr -= 3;
            break;
        }
    }
    return start_ptr;
}

static uint8_t* skip_chars_4(uint8_t* start_ptr, uint8_t* end_ptr, char32_t* skipchars)
{
    while (start_ptr < end_ptr) {
        if (!utf32_strchr(skipchars, *(char32_t*) start_ptr)) {
            break;
        }
        start_ptr += 4;
    }
    return start_ptr;
}

StrSkipChars _pw_skip_chars_variants[5] = {
    nullptr,
    skip_chars_1,
    skip_chars_2,
    skip_chars_3,
    skip_chars_4
};

unsigned pw_string_skip_chars(PwValuePtr str, unsigned position, char32_t* skipchars)
{
    pw_assert_string(str);
    uint8_t str_char_size = str->char_size;
    uint8_t* str_end_ptr;
    uint8_t* str_start_ptr = _pw_string_start_end(str, &str_end_ptr) + position * str_char_size;
    StrSkipChars fn_skip_chars = _pw_skip_chars_variants[str_char_size];
    return position + (fn_skip_chars(str_start_ptr, str_end_ptr, skipchars) - str_start_ptr) / str_char_size;
}
