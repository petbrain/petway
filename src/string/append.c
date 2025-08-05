#include "include/pw.h"
#include "src/string/pw_string_internal.h"

/*
 * Append variants with no dest expansion
 */
#define APPEND_NOEXPAND_NN(DEST_CHAR_SIZE, SRC_CHAR_SIZE)  \
    [[nodiscard]] static bool append_##DEST_CHAR_SIZE##_##SRC_CHAR_SIZE(  \
        PwValuePtr dest, unsigned dest_pos, uint8_t* src_start_ptr, uint8_t* src_end_ptr)  \
    {  \
        _pw_strcopy_n_n(_pw_string_char_ptr(dest, dest_pos), src_start_ptr, src_end_ptr);  \
        return true;  \
    }
APPEND_NOEXPAND_NN(1, 1)
APPEND_NOEXPAND_NN(2, 2)
APPEND_NOEXPAND_NN(3, 3)
APPEND_NOEXPAND_NN(4, 4)

#define APPEND_NOEXPAND(DEST_CHAR_SIZE, SRC_CHAR_SIZE)  \
    [[nodiscard]] static bool append_##DEST_CHAR_SIZE##_##SRC_CHAR_SIZE(  \
        PwValuePtr dest, unsigned dest_pos, uint8_t* src_start_ptr, uint8_t* src_end_ptr)  \
    {  \
        _pw_strcopy_##DEST_CHAR_SIZE##_##SRC_CHAR_SIZE(_pw_string_char_ptr(dest, dest_pos), src_start_ptr, src_end_ptr);  \
        return true;  \
    }
APPEND_NOEXPAND(2, 1)
APPEND_NOEXPAND(3, 1)
APPEND_NOEXPAND(3, 2)
APPEND_NOEXPAND(4, 1)
APPEND_NOEXPAND(4, 2)
APPEND_NOEXPAND(4, 3)


/*
 * Append variants with dest expansion for char_size 1, 2, and 4
 */
#define APPEND_EXPAND(DEST_CHAR_TYPE, DEST_CHAR_SIZE, SRC_CHAR_TYPE, SRC_CHAR_SIZE)  \
    [[nodiscard]] static bool append_##DEST_CHAR_SIZE##_##SRC_CHAR_SIZE(  \
        PwValuePtr dest, unsigned dest_pos, uint8_t* src_start_ptr, uint8_t* src_end_ptr)  \
    {  \
        uint8_t* dest_ptr = _pw_string_char_ptr(dest, dest_pos);  \
        while (src_start_ptr < src_end_ptr) {  \
            char32_t c = *((SRC_CHAR_TYPE*) src_start_ptr);  \
            uint8_t char_size = calc_char_size(c);  \
            if (_pw_unlikely(char_size > DEST_CHAR_SIZE)) {  \
                if (!_pw_expand_string(dest, 0, char_size)) {  \
                    return false;  \
                }  \
                StrAppend fn_append = _pw_str_append_variants[char_size][SRC_CHAR_SIZE];  \
                return fn_append(dest, dest_pos, src_start_ptr, src_end_ptr);  \
            }  \
            *((DEST_CHAR_TYPE*) dest_ptr) = (DEST_CHAR_TYPE) c;  \
            dest_ptr += DEST_CHAR_SIZE;  \
            src_start_ptr += SRC_CHAR_SIZE;  \
            dest_pos++;  \
        }  \
        return true;  \
    }
APPEND_EXPAND(uint8_t,  1, uint16_t, 2)
APPEND_EXPAND(uint8_t,  1, char32_t, 4)
APPEND_EXPAND(uint16_t, 2, char32_t, 4)

/*
 * Append variants with dest expansion for source char_size 3
 */
#define APPEND_EXPAND_N_3(DEST_CHAR_TYPE, DEST_CHAR_SIZE)  \
    [[nodiscard]] static bool append_##DEST_CHAR_SIZE##_3(  \
        PwValuePtr dest, unsigned dest_pos, uint8_t* src_start_ptr, uint8_t* src_end_ptr)  \
    {  \
        uint8_t* dest_ptr = _pw_string_char_ptr(dest, dest_pos);  \
        while (src_start_ptr < src_end_ptr) {  \
            char32_t c = *src_start_ptr++;  \
            c |= (*src_start_ptr++) << 8;  \
            c |= (*src_start_ptr++) << 16;  \
            uint8_t char_size = calc_char_size(c);  \
            if (_pw_unlikely(char_size > DEST_CHAR_SIZE)) {  \
                if (!_pw_expand_string(dest, 0, char_size)) {  \
                    return false;  \
                }  \
                StrAppend fn_append = _pw_str_append_variants[char_size][3];  \
                return fn_append(dest, dest_pos, src_start_ptr - 3, src_end_ptr);  \
            }  \
            *((DEST_CHAR_TYPE*) dest_ptr) = (DEST_CHAR_TYPE) c;  \
            dest_ptr += DEST_CHAR_SIZE;  \
            dest_pos++;  \
        }  \
        return true;  \
    }
