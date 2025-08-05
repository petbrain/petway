#define _GNU_SOURCE  // for memrchr
#include <string.h>

#include "include/pw.h"
#include "src/string/pw_string_internal.h"


static uint8_t* strrchr2_1(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint, uint8_t* char_size)
{
    *char_size = 1;
    return memrchr(start_ptr, (uint8_t) codepoint, end_ptr - start_ptr);
}

static uint8_t* strrchr2_2(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint, uint8_t* char_size)
{
    char32_t width = 0;
    while (start_ptr < end_ptr) {
        end_ptr -= 2;
        char32_t c = *(uint16_t*) end_ptr;
        width |= c;
        if (c == codepoint) {
            *char_size = calc_char_size(width);
            return end_ptr;
        }
    }
    *char_size = calc_char_size(width);
    return nullptr;
}

static uint8_t* strrchr2_3(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint, uint8_t* char_size)
{
    char32_t width = 0;
    while (start_ptr < end_ptr) {
        end_ptr -= 3;
        char32_t c = *end_ptr++;
        c |= (*end_ptr++) << 8;
        c |= (*end_ptr++) << 16;
        end_ptr -= 3;
        width |= c;
        if (c == codepoint) {
            *char_size = calc_char_size(width);
            return end_ptr;
        }
    }
    *char_size = calc_char_size(width);
    return nullptr;
}

static uint8_t* strrchr2_4(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint, uint8_t* char_size)
{
    char32_t width = 0;
    while (start_ptr < end_ptr) {
        end_ptr -= 4;
        char32_t c = *(char32_t*) end_ptr;
        width |= c;
        if (c == codepoint) {
            *char_size = calc_char_size(width);
            return end_ptr;
        }
    }
    *char_size = calc_char_size(width);
    return nullptr;
}

StrRChr2 _pw_strrchr2_variants[5] = {
    nullptr,
    strrchr2_1,
    strrchr2_2,
    strrchr2_3,
    strrchr2_4
};
