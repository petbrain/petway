#include <string.h>

#include "include/pw.h"
#include "src/string/pw_string_internal.h"


static uint8_t* strchr2_1(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint, uint8_t* char_size)
{
    *char_size = 1;
    return memchr(start_ptr, (uint8_t) codepoint, end_ptr - start_ptr);
}

static uint8_t* strchr2_2(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint, uint8_t* char_size)
{
    char32_t width = 0;
    while (start_ptr < end_ptr) {
        char32_t c = *(uint16_t*) start_ptr;
        width |= c;
        if (c == codepoint) {
            *char_size = calc_char_size(width);
            return start_ptr;
        }
        start_ptr += 2;
    }
    *char_size = calc_char_size(width);
    return nullptr;
}

static uint8_t* strchr2_3(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint, uint8_t* char_size)
{
    char32_t width = 0;
    while (start_ptr < end_ptr) {
        char32_t c = *start_ptr++;
        c |= (*start_ptr++) << 8;
        c |= (*start_ptr++) << 16;
        width |= c;
        if (c == codepoint) {
            *char_size = calc_char_size(width);
            return start_ptr - 3;
        }
    }
    *char_size = calc_char_size(width);
    return nullptr;
}

static uint8_t* strchr2_4(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint, uint8_t* char_size)
{
    char32_t width = 0;
    while (start_ptr < end_ptr) {
        char32_t c = *(char32_t*) start_ptr;
        width |= c;
        if (c == codepoint) {
            *char_size = calc_char_size(width);
            return start_ptr;
        }
        start_ptr += 4;
    }
    *char_size = calc_char_size(width);
    return nullptr;
}

StrChr2 _pw_strchr2_variants[5] = {
    nullptr,
    strchr2_1,
    strchr2_2,
    strchr2_3,
    strchr2_4
};

[[ nodiscard]] bool pw_strchr2(PwValuePtr str, char32_t chr, unsigned start_pos, unsigned* result, uint8_t* max_char_size)
{
    pw_assert_string(str);
    uint8_t char_size = str->char_size;
    uint8_t* end_ptr;
    uint8_t* start_ptr = _pw_string_start_end(str, &end_ptr) + start_pos * char_size;
    if (_pw_unlikely(start_ptr >= end_ptr)) {
        *max_char_size = 1;
        return false;
    }
    StrChr2 fn_strchr2 = _pw_strchr2_variants[char_size];
    return fn_strchr2(start_ptr, end_ptr, chr, max_char_size);
}