APPEND_EXPAND_N_3(uint8_t,  1)
APPEND_EXPAND_N_3(uint16_t, 2)

/*
 * Append variant with dest expansion for source char_size 4 and dest char_size 3
 */
[[nodiscard]] static bool append_3_4(PwValuePtr dest, unsigned dest_pos, uint8_t* src_start_ptr, uint8_t* src_end_ptr)
{
    uint8_t* dest_ptr = _pw_string_char_ptr(dest, dest_pos);
    while (src_start_ptr < src_end_ptr) {
        char32_t c = *((char32_t*) src_start_ptr);
        uint8_t char_size = calc_char_size(c);
        if (_pw_unlikely(char_size > 3)) {
            if (!_pw_expand_string(dest, 0, char_size)) {
                return false;
            }
            StrAppend fn_append = _pw_str_append_variants[char_size][0];
            return fn_append(dest, dest_pos, src_start_ptr, src_end_ptr);
        }
        *dest_ptr++ = (uint8_t) c; c >>= 8;
        *dest_ptr++ = (uint8_t) c; c >>= 8;
        *dest_ptr++ = (uint8_t) c;
        src_start_ptr += 4;
        dest_pos++;
    }
    return true;
}

/*
 * Append variants with dest expansion for UTF-8 source and dest char_size 1, 2, and 4
 */
#define APPEND_UTF8(DEST_CHAR_TYPE, DEST_CHAR_SIZE)  \
    [[nodiscard]] static bool append_##DEST_CHAR_SIZE##_0(  \
        PwValuePtr dest, unsigned dest_pos, uint8_t* src_start_ptr, uint8_t* src_end_ptr)  \
    {  \
        uint8_t* dest_ptr = _pw_string_char_ptr(dest, dest_pos);  \
        while (src_start_ptr < src_end_ptr) {  \
            uint8_t* current_ptr = src_start_ptr;  \
            char32_t c = _pw_decode_utf8_char(&src_start_ptr);  \
            uint8_t char_size = calc_char_size(c);  \
            if (_pw_unlikely(char_size > DEST_CHAR_SIZE)) {  \
                if (!_pw_expand_string(dest, 0, char_size)) {  \
                    return false;  \
                }  \
                StrAppend fn_append = _pw_str_append_variants[char_size][0];  \
                return fn_append(dest, dest_pos, current_ptr, src_end_ptr);  \
            }  \
            *((DEST_CHAR_TYPE*) dest_ptr) = (DEST_CHAR_TYPE) c;  \
            dest_ptr += DEST_CHAR_SIZE;  \
            dest_pos++;  \
        }  \
        return true;  \
    }
APPEND_UTF8(uint8_t,  1)
APPEND_UTF8(uint16_t, 2)
APPEND_UTF8(char32_t, 4)

[[nodiscard]] static bool append_3_0(PwValuePtr dest, unsigned dest_pos, uint8_t* src_start_ptr, uint8_t* src_end_ptr)
{
    uint8_t* dest_ptr = _pw_string_char_ptr(dest, dest_pos);
    while (src_start_ptr < src_end_ptr) {
        uint8_t* current_ptr = src_start_ptr;
        char32_t c = _pw_decode_utf8_char(&src_start_ptr);
        uint8_t char_size = calc_char_size(c);
        if (_pw_unlikely(char_size > 3)) {
            if (!_pw_expand_string(dest, 0, char_size)) {
                return false;
            }
            StrAppend fn_append = _pw_str_append_variants[char_size][0];
            return fn_append(dest, dest_pos, current_ptr, src_end_ptr);
        }
        *dest_ptr++ = (uint8_t) c; c >>= 8;
        *dest_ptr++ = (uint8_t) c; c >>= 8;
        *dest_ptr++ = (uint8_t) c;
        dest_pos++;
    }
    return true;
}

StrAppend _pw_str_append_variants[5][5] = {
    {
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    },
    {
        append_1_0,
        append_1_1,
        append_1_2,
        append_1_3,
        append_1_4
    },
    {
        append_2_0,
        append_2_1,
        append_2_2,
        append_2_3,
        append_2_4
    },
    {
        append_3_0,
        append_3_1,
        append_3_2,
        append_3_3,
        append_3_4
    },
    {
        append_4_0,
        append_4_1,
        append_4_2,
        append_4_3,
        append_4_4
    }
};

/****************************************************************
 * High-level functions
 */

[[ nodiscard]] bool _pw_string_append_c32(PwValuePtr dest, char32_t c)
{
    pw_assert_string(dest);
    if (!_pw_expand_string(dest, 1, calc_char_size(c))) {
        return false;
    }
    unsigned dest_pos = _pw_string_inc_length(dest, 1);
    _pw_put_char(_pw_string_char_ptr(dest, dest_pos), c, dest->char_size);
    return true;
}

