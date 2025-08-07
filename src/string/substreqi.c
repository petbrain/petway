#include "include/pw.h"
#include "src/string/pw_string_internal.h"

#define SUBSTREQI(STR_CHAR_TYPE, STR_CHAR_SIZE, SUBSTR_CHAR_TYPE, SUBSTR_CHAR_SIZE)  \
    static bool substreqi_##STR_CHAR_SIZE##_##SUBSTR_CHAR_SIZE(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)  \
    {  \
        while (substr_start_ptr < substr_end_ptr) {  \
            if (pw_char_lower(*((STR_CHAR_TYPE*) str_start_ptr)) != pw_char_lower(*((SUBSTR_CHAR_TYPE*) substr_start_ptr))) {  \
                return false;  \
            }  \
            str_start_ptr += STR_CHAR_SIZE;  \
            substr_start_ptr += SUBSTR_CHAR_SIZE;  \
        }  \
        return true;  \
    }
SUBSTREQI(uint8_t,  1,  uint8_t,  1)
SUBSTREQI(uint8_t,  1,  uint16_t, 2)
SUBSTREQI(uint8_t,  1,  char32_t, 4)
SUBSTREQI(uint16_t, 2,  uint8_t,  1)
SUBSTREQI(uint16_t, 2,  uint16_t, 2)
SUBSTREQI(uint16_t, 2,  char32_t, 4)
SUBSTREQI(char32_t, 4,  uint8_t,  1)
SUBSTREQI(char32_t, 4,  uint16_t, 2)
SUBSTREQI(char32_t, 4,  char32_t, 4)

#define SUBSTREQI_N_3(STR_CHAR_TYPE, STR_CHAR_SIZE)  \
    static bool substreqi_##STR_CHAR_SIZE##_3(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)  \
    {  \
        while (substr_start_ptr < substr_end_ptr) {  \
            char32_t c = *substr_start_ptr++;  \
            c |= (*substr_start_ptr++) << 8;  \
            c |= (*substr_start_ptr++) << 16;  \
            if (pw_char_lower(*((STR_CHAR_TYPE*) str_start_ptr)) != pw_char_lower(c)) {  \
                return false;  \
            }  \
            str_start_ptr += STR_CHAR_SIZE;  \
        }  \
        return true;  \
    }
SUBSTREQI_N_3(uint8_t,  1)
SUBSTREQI_N_3(uint16_t, 2)
SUBSTREQI_N_3(char32_t, 4)

#define SUBSTREQI_3_N(SUBSTR_CHAR_TYPE, SUBSTR_CHAR_SIZE)  \
    static bool substreqi_3_##SUBSTR_CHAR_SIZE(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)  \
    {  \
        while (substr_start_ptr < substr_end_ptr) {  \
            char32_t c = *str_start_ptr++;  \
            c |= (*str_start_ptr++) << 8;  \
            c |= (*str_start_ptr++) << 16;  \
            if (pw_char_lower(c) != pw_char_lower(*((SUBSTR_CHAR_TYPE*) substr_start_ptr))) {  \
                return false;  \
            }  \
            substr_start_ptr += SUBSTR_CHAR_SIZE;  \
        }  \
        return true;  \
    }
SUBSTREQI_3_N(uint8_t,  1)
SUBSTREQI_3_N(uint16_t, 2)
SUBSTREQI_3_N(char32_t, 4)

static bool substreqi_3_3(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        char32_t c1 = *str_start_ptr++;
        c1 |= (*str_start_ptr++) << 8;
        c1 |= (*str_start_ptr++) << 16;
        char32_t c2 = *substr_start_ptr++;  \
        c2 |= (*substr_start_ptr++) << 8;  \
        c2 |= (*substr_start_ptr++) << 16;  \
        if (pw_char_lower(c1) != pw_char_lower(c2)) {
            return false;
        }
    }
    return true;
}

// Comparison with UTF-8 string.

#define SUBSTREQ_UTF8(CHAR_TYPE, CHAR_SIZE)  \
    static bool substreqi_##CHAR_SIZE##_0(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)  \
    {  \
        while(substr_start_ptr < substr_end_ptr) {  \
            char32_t c = _pw_decode_utf8_char(&substr_start_ptr);  \
            if (pw_char_lower(c) != pw_char_lower(*((CHAR_TYPE*) str_start_ptr))) {  \
                return false;  \
            }  \
            str_start_ptr += CHAR_SIZE;  \
        }  \
        return true;  \
    }
SUBSTREQ_UTF8(uint8_t,  1)
SUBSTREQ_UTF8(uint16_t, 2)
SUBSTREQ_UTF8(char32_t, 4)

static bool substreqi_3_0(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while(substr_start_ptr < substr_end_ptr) {
        char32_t c1 = _pw_decode_utf8_char(&substr_start_ptr);
        char32_t c2 = *str_start_ptr++;
        c2 |= (*str_start_ptr++) << 8;
        c2 |= (*str_start_ptr++) << 16;
        if (pw_char_lower(c1) != pw_char_lower(c2)) {
            return false;
        }
    }
    return true;
}


SubstrEq _pw_substreqi_variants[5][5] = {
    {
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    },
    {
        substreqi_1_0,
        substreqi_1_1,
        substreqi_1_2,
        substreqi_1_3,
        substreqi_1_4
    },
    {
        substreqi_2_0,
        substreqi_2_1,
        substreqi_2_2,
        substreqi_2_3,
        substreqi_2_4
    },
    {
        substreqi_3_0,
        substreqi_3_1,
        substreqi_3_2,
        substreqi_3_3,
        substreqi_3_4
    },
    {
        substreqi_4_0,
        substreqi_4_1,
        substreqi_4_2,
        substreqi_4_3,
        substreqi_4_4
    }
};


/****************************************************************
 * High-level functions
 */

[[nodiscard]] bool _pw_substring_eqi(PwValuePtr a, unsigned start_pos, unsigned end_pos, PwValuePtr b)
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
    SubstrEq fn_substreqi = _pw_substreqi_variants[a->char_size][b->char_size];
    return fn_substreqi(a_ptr + start_pos * a->char_size, b_start_ptr, b_end_ptr);
}

[[nodiscard]] bool _pw_startswithi_c32(PwValuePtr str, char32_t prefix)
{
    pw_assert_string(str);
    unsigned length = pw_strlen(str);
    if (_pw_unlikely(length == 0)) {
        return false;
    }
    return pw_char_lower(_pw_get_char(_pw_string_start(str), str->char_size)) == pw_char_lower(prefix);
}

[[nodiscard]] bool _pw_startswithi(PwValuePtr str, PwValuePtr prefix)
{
    return _pw_substring_eqi(str, 0, pw_strlen(prefix), prefix);
}

[[nodiscard]] bool _pw_endswithi_c32(PwValuePtr str, char32_t prefix)
{
    pw_assert_string(str);
    unsigned length;
    uint8_t* ptr = _pw_string_start_length(str, &length);
    if (_pw_unlikely(length == 0)) {
        return false;
    }
    uint8_t char_size = str->char_size;
    return pw_char_lower(prefix) == pw_char_lower(_pw_get_char(ptr + (length - 1) * char_size, char_size));
}

[[nodiscard]] bool _pw_endswithi(PwValuePtr str, PwValuePtr suffix)
{
    unsigned str_len = pw_strlen(str);
    unsigned suffix_len = pw_strlen(suffix);
    if (_pw_unlikely(str_len < suffix_len)) {
        return false;
    }
    return _pw_substring_eqi(str, str_len - suffix_len, str_len, suffix);
}
