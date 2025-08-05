#include "include/pw.h"
#include "src/string/pw_string_internal.h"


static uint8_t* skip_spaces_1(uint8_t* start_ptr, uint8_t* end_ptr)
{
    while (start_ptr < end_ptr) {
        if (!isspace(*start_ptr)) {
            break;
        }
        start_ptr++;
    }
    return start_ptr;
}

static uint8_t* skip_spaces_2(uint8_t* start_ptr, uint8_t* end_ptr)
{
    while (start_ptr < end_ptr) {
        if (!pw_isspace(*(uint16_t*) start_ptr)) {
            break;
        }
        start_ptr += 2;
    }
    return start_ptr;
}

static uint8_t* skip_spaces_3(uint8_t* start_ptr, uint8_t* end_ptr)
{
    while (start_ptr < end_ptr) {
        char32_t c = *start_ptr++;
        c |= (*start_ptr++) << 8;
        c |= (*start_ptr++) << 16;

        if (!pw_isspace(c)) {
            start_ptr -= 3;
            break;
        }
    }
    return start_ptr;
}

static uint8_t* skip_spaces_4(uint8_t* start_ptr, uint8_t* end_ptr)
{
    while (start_ptr < end_ptr) {
        if (!pw_isspace(*(char32_t*) start_ptr)) {
            break;
        }
        start_ptr += 4;
    }
    return start_ptr;
}

StrSkipSpaces _pw_skip_spaces_variants[5] = {
    nullptr,
    skip_spaces_1,
    skip_spaces_2,
    skip_spaces_3,
    skip_spaces_4
};

unsigned pw_string_skip_spaces(PwValuePtr str, unsigned position)
{
    pw_assert_string(str);
    uint8_t str_char_size = str->char_size;
    uint8_t* str_end_ptr;
    uint8_t* str_start_ptr = _pw_string_start_end(str, &str_end_ptr) + position * str_char_size;
    StrSkipSpaces fn_skip_spaces = _pw_skip_spaces_variants[str_char_size];
    return position + (fn_skip_spaces(str_start_ptr, str_end_ptr) - str_start_ptr) / str_char_size;
}
