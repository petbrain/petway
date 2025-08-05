#include <string.h>

#include "include/pw.h"
#include "src/string/pw_string_internal.h"

static bool substreq_n_n(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    return 0 == memcmp(str_start_ptr, substr_start_ptr, substr_end_ptr - substr_start_ptr);
}

static bool substreq_1_2(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        if (*str_start_ptr++ != *((uint16_t*) substr_start_ptr)) {
            return false;
        }
        substr_start_ptr += 2;
    }
    return true;
}

static bool substreq_1_3(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        char32_t sub_chr = *substr_start_ptr++;
        sub_chr |= (*substr_start_ptr++) << 8;
        sub_chr |= (*substr_start_ptr++) << 16;
        if (*str_start_ptr++ != sub_chr) {
            return false;
        }
    }
    return true;
}

static bool substreq_1_4(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        if (*str_start_ptr++ != *((char32_t*) substr_start_ptr)) {
            return false;
        }
        substr_start_ptr += 4;
    }
    return true;
}


static bool substreq_2_1(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        if (*((uint16_t*) str_start_ptr) != *substr_start_ptr++) {
            return false;
        }
        str_start_ptr += 2;
    }
    return true;
}

static bool substreq_2_3(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        char32_t sub_chr = *substr_start_ptr++;
        sub_chr |= (*substr_start_ptr++) << 8;
        sub_chr |= (*substr_start_ptr++) << 16;
        if (*((uint16_t*) str_start_ptr) != sub_chr) {
            return false;
        }
        str_start_ptr += 2;
    }
    return true;
}

static bool substreq_2_4(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        if (*((uint16_t*) str_start_ptr) != *((char32_t*) substr_start_ptr)) {
            return false;
        }
        str_start_ptr += 2;
        substr_start_ptr += 4;
    }
    return true;
}


static bool substreq_3_1(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        char32_t chr = *str_start_ptr++;
        chr |= (*str_start_ptr++) << 8;
        chr |= (*str_start_ptr++) << 16;
        if (chr != *substr_start_ptr++) {
            return false;
        }
    }
    return true;
}

static bool substreq_3_2(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        char32_t chr = *str_start_ptr++;
        chr |= (*str_start_ptr++) << 8;
        chr |= (*str_start_ptr++) << 16;
        if (chr != *((uint16_t*) substr_start_ptr)) {
            return false;
        }
        substr_start_ptr += 2;
    }
    return true;
}

static bool substreq_3_4(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        char32_t chr = *str_start_ptr++;
        chr |= (*str_start_ptr++) << 8;
        chr |= (*str_start_ptr++) << 16;
        if (chr != *((char32_t*) substr_start_ptr)) {
            return false;
        }
        str_start_ptr += 4;
    }
    return true;
}


static bool substreq_4_1(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        if (*((char32_t*) str_start_ptr) != *substr_start_ptr++) {
            return false;
        }
        str_start_ptr += 4;
    }
    return true;
}

static bool substreq_4_2(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        if (*((char32_t*) str_start_ptr) != *((uint16_t*) substr_start_ptr)) {
            return false;
        }
        str_start_ptr += 4;
        substr_start_ptr += 2;
    }
    return true;
}

static bool substreq_4_3(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        char32_t sub_chr = *substr_start_ptr++;
        sub_chr |= (*substr_start_ptr++) << 8;
        sub_chr |= (*substr_start_ptr++) << 16;
        if (*((char32_t*) str_start_ptr) != sub_chr) {
            return false;
        }
        str_start_ptr += 4;
    }
    return true;
}

// Comparison with UTF-8 string.

#define CMP_UTF8(CHAR_TYPE, CHAR_SIZE)  \
    static bool substreq_##CHAR_SIZE##_0(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)  \
    {  \
        while(substr_start_ptr < substr_end_ptr) {  \
            char32_t c = _pw_decode_utf8_char(&substr_start_ptr);  \
            if (c != *((CHAR_TYPE*) str_start_ptr)) {  \
                return false;  \
            }  \
            str_start_ptr += CHAR_SIZE;  \
        }  \
        return true;  \
    }
