#include <string.h>

#include "include/pw.h"
#include "src/string/pw_string_internal.h"


static uint8_t* strchr_1(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint)
{
    return memchr(start_ptr, (uint8_t) codepoint, end_ptr - start_ptr);
}

static uint8_t* strchr_2(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint)
{
    while (start_ptr < end_ptr) {
        if (codepoint == *((uint16_t*) start_ptr)) {
            return start_ptr;
        }
        start_ptr += 2;
    }
    return nullptr;
}

static uint8_t* strchr_3(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint)
{
    while (start_ptr < end_ptr) {
        char32_t c = *start_ptr++;
        c |= (*start_ptr++) << 8;
        c |= (*start_ptr++) << 16;

        if (c == codepoint) {
            return start_ptr - 3;
        }
    }
    return nullptr;
}

static uint8_t* strchr_4(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint)
{
    while (start_ptr < end_ptr) {
        if (codepoint == *((char32_t*) start_ptr)) {
            return start_ptr;
        }
        start_ptr += 4;
    }
    return nullptr;
}

StrChr _pw_strchr_variants[5] = {
    nullptr,
    strchr_1,
    strchr_2,
    strchr_3,
    strchr_4
};

[[ nodiscard]] bool pw_strchr(PwValuePtr str, char32_t chr, unsigned start_pos, unsigned* result)
{
    pw_assert_string(str);
    uint8_t char_size = str->char_size;
    if (_pw_unlikely(char_size < calc_char_size(chr))) {
        return false;
    }
    uint8_t* end_ptr;
    uint8_t* start_ptr = _pw_string_start_end(str, &end_ptr) + start_pos * char_size;
    if (_pw_unlikely(start_ptr >= end_ptr)) {
        return false;
    }
    StrChr fn_strchr = _pw_strchr_variants[char_size];
    uint8_t* char_ptr = fn_strchr(start_ptr, end_ptr, chr);
    if (_pw_unlikely(!char_ptr)) {
        return false;
    }
    if (result) {
        *result = start_pos + (char_ptr - start_ptr) / char_size;
    }
    return true;
}
