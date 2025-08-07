#include "include/pw.h"
#include "src/string/pw_string_internal.h"

#define EQUZ(A_CHAR_TYPE, A_CHAR_SIZE, B_CHAR_TYPE, B_CHAR_SIZE)  \
    static int streqz_##A_CHAR_SIZE##_##B_CHAR_SIZE(uint8_t* a_start_ptr, unsigned a_length, uint8_t* b_start_ptr)  \
    {  \
        while (a_length--) {  \
            char32_t b_chr = *(B_CHAR_TYPE*) b_start_ptr;  \
            if (!b_chr) {  \
                return PW_EQ_PARTIAL;  \
            }  \
            if (*(A_CHAR_TYPE*) a_start_ptr != b_chr) {  \
                return PW_NEQ;  \
            }  \
            a_start_ptr += A_CHAR_SIZE;  \
            b_start_ptr += B_CHAR_SIZE;  \
        }  \
        return (0 == *(B_CHAR_TYPE*) b_start_ptr)? PW_EQ : PW_NEQ;  \
    }
EQUZ(uint8_t,  1, uint8_t,  1)
EQUZ(uint8_t,  1, uint16_t, 2)
EQUZ(uint8_t,  1, char32_t, 4)
EQUZ(uint16_t, 2, uint8_t,  1)
EQUZ(uint16_t, 2, uint16_t, 2)
EQUZ(uint16_t, 2, char32_t, 4)
EQUZ(char32_t, 4, uint8_t,  1)
EQUZ(char32_t, 4, uint16_t, 2)
EQUZ(char32_t, 4, char32_t, 4)

#define EQUZ_N_3(A_CHAR_TYPE, A_CHAR_SIZE)  \
    static int streqz_##A_CHAR_SIZE##_3(uint8_t* a_start_ptr, unsigned a_length, uint8_t* b_start_ptr)  \
    {  \
        while (a_length--) {  \
            char32_t b_chr = *b_start_ptr++;  \
            b_chr |= (*b_start_ptr++) << 8;  \
            b_chr |= (*b_start_ptr++) << 16;  \
            if (!b_chr) {  \
                return PW_EQ_PARTIAL;  \
            }  \
            if (*(A_CHAR_TYPE*) a_start_ptr != b_chr) {  \
                return PW_NEQ;  \
            }  \
            a_start_ptr += A_CHAR_SIZE;  \
        }  \
        if (_pw_unlikely(*b_start_ptr++)) { return false; }  \
        if (_pw_unlikely(*b_start_ptr++)) { return false; }  \
        if (_pw_unlikely(*b_start_ptr)) { return false; }  \
        return PW_EQ;  \
    }
EQUZ_N_3(uint8_t,  1)
EQUZ_N_3(uint16_t, 2)
EQUZ_N_3(char32_t, 4)

#define EQUZ_3_N(B_CHAR_TYPE, B_CHAR_SIZE)  \
    static int streqz_3_##B_CHAR_SIZE(uint8_t* a_start_ptr, unsigned a_length, uint8_t* b_start_ptr)  \
    {  \
        while (a_length--) {  \
            char32_t b_chr = *(B_CHAR_TYPE*) b_start_ptr;  \
            if (!b_chr) {  \
                return PW_EQ_PARTIAL;  \
            }  \
            char32_t a_chr = *a_start_ptr++;  \
            a_chr |= (*a_start_ptr++) << 8;  \
            a_chr |= (*a_start_ptr++) << 16;  \
            if (a_chr != b_chr) {  \
                return PW_NEQ;  \
            }  \
        }  \
        return (0 == *(B_CHAR_TYPE*) b_start_ptr)? PW_EQ : PW_NEQ;  \
    }
EQUZ_3_N(uint8_t,  1)
EQUZ_3_N(uint16_t, 2)
EQUZ_3_N(char32_t, 4)

