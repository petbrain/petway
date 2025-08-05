#include <string.h>

#include "include/pw.h"
#include "src/string/pw_string_internal.h"


void _pw_strcopy_n_n(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr)
{
    memcpy(dest_ptr, src_start_ptr, src_end_ptr - src_start_ptr);
}

#define COPY(DEST_CHAR_TYPE, DEST_CHAR_SIZE, SRC_CHAR_TYPE, SRC_CHAR_SIZE)  \
    void _pw_strcopy_##DEST_CHAR_SIZE##_##SRC_CHAR_SIZE(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr)  \
    {  \
        while (src_start_ptr < src_end_ptr) {  \
            *((DEST_CHAR_TYPE*) dest_ptr) = *((SRC_CHAR_TYPE*) src_start_ptr);  \
            dest_ptr += DEST_CHAR_SIZE;  \
            src_start_ptr += SRC_CHAR_SIZE;  \
        }  \
    }
COPY(uint8_t,  1, uint16_t, 2)
COPY(uint8_t,  1, char32_t, 4)
COPY(uint16_t, 2, uint8_t,  1)
COPY(uint16_t, 2, char32_t, 4)
COPY(char32_t, 4, uint8_t,  1)
COPY(char32_t, 4, uint16_t, 2)

#define COPY_N_3(DEST_CHAR_TYPE, DEST_CHAR_SIZE)  \
    void _pw_strcopy_##DEST_CHAR_SIZE##_3(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr)  \
    {  \
        while (src_start_ptr < src_end_ptr) {  \
            char32_t c = *src_start_ptr++;  \
            c |= (*src_start_ptr++) << 8;  \
            c |= (*src_start_ptr++) << 16;  \
            *((DEST_CHAR_TYPE*) dest_ptr) = c;  \
            dest_ptr += DEST_CHAR_SIZE;  \
        }  \
    }
COPY_N_3(uint8_t,  1)
COPY_N_3(uint16_t, 2)
COPY_N_3(char32_t, 4)

#define COPY_3_N(SRC_CHAR_TYPE, SRC_CHAR_SIZE)  \
    void _pw_strcopy_3_##SRC_CHAR_SIZE(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr)  \
    {  \
        while (src_start_ptr < src_end_ptr) {  \
            char32_t c = *((SRC_CHAR_TYPE*) src_start_ptr);  \
            *dest_ptr++ = (uint8_t) c; c >>= 8;  \
            *dest_ptr++ = (uint8_t) c; c >>= 8;  \
            *dest_ptr++ = (uint8_t) c;  \
            src_start_ptr += SRC_CHAR_SIZE;  \
        }  \
    }
COPY_3_N(uint8_t,  1)
COPY_3_N(uint16_t, 2)
COPY_3_N(char32_t, 4)


StrCopy _pw_strcopy_variants[5][5] = {
    {
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    },
    {
        nullptr,
        _pw_strcopy_n_n,
        _pw_strcopy_1_2,
        _pw_strcopy_1_3,
        _pw_strcopy_1_4
    },
    {
        nullptr,
        _pw_strcopy_2_1,
        _pw_strcopy_n_n,
        _pw_strcopy_2_3,
        _pw_strcopy_2_4
    },
    {
        nullptr,
        _pw_strcopy_3_1,
        _pw_strcopy_3_2,
        _pw_strcopy_n_n,
        _pw_strcopy_3_4
    },
    {
        nullptr,
        _pw_strcopy_4_1,
        _pw_strcopy_4_2,
        _pw_strcopy_4_3,
        _pw_strcopy_n_n
    }
};
