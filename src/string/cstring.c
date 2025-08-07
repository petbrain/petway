#include <stdlib.h>

#include "include/pw.h"
#include "src/string/pw_string_internal.h"

static void _cp_to_utf8_uint8_t(uint8_t* self_ptr, char* dest, unsigned length)
{
    memcpy(dest, self_ptr, length);
    *(dest + length) = 0;
}

// integral types:

#define STR_COPY_TO_UTF8_IMPL(type_name_self)  \
    static void _cp_to_utf8_##type_name_self(uint8_t* self_ptr, char* dest, unsigned length)  \
    {  \
        type_name_self* src_ptr = (type_name_self*) self_ptr;  \
        while (length--) {  \
            dest += pw_char32_to_utf8(*src_ptr++, dest); \
        }  \
        *dest = 0;  \
    }

STR_COPY_TO_UTF8_IMPL(uint16_t)
STR_COPY_TO_UTF8_IMPL(uint32_t)

// uint24_t

static void _cp_to_utf8_uint24_t(uint8_t* self_ptr, char* dest, unsigned length)
{
    while (length--) {
        char32_t c = *self_ptr++;
        c |= (*self_ptr++) << 8;
        c |= (*self_ptr++) << 16;
        dest += pw_char32_to_utf8(c, dest);
    }
    *dest = 0;
}

typedef void (*CopyToUtf8)(uint8_t* self_ptr, char* dest_ptr, unsigned length);

static CopyToUtf8 _pw_copy_to_utf8_variants[5] = {
    nullptr,
    _cp_to_utf8_uint8_t,
    _cp_to_utf8_uint16_t,
    _cp_to_utf8_uint24_t,
    _cp_to_utf8_uint32_t
};

CStringPtr pw_string_to_utf8(PwValuePtr str)
{
    CStringPtr result = nullptr;

    result = malloc(pw_strlen_in_utf8(str) + 1);
    if (!result) {
        return nullptr;
    }
    unsigned length;
    uint8_t* ptr = _pw_string_start_length(str, &length);
    CopyToUtf8 fn_copy_to_utf8 = _pw_copy_to_utf8_variants[str->char_size];
    fn_copy_to_utf8(ptr, result, length);
    return result;
}

void pw_string_to_utf8_buf(PwValuePtr str, char* buffer)
{
    pw_assert_string(str);
    unsigned length;
    uint8_t* ptr = _pw_string_start_length(str, &length);
    CopyToUtf8 fn_copy_to_utf8 = _pw_copy_to_utf8_variants[str->char_size];
    fn_copy_to_utf8(ptr, buffer, length);
}

void pw_substr_to_utf8_buf(PwValuePtr str, unsigned start_pos, unsigned end_pos, char* buffer)
{
    pw_assert_string(str);
    unsigned length;
    uint8_t* ptr = _pw_string_start_length(str, &length);
    if (end_pos >= length) {
        end_pos = length;
    }
    if (end_pos <= start_pos) {
        *buffer = 0;
        return;
    }
    CopyToUtf8 fn_copy_to_utf8 = _pw_copy_to_utf8_variants[str->char_size];
    fn_copy_to_utf8(
        ptr + start_pos * str->char_size,
        buffer,
        end_pos - start_pos
    );
}

void pw_destroy_cstring(CStringPtr* str)
{
    free(*str);
    *str = nullptr;
}