static int streqz_3_3(uint8_t* a_start_ptr, unsigned a_length, uint8_t* b_start_ptr)
{
    while (a_length--) {
        char32_t b_chr = *b_start_ptr++;
        b_chr |= (*b_start_ptr++) << 8;
        b_chr |= (*b_start_ptr++) << 16;
        if (!b_chr) {
            return PW_EQ_PARTIAL;
        }
        char32_t a_chr = *a_start_ptr++;
        a_chr |= (*a_start_ptr++) << 8;
        a_chr |= (*a_start_ptr++) << 16;
        if (a_chr != b_chr) {
            return PW_NEQ;
        }
    }
    if (_pw_unlikely(*b_start_ptr++)) { return PW_NEQ; }
    if (_pw_unlikely(*b_start_ptr++)) { return PW_NEQ; }
    if (_pw_unlikely(*b_start_ptr)) { return PW_NEQ; }
    return true;
}

// Comparison with 0-terminated UTF-8 string.

#define EQUZ_UTF8(CHAR_TYPE, CHAR_SIZE)  \
    static int streqz_##CHAR_SIZE##_0(uint8_t* a_start_ptr, unsigned a_length, uint8_t* b_start_ptr)  \
    {  \
        while(a_length--) {  \
            char32_t c1 = _pw_decode_utf8_char((char8_t**) &b_start_ptr);  \
            if (!c1) {  \
                return PW_EQ_PARTIAL;  \
            }  \
            if (c1 != *((CHAR_TYPE*) a_start_ptr)) {  \
                return PW_NEQ;  \
            }  \
            a_start_ptr += CHAR_SIZE;  \
        }  \
        return (0 == *b_start_ptr)? PW_EQ : PW_NEQ;  \
    }
EQUZ_UTF8(uint8_t,  1)
EQUZ_UTF8(uint16_t, 2)
EQUZ_UTF8(char32_t, 4)

static int streqz_3_0(uint8_t* a_start_ptr, unsigned a_length, uint8_t* b_start_ptr)
{
    while(a_length--) {
        char32_t c1 = _pw_decode_utf8_char((char8_t**) &b_start_ptr);
        if (!c1) {
            return PW_EQ_PARTIAL;
        }
        char32_t c2 = *a_start_ptr++;
        c2 |= (*a_start_ptr++) << 8;
        c2 |= (*a_start_ptr++) << 16;
        if (c1 != c2) {
            return PW_NEQ;
        }
    }
    return (0 == *b_start_ptr)? PW_EQ : PW_NEQ;
}


StrEqualZ _pw_str_equalz_variants[5][5] = {
    {
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    },
    {
        streqz_1_0,
        streqz_1_1,
        streqz_1_2,
        streqz_1_3,
        streqz_1_4
    },
    {
        streqz_2_0,
        streqz_2_1,
        streqz_2_2,
        streqz_2_3,
        streqz_2_4
    },
    {
        streqz_3_0,
        streqz_3_1,
        streqz_3_2,
        streqz_3_3,
        streqz_3_4
    },
    {
        streqz_4_0,
        streqz_4_1,
        streqz_4_2,
        streqz_4_3,
        streqz_4_4
    }
};

/****************************************************************
 * High-level functions
 */

[[nodiscard]] bool _pw_equal_z(PwValuePtr a, void* b, uint8_t b_char_size)
{
    if (_pw_unlikely(!pw_is_string(a))) {
        return false;
    }
    unsigned a_length;
    uint8_t* a_ptr = _pw_string_start_length(a, &a_length);

    if (_pw_unlikely(a_length == 0)) {
        if (b_char_size == 4) {
            return 0 == *(char32_t*) b;
        } else {
            return 0 == *(char*) b;
        }
    }

    StrEqualZ fn_equalz = _pw_str_equalz_variants[a->char_size][b_char_size];
    int match = fn_equalz(a_ptr, a_length, b);
    return (match == PW_EQ)? true : false;  // partial match means not equal
}