[[nodiscard]] bool _pw_string_append_ascii(PwValuePtr dest, char* start_ptr, char* end_ptr)
{
    pw_assert_string(dest);

    if (!end_ptr) {
        end_ptr = start_ptr + strlen(start_ptr);
    }
    size_t src_len = end_ptr - start_ptr;
    if (src_len == 0) {
        return true;
    }
    if (src_len >= UINT_MAX) {
        pw_set_status(PwStatus(PW_ERROR_STRING_TOO_LONG));
        return false;
    }
    if (!_pw_expand_string(dest, src_len, 1)) {
        return false;
    }
    unsigned dest_pos = _pw_string_inc_length(dest, src_len);

    StrAppend fn_append = _pw_str_append_variants[dest->char_size][1];
    return fn_append(dest, dest_pos, (uint8_t*) start_ptr, (uint8_t*) end_ptr);
}

[[nodiscard]] bool _pw_string_append_utf8(PwValuePtr dest, char8_t* start_ptr, char8_t* end_ptr)
{
    pw_assert_string(dest);

    uint8_t src_char_size = 1;
    size_t src_len;
    if (end_ptr) {
        src_len = end_ptr - start_ptr;
    } else {
        src_len = utf8_strlen3(start_ptr, &src_char_size, &end_ptr);
    }
    if (src_len == 0) {
        return true;
    }
    if (src_len >= UINT_MAX) {
        pw_set_status(PwStatus(PW_ERROR_STRING_TOO_LONG));
        return false;
    }
    if (!_pw_expand_string(dest, src_len, src_char_size)) {
        return false;
    }
    unsigned dest_pos = _pw_string_inc_length(dest, src_len);

    StrAppend fn_append = _pw_str_append_variants[dest->char_size][0];
    return fn_append(dest, dest_pos, (uint8_t*) start_ptr, (uint8_t*) end_ptr);
}

[[nodiscard]] bool _pw_string_append_utf32(PwValuePtr dest, char32_t* start_ptr, char32_t* end_ptr)
{
    pw_assert_string(dest);

    uint8_t src_char_size = 1;
    if (!end_ptr) {
        end_ptr = start_ptr + utf32_strlen2(start_ptr, &src_char_size);
    }
    size_t src_len = end_ptr - start_ptr;
    if (src_len == 0) {
        return true;
    }
    if (src_len >= UINT_MAX / 4) {
        pw_set_status(PwStatus(PW_ERROR_STRING_TOO_LONG));
        return false;
    }
    if (!_pw_expand_string(dest, src_len, src_char_size)) {
        return false;
    }
    unsigned dest_pos = _pw_string_inc_length(dest, src_len);

    StrAppend fn_append = _pw_str_append_variants[dest->char_size][4];
    return fn_append(dest, dest_pos, (uint8_t*) start_ptr, (uint8_t*) end_ptr);
}

[[ nodiscard]] bool _pw_string_append(PwValuePtr dest, PwValuePtr src)
{
    pw_assert_string(dest);
    unsigned src_len = pw_strlen(src);
    if (!_pw_expand_string(dest, src_len, src->char_size)) {
        return false;
    }
    unsigned dest_pos = _pw_string_inc_length(dest, src_len);
    uint8_t* src_end_ptr;
    uint8_t* src_start_ptr = _pw_string_start_end(src, &src_end_ptr);

    StrAppend fn_append = _pw_str_append_variants[dest->char_size][src->char_size];
    return fn_append(dest, dest_pos, src_start_ptr, src_end_ptr);
}

[[ nodiscard]] bool pw_string_append_substring(PwValuePtr dest, PwValuePtr src, unsigned src_start_pos, unsigned src_end_pos)
{
    pw_assert_string(dest);
    unsigned src_len  = pw_strlen(src);
    if (src_end_pos > src_len) {
        src_end_pos = src_len;
    }
    if (src_start_pos >= src_end_pos) {
        return true;
    }
    src_len = src_end_pos - src_start_pos;

    // expand with char_size unchanged because it is unknown for substring of src

    if (!_pw_expand_string(dest, src_len, 1)) {
        return false;
    }
    unsigned dest_pos = _pw_string_inc_length(dest, src_len);
    uint8_t src_char_size = src->char_size;
    uint8_t* src_start_ptr = _pw_string_start(src);
    uint8_t* src_end_ptr = src_start_ptr + src_end_pos * src_char_size;
    src_start_ptr += src_start_pos * src_char_size;

    StrAppend fn_append = _pw_str_append_variants[dest->char_size][src_char_size];
    return fn_append(dest, dest_pos, src_start_ptr, src_end_ptr);
}