CMP_UTF8(uint8_t,  1)
CMP_UTF8(uint16_t, 2)
CMP_UTF8(char32_t, 4)

static bool substreq_3_0(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while(substr_start_ptr < substr_end_ptr) {
        char32_t c1 = _pw_decode_utf8_char(&substr_start_ptr);
        char32_t c2 = *str_start_ptr++;
        c2 |= (*str_start_ptr++) << 8;
        c2 |= (*str_start_ptr++) << 16;
        if (c1 != c2) {
            return false;
        }
    }
    return true;
}


SubstrEq _pw_substreq_variants[5][5] = {
    {
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    },
    {
        substreq_1_0,
        substreq_n_n,
        substreq_1_2,
        substreq_1_3,
        substreq_1_4
    },
    {
        substreq_2_0,
        substreq_2_1,
        substreq_n_n,
        substreq_2_3,
        substreq_2_4
    },
    {
        substreq_3_0,
        substreq_3_1,
        substreq_3_2,
        substreq_n_n,
        substreq_3_4
    },
    {
        substreq_4_0,
        substreq_4_1,
        substreq_4_2,
        substreq_4_3,
        substreq_n_n
    }
};


/****************************************************************
 * High-level functions
 */

[[nodiscard]] bool _pw_substring_eq(PwValuePtr a, unsigned start_pos, unsigned end_pos, PwValuePtr b)
{
    pw_assert_string(a);
    unsigned a_length;
    uint8_t* a_ptr = _pw_string_start_length(a, &a_length);

    if (_pw_unlikely(end_pos > a_length)) {
        end_pos = a_length;
    }
    if (_pw_unlikely(end_pos < start_pos)) {
        return false;
    }
    unsigned substr_length = end_pos - start_pos;
    unsigned b_length = pw_strlen(b);
    if (_pw_unlikely(substr_length != b_length)) {
        return false;
    }
    if (_pw_unlikely(substr_length == 0)) {
        return true;
    }
    uint8_t* b_end_ptr;
    uint8_t* b_start_ptr = _pw_string_start_end(b, &b_end_ptr);
    SubstrEq fn_substreq = _pw_substreq_variants[a->char_size][b->char_size];
    return fn_substreq(a_ptr + start_pos * a->char_size, b_start_ptr, b_end_ptr);
}

[[nodiscard]] bool _pw_startswith_c32(PwValuePtr str, char32_t prefix)
{
    pw_assert_string(str);
    unsigned length = pw_strlen(str);
    if (_pw_unlikely(length == 0)) {
        return false;
    }
    return _pw_get_char(_pw_string_start(str), str->char_size) == prefix;
}

[[nodiscard]] bool _pw_startswith(PwValuePtr str, PwValuePtr prefix)
{
    return _pw_substring_eq(str, 0, pw_strlen(prefix), prefix);
}

[[nodiscard]] bool _pw_endswith_c32(PwValuePtr str, char32_t prefix)
{
    pw_assert_string(str);
    unsigned length;
    uint8_t* ptr = _pw_string_start_length(str, &length);
    if (_pw_unlikely(length == 0)) {
        return false;
    }
    uint8_t char_size = str->char_size;
    return prefix == _pw_get_char(ptr + (length - 1) * char_size, char_size);
}

[[nodiscard]] bool _pw_endswith(PwValuePtr str, PwValuePtr suffix)
{
    unsigned str_len = pw_strlen(str);
    unsigned suffix_len = pw_strlen(suffix);
    if (_pw_unlikely(str_len < suffix_len)) {
        return false;
    }
    return _pw_substring_eq(str, str_len - suffix_len, str_len, suffix);
}
