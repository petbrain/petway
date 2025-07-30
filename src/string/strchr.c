#include <string.h>  // for memchr

#include "include/pw.h"
#include "src/string/pw_string_internal.h"

static uint8_t* strchr_1(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint)
{
    return memchr(start_ptr, (char) codepoint, end_ptr - start_ptr);
}

static uint8_t* strchr_2(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint)
{
    while (start_ptr < end_ptr) {
        if (((uint16_t) codepoint) == *((uint16_t*) start_ptr)) {
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
