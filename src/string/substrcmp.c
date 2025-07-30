#include <string.h>  // for memcmp

#include "include/pw.h"
#include "src/string/pw_string_internal.h"

static bool substrcmp_n_n(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    return 0 == memcmp(str_start_ptr, substr_start_ptr, substr_end_ptr - substr_start_ptr);
}

static bool substrcmp_1_2(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        if (*str_start_ptr++ != *((uint16_t*) substr_start_ptr)) {
            return false;
        }
        substr_start_ptr += 2;
    }
    return true;
}

static bool substrcmp_1_3(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
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

static bool substrcmp_1_4(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        if (*str_start_ptr++ != *((char32_t*) substr_start_ptr)) {
            return false;
        }
        substr_start_ptr += 4;
    }
    return true;
}


static bool substrcmp_2_1(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        if (*((uint16_t*) str_start_ptr) != *substr_start_ptr++) {
            return false;
        }
        str_start_ptr += 2;
    }
    return true;
}

static bool substrcmp_2_3(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
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

static bool substrcmp_2_4(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
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


static bool substrcmp_3_1(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
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

static bool substrcmp_3_2(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
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

static bool substrcmp_3_4(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
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


static bool substrcmp_4_1(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
{
    while (substr_start_ptr < substr_end_ptr) {
        if (*((char32_t*) str_start_ptr) != *substr_start_ptr++) {
            return false;
        }
        str_start_ptr += 4;
    }
    return true;
}

static bool substrcmp_4_2(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
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

static bool substrcmp_4_3(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr)
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

SubstrCmp _pw_substrcmp_variants[5][5] = {
    {
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    },
    {
        nullptr,
        substrcmp_n_n,
        substrcmp_1_2,
        substrcmp_1_3,
        substrcmp_1_4
    },
    {
        nullptr,
        substrcmp_2_1,
        substrcmp_n_n,
        substrcmp_2_3,
        substrcmp_2_4
    },
    {
        nullptr,
        substrcmp_3_1,
        substrcmp_3_2,
        substrcmp_n_n,
        substrcmp_3_4
    },
    {
        nullptr,
        substrcmp_4_1,
        substrcmp_4_2,
        substrcmp_4_3,
        substrcmp_n_n
    }
};
