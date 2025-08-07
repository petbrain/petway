#include "include/pw.h"
#include "src/string/pw_string_internal.h"

#define SUBSTREQI(CHAR_TYPE, CHAR_SIZE)  \
    static uint8_t* strchri_##CHAR_SIZE(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint)  \
    {  \
        while (start_ptr < end_ptr) {  \
            if (codepoint == (char32_t) pw_char_lower(*((CHAR_TYPE*) start_ptr))) {  \
                return start_ptr;  \
            }  \
            start_ptr += CHAR_SIZE;  \
        }  \
        return nullptr;  \
    }
SUBSTREQI(uint8_t,  1)
SUBSTREQI(uint16_t, 2)
SUBSTREQI(char32_t, 4)

static uint8_t* strchri_3(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint)
{
    while (start_ptr < end_ptr) {
        char32_t c = *start_ptr++;
        c |= (*start_ptr++) << 8;
        c |= (*start_ptr++) << 16;

        if (codepoint == (char32_t) pw_char_lower(c)) {
            return start_ptr - 3;
        }
    }
    return nullptr;
}

StrChr _pw_strchri_variants[5] = {
    nullptr,
    strchri_1,
    strchri_2,
    strchri_3,
    strchri_4
};

[[ nodiscard]] bool pw_strchri(PwValuePtr str, char32_t chr, unsigned start_pos, unsigned* result)
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
    StrChr fn_strchri = _pw_strchri_variants[char_size];
    uint8_t* char_ptr = fn_strchri(start_ptr, end_ptr, pw_char_lower(chr));
    if (_pw_unlikely(!char_ptr)) {
        return false;
    }
    if (result) {
        *result = start_pos + (char_ptr - start_ptr) / char_size;
    }
    return true;
}
