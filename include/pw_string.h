#pragma once

#include <ctype.h>

#include <pw_assert.h>
#include <pw_types.h>

#ifdef PW_WITH_ICU
    // ICU library for character classification:
#   include <unicode/uchar.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline uint8_t _pw_string_char_size(PwValuePtr s)
/*
 * char_size is stored as 0-based whereas all
 * functions use 1-based values.
 */
{
    return s->str_char_size + 1;
}

static inline uint8_t pw_string_char_size(PwValuePtr str)
{
    pw_assert_string(str);
    return _pw_string_char_size(str);
}

static inline uint8_t* _pw_string_start(PwValuePtr str)
/*
 * Return pointer to the start of internal string data.
 */
{
    if (str->str_embedded) {
        return str->str_1;
    } else {
        return str->string_data->data;
    }
}

static inline uint8_t* _pw_string_start_end(PwValuePtr str, uint8_t** end)
/*
 * Return pointer to the start of internal string data
 * and write final pointer to `end`.
 */
{
    if (str->str_embedded) {
        *end = &str->str_1[str->str_embedded_length * _pw_string_char_size(str)];
        return str->str_1;
    } else {
        _PwStringData* sdata = str->string_data;
        *end = &sdata->data[str->str_length * _pw_string_char_size(str)];
        return sdata->data;
    }
}

static inline uint8_t* _pw_string_start_length(PwValuePtr str, unsigned* length)
/*
 * Return pointer to the start of internal string data
 * and write length of string to `length`.
 */
{
   if (str->str_embedded) {
        *length = str->str_embedded_length;
        return str->str_1;
    } else {
        *length = str->str_length;
        return str->string_data->data;
    }
}

static inline uint8_t* _pw_string_char_ptr(PwValuePtr str, unsigned position)
/*
 * Return address of character in string at `position`.
 * If position is beyond end of line return nullptr.
 */
{
    unsigned offset = position * _pw_string_char_size(str);
    if (str->str_embedded) {
        if (position < str->str_embedded_length) {
            return &str->str_1[offset];
        }
    } else {
        if (position < str->str_length) {
            return &str->string_data->data[offset];
        }
    }
    return nullptr;
}

#define PW_CHAR_METHODS_IMPL(type_name)  \
    static inline char32_t _pw_get_char_##type_name(uint8_t* p)  \
    {  \
        return *((type_name*) p); \
    }  \
    static void _pw_put_char_##type_name(uint8_t* p, char32_t c)  \
    {  \
        *((type_name*) p) = (type_name) c; \
    }

PW_CHAR_METHODS_IMPL(uint8_t)
PW_CHAR_METHODS_IMPL(uint16_t)
PW_CHAR_METHODS_IMPL(uint32_t)

static inline char32_t _pw_get_char_uint24_t(uint8_t* p)
{
    // always little endian
    char32_t c = p[0] | (p[1] << 8) | (p[2] << 16);
    return c;
}

static inline void _pw_put_char_uint24_t(uint8_t* p, char32_t c)
{
    // always little endian
    p[0] = (uint8_t) c; c >>= 8;
    p[1] = (uint8_t) c; c >>= 8;
    p[2] = (uint8_t) c;
}

static inline char32_t _pw_get_char(uint8_t* ptr, uint8_t char_size)
{
    switch (char_size) {
        case 1: return _pw_get_char_uint8_t(ptr);
        case 2: return _pw_get_char_uint16_t(ptr);
        case 3: return _pw_get_char_uint24_t(ptr);
        case 4: return _pw_get_char_uint32_t(ptr);
        default: _pw_panic_bad_char_size(char_size);
    }
}

static inline void _pw_put_char(uint8_t* ptr, char32_t chr, uint8_t char_size)
{
    switch (char_size) {
        case 1: _pw_put_char_uint8_t(ptr, chr); return;
        case 2: _pw_put_char_uint16_t(ptr, chr); return;
        case 3: _pw_put_char_uint24_t(ptr, chr); return;
        case 4: _pw_put_char_uint32_t(ptr, chr); return;
        default: _pw_panic_bad_char_size(char_size);
    }
}

static inline char32_t _pw_char_at(PwValuePtr str, unsigned position)
/*
 * Return character at `position`.
 * If position is beyond end of line return 0.
 */
{
    uint8_t char_size = _pw_string_char_size(str);
    unsigned offset = position * char_size;
    if (str->str_embedded) {
        if (position < str->str_embedded_length) {
            return _pw_get_char(&str->str_1[offset], char_size);
        }
    } else {
        if (position < str->str_length) {
            return _pw_get_char(&str->string_data->data[offset], char_size);
        }
    }
    return 0;
}

static inline char32_t pw_char_at(PwValuePtr str, unsigned position)
{
    pw_assert_string(str);
    return _pw_char_at(str, position);
}

// check if `index` is within string length
#define pw_string_index_valid(str, index) ((index) < pw_strlen(str))

bool _pw_equal_u8 (PwValuePtr a, char8_t* b);
bool _pw_equal_u32(PwValuePtr a, char32_t* b);

unsigned pw_strlen(PwValuePtr str);
/*
 * Return length of string.
 */

void pw_string_to_utf8_buf(PwValuePtr str, char* buffer);
void pw_substr_to_utf8_buf(PwValuePtr str, unsigned start_pos, unsigned end_pos, char* buffer);
/*
 * Copy string to buffer, appending terminating 0.
 * Use carefully. The caller is responsible to allocate the buffer.
 * Encode multibyte chars to UTF-8.
 */

PwResult pw_substr(PwValuePtr str, unsigned start_pos, unsigned end_pos);
/*
 * Get substring from `start_pos` to `end_pos`.
 */

bool pw_string_erase(PwValuePtr str, unsigned start_pos, unsigned end_pos);
/*
 * Erase characters from `start_pos` to `end_pos`.
 * This may make a copy of string, so checking return value is mandatory.
 */

bool pw_string_truncate(PwValuePtr str, unsigned position);
/*
 * Truncate string at given `position`.
 * This may make a copy of string, so checking return value is mandatory.
 */

bool pw_strchr(PwValuePtr str, char32_t chr, unsigned start_pos, unsigned* result);
/*
 * Find first occurence of `chr` in `str` starting from `start_pos`.
 *
 * Return true if character is found and write its position to `result`.
 * `result` can be nullptr if position is not needed and the function
 * is called just to check if `chr` is in `str`.
 */

bool pw_string_ltrim(PwValuePtr str);
bool pw_string_rtrim(PwValuePtr str);
bool pw_string_trim(PwValuePtr str);

bool pw_string_lower(PwValuePtr str);
bool pw_string_upper(PwValuePtr str);

unsigned pw_strlen_in_utf8(PwValuePtr str);
/*
 * Return length of str as if was encoded in UTF-8.
 * `str` can be either String or CharPtr.
 */

char* pw_char32_to_utf8(char32_t codepoint, char* buffer);
/*
 * Write up to 4 characters to buffer.
 * Return pointer to the next position in buffer.
 */

void _pw_putchar32_utf8(FILE* fp, char32_t codepoint);

unsigned utf8_strlen(char8_t* str);
/*
 * Count the number of codepoints in UTF8-encoded string.
 */

unsigned utf8_strlen2(char8_t* str, uint8_t* char_size);
/*
 * Count the number of codepoints in UTF8-encoded string
 * and find max char size.
 */

unsigned utf8_strlen2_buf(char8_t* buffer, unsigned* size, uint8_t* char_size);
/*
 * Count the number of codepoints in the buffer and find max char size.
 *
 * Null characters are allowed! They are counted as zero codepoints.
 *
 * Return the number of codepoints.
 * Write the number of processed bytes back to `size`.
 * This number can be less than original `size` if buffer ends with
 * incomplete sequence.
 */

char8_t* utf8_skip(char8_t* str, unsigned n);
/*
 * Skip `n` characters, return pointer to `n`th char.
 */

unsigned u32_strlen(char32_t* str);
/*
 * Find length of null-terminated `str`.
 */

unsigned u32_strlen2(char32_t* str, uint8_t* char_size);
/*
 * Find both length of null-terminated `str` and max char size in one go.
 */

int u32_strcmp   (char32_t* a, char32_t* b);
int u32_strcmp_u8(char32_t* a, char8_t*  b);
/*
 * Compare  null-terminated strings.
 */

char32_t* u32_strchr(char32_t* str, char32_t chr);
/*
 * Find the first occurrence of `chr` in the null-terminated `str`.
 */

uint8_t u32_char_size(char32_t* str, unsigned max_len);
/*
 * Find the maximal size of character in `str`, up to `max_len` or null terminator.
 */

#ifdef PW_WITH_ICU
#   define pw_char_lower(c)  u_tolower(c)
#   define pw_char_upper(c)  u_toupper(c)
#else
#   define pw_char_lower(c)  tolower(c)
#   define pw_char_upper(c)  toupper(c)
#endif

/*
 * pw_strcat generic macro allows passing arguments
 * either by value or by reference.
 * When passed by value, the function destroys them
 * .
 * CAVEAT: DO NOT PASS LOCAL VARIABLES BY VALUES!
 */
#define pw_strcat(str, ...) _Generic((str),  \
        _PwValue:   _pw_strcat_va_v,  \
        PwValuePtr: _pw_strcat_va_p   \
    )((str) __VA_OPT__(,) __VA_ARGS__,\
        _Generic((str),  \
            _PwValue:   PwVaEnd(),  \
            PwValuePtr: nullptr     \
        ))

PwResult _pw_strcat_va_v(...);
PwResult _pw_strcat_va_p(...);

PwResult _pw_strcat_ap_v(va_list ap);
PwResult _pw_strcat_ap_p(va_list ap);

unsigned pw_string_skip_spaces(PwValuePtr str, unsigned position);
/*
 * Find position of the first non-space character starting from `position`.
 * If non-space character is not found, the length is returned.
 */

unsigned pw_string_skip_chars(PwValuePtr str, unsigned position, char32_t* skipchars);
/*
 * Find position of the first character not belonging to `skipchars` starting from `position`.
 * If no `skipchars` encountered, the length is returned.
 */

/****************************************************************
 * Character classification functions
 */

#ifdef PW_WITH_ICU
#   define pw_isspace(c)  u_isspace(c)
#else
#   define pw_isspace(c)  isspace(c)
#endif
/*
 * Return true if `c` is a whitespace character.
 */

#define pw_isdigit(c)  isdigit(c)
/*
 * Return true if `c` is an ASCII digit.
 * Do not consider any other unicode digits because this function
 * is used in conjunction with standard C library (string to number conversion)
 * that does not support unicode character classification.
 */

bool pw_string_isdigit(PwValuePtr str);
/*
 * Return true if `str` is not empty and contains all digits.
 */

/****************************************************************
 * Constructors
 */

#define pw_create_string(initializer) _Generic((initializer),   \
                 char*: _pw_create_string_u8_wrapper, \
              char8_t*: _pw_create_string_u8,         \
             char32_t*: _pw_create_string_u32,        \
            PwValuePtr: _pw_create_string             \
    )((initializer))

PwResult _pw_create_string_u8 (char8_t*   initializer);
PwResult _pw_create_string_u32(char32_t*  initializer);
PwResult _pw_create_string    (PwValuePtr initializer);

static inline PwResult _pw_create_string_u8_wrapper(char* initializer)
{
    return _pw_create_string_u8((char8_t*) initializer);
}

PwResult pw_create_empty_string(unsigned capacity, uint8_t char_size);

/****************************************************************
 * Append functions
 */

#define pw_string_append(dest, src) _Generic((src),   \
              char32_t: _pw_string_append_c32,        \
                   int: _pw_string_append_c32,        \
                 char*: _pw_string_append_u8_wrapper, \
              char8_t*: _pw_string_append_u8,         \
             char32_t*: _pw_string_append_u32,        \
            PwValuePtr: _pw_string_append             \
    )((dest), (src))

bool _pw_string_append_c32(PwValuePtr dest, char32_t   c);
bool _pw_string_append_u8 (PwValuePtr dest, char8_t*   src);
bool _pw_string_append_u32(PwValuePtr dest, char32_t*  src);
bool _pw_string_append    (PwValuePtr dest, PwValuePtr src);

static inline bool _pw_string_append_u8_wrapper(PwValuePtr dest, char* src)
{
    return _pw_string_append_u8(dest, (char8_t*) src);
}

bool pw_string_append_utf8(PwValuePtr dest, char8_t* buffer, unsigned size, unsigned* bytes_processed);
/*
 * Append UTF-8-encoded characters from `buffer`.
 * Write the number of bytes processed to `bytes_processed`, which can be less
 * than `size` if buffer ends with incomplete UTF-8 sequence.
 *
 * Return false if out of memory.
 */

bool pw_string_append_buffer(PwValuePtr dest, uint8_t* buffer, unsigned size);
/*
 * Append bytes from `buffer`.
 * `dest` char size must be 1.
 *
 * Return false if out of memory.
 */

/****************************************************************
 * Insert functions
 * TODO other types
 */

#define pw_string_insert_chars(str, position, chr, n) _Generic((chr), \
              char32_t: _pw_string_insert_many_c32,   \
                   int: _pw_string_insert_many_c32    \
    )((str), (position), (chr), (n))

bool _pw_string_insert_many_c32(PwValuePtr str, unsigned position, char32_t chr, unsigned n);

/****************************************************************
 * Append substring functions.
 *
 * Append `src` substring starting from `src_start_pos` to `src_end_pos`.
 */

#define pw_string_append_substring(dest, src, src_start_pos, src_end_pos) _Generic((src), \
                 char*: _pw_string_append_substring_u8_wrapper,  \
              char8_t*: _pw_string_append_substring_u8,          \
             char32_t*: _pw_string_append_substring_u32,         \
            PwValuePtr: _pw_string_append_substring              \
    )((dest), (src), (src_start_pos), (src_end_pos))

bool _pw_string_append_substring_u8 (PwValuePtr dest, char8_t*   src, unsigned src_start_pos, unsigned src_end_pos);
bool _pw_string_append_substring_u32(PwValuePtr dest, char32_t*  src, unsigned src_start_pos, unsigned src_end_pos);
bool _pw_string_append_substring    (PwValuePtr dest, PwValuePtr src, unsigned src_start_pos, unsigned src_end_pos);

static inline bool _pw_string_append_substring_u8_wrapper(PwValuePtr dest, char* src, unsigned src_start_pos, unsigned src_end_pos)
{
    return _pw_string_append_substring_u8(dest, (char8_t*) src, src_start_pos, src_end_pos);
}

/****************************************************************
 * Substring comparison functions.
 *
 * Compare `str_a` from `start_pos` to `end_pos` with `str_b`.
 */

#define pw_substring_eq(a, start_pos, end_pos, b) _Generic((b), \
             char*: _pw_substring_eq_u8_wrapper,  \
          char8_t*: _pw_substring_eq_u8,          \
         char32_t*: _pw_substring_eq_u32,         \
        PwValuePtr: _pw_substring_eq              \
    )((a), (start_pos), (end_pos), (b))

bool _pw_substring_eq_u8 (PwValuePtr a, unsigned start_pos, unsigned end_pos, char8_t*   b);
bool _pw_substring_eq_u32(PwValuePtr a, unsigned start_pos, unsigned end_pos, char32_t*  b);
bool _pw_substring_eq    (PwValuePtr a, unsigned start_pos, unsigned end_pos, PwValuePtr b);

static inline bool _pw_substring_eq_u8_wrapper(PwValuePtr a, unsigned start_pos, unsigned end_pos, char* b)
{
    return _pw_substring_eq_u8(a, start_pos, end_pos, (char8_t*) b);
}


#define pw_startswith(str, prefix) _Generic((prefix), \
               int: _pw_startswith_c32,  \
          char32_t: _pw_startswith_c32,  \
             char*: _pw_startswith_u8_wrapper,  \
          char8_t*: _pw_startswith_u8,   \
         char32_t*: _pw_startswith_u32,  \
        PwValuePtr: _pw_startswith       \
    )((str), (prefix))

bool _pw_startswith_c32(PwValuePtr str, char32_t   prefix);
bool _pw_startswith_u8 (PwValuePtr str, char8_t*   prefix);
bool _pw_startswith_u32(PwValuePtr str, char32_t*  prefix);
bool _pw_startswith    (PwValuePtr str, PwValuePtr prefix);

static inline bool _pw_startswith_u8_wrapper(PwValuePtr str, char* prefix)
{
    return _pw_startswith_u8(str, (char8_t*) prefix);
}


#define pw_endswith(str, suffix) _Generic((suffix), \
               int: _pw_endswith_c32,  \
          char32_t: _pw_endswith_c32,  \
             char*: _pw_endswith_u8_wrapper,  \
          char8_t*: _pw_endswith_u8,   \
         char32_t*: _pw_endswith_u32,  \
        PwValuePtr: _pw_endswith       \
    )((str), (suffix))

bool _pw_endswith_c32(PwValuePtr str, char32_t   suffix);
bool _pw_endswith_u8 (PwValuePtr str, char8_t*   suffix);
bool _pw_endswith_u32(PwValuePtr str, char32_t*  suffix);
bool _pw_endswith    (PwValuePtr str, PwValuePtr suffix);

static inline bool _pw_endswith_u8_wrapper(PwValuePtr str, char* suffix)
{
    return _pw_endswith_u8(str, (char8_t*) suffix);
}


/****************************************************************
 * Split functions.
 * Return array of strings.
 * maxsplit == 0 imposes no limit
 */

PwResult pw_string_split(PwValuePtr str, unsigned maxsplit);  // split by spaces
PwResult pw_string_split_chr(PwValuePtr str, char32_t splitter, unsigned maxsplit);
PwResult pw_string_rsplit_chr(PwValuePtr str, char32_t splitter, unsigned maxsplit);

#define pw_string_split_any(str, splitters, maxsplit) _Generic((splitters),  \
                 char*: _pw_string_split_any_u8_wrapper, \
              char8_t*: _pw_string_split_any_u8,         \
             char32_t*: _pw_string_split_any_u32,        \
            PwValuePtr: _pw_string_split_any             \
    )((str), (splitters), (maxsplit))

PwResult _pw_string_split_any_u8 (PwValuePtr str, char8_t*   splitters, unsigned maxsplit);
PwResult _pw_string_split_any_u32(PwValuePtr str, char32_t*  splitters, unsigned maxsplit);
PwResult _pw_string_split_any    (PwValuePtr str, PwValuePtr splitters, unsigned maxsplit);

static inline PwResult _pw_string_split_any_u8_wrapper(PwValuePtr str, char* splitters, unsigned maxsplit)
{
    return _pw_string_split_any_u8(str, (char8_t*) splitters, maxsplit);
}

#define pw_string_split_strict(str, splitter, maxsplit) _Generic((splitter),  \
              char32_t: _pw_string_split_c32,               \
                   int: _pw_string_split_c32,               \
                 char*: _pw_string_split_strict_u8_wrapper, \
              char8_t*: _pw_string_split_strict_u8,         \
             char32_t*: _pw_string_split_strict_u32,        \
            PwValuePtr: _pw_string_split_strict             \
    )((str), (splitter), (maxsplit))

PwResult _pw_string_split_strict_u8 (PwValuePtr str, char8_t*   splitter, unsigned maxsplit);
PwResult _pw_string_split_strict_u32(PwValuePtr str, char32_t*  splitter, unsigned maxsplit);
PwResult _pw_string_split_strict    (PwValuePtr str, PwValuePtr splitter, unsigned maxsplit);

static inline PwResult _pw_string_split_strict_u8_wrapper(PwValuePtr str, char* splitter, unsigned maxsplit)
{
    return _pw_string_split_strict_u8(str, (char8_t*) splitter, maxsplit);
}


/****************************************************************
 * String variable declarations and rvalues with initialization
 */

#define __PWDECL_String_1_12(name, len, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11)  \
    /* declare String variable, character size 1 byte, up to 12 chars */  \
    _PwValue name = {  \
        ._emb_string_type_id = PwTypeId_String,  \
        .str_embedded = 1,  \
        .str_embedded_length = (len),  \
        .str_1[0] = (c0),  \
        .str_1[1] = (c1),  \
        .str_1[2] = (c2),  \
        .str_1[3] = (c3),  \
        .str_1[4] = (c4),  \
        .str_1[5] = (c5),  \
        .str_1[6] = (c6),  \
        .str_1[7] = (c7),  \
        .str_1[8] = (c8),  \
        .str_1[9] = (c9),  \
        .str_1[10] = (c10), \
        .str_1[11] = (c11) \
    }

#define PWDECL_String_1_12(name, len, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11)  \
    _PW_VALUE_CLEANUP __PWDECL_String_1_12(name, (len), (c0), (c1), (c2), (c3), (c4), (c5), (c6), (c7), (c8), (c9), (c10), (c11))

#define PwString_1_12(len, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11)  \
    /* make String rvalue, character size 1 byte, up to 12 chars */  \
    ({  \
        __PWDECL_String_1_12(v, (len), (c0), (c1), (c2), (c3), (c4), (c5), (c6), (c7), (c8), (c9), (c10), (c11));  \
        static_assert((len) <= PW_LENGTH(v.str_1));  \
        v;  \
    })

#define __PWDECL_String_2_6(name, len, c0, c1, c2, c3, c4, c5)  \
    /* declare String variable, character size 2 bytes, up to 6 chars */  \
    _PwValue name = {  \
        ._emb_string_type_id = PwTypeId_String,  \
        .str_embedded = 1,  \
        .str_embedded_char_size = 1,  \
        .str_embedded_length = (len),  \
        .str_2[0] = (c0),  \
        .str_2[1] = (c1),  \
        .str_2[2] = (c2),  \
        .str_2[3] = (c3),  \
        .str_2[4] = (c4),  \
        .str_2[5] = (c5)   \
    }

#define PWDECL_String_2_6(name, len, c0, c1, c2, c3, c4, c5)  \
    _PW_VALUE_CLEANUP __PWDECL_String_2_6(name, (len), (c0), (c1), (c2), (c3), (c4), (c5))

#define PwString_2_6(len, c0, c1, c2, c3, c4, c5)  \
    /* make String rvalue, character size 2 bytes, up to 6 chars */  \
    ({  \
        __PWDECL_String_2_6(v, (len), (c0), (c1), (c2), (c3), (c4), (c5));  \
        static_assert((len) <= PW_LENGTH(v.str_2));  \
        v;  \
    })

#define __PWDECL_String_3_4(name, len, c0, c1, c2, c3)  \
    /* declare String variable, character size 3 bytes, up to 4 chars */  \
    _PwValue name = {  \
        ._emb_string_type_id = PwTypeId_String,  \
        .str_embedded = 1,  \
        .str_embedded_char_size = 2,  \
        .str_embedded_length = (len),  \
        .str_3[0] = {{ [0] = (uint8_t) (c0), [1] = (uint8_t) ((c0) >> 8), [2] = (uint8_t) ((c0) >> 16) }},  \
        .str_3[1] = {{ [0] = (uint8_t) (c1), [1] = (uint8_t) ((c1) >> 8), [2] = (uint8_t) ((c1) >> 16) }},  \
        .str_3[2] = {{ [0] = (uint8_t) (c2), [1] = (uint8_t) ((c2) >> 8), [2] = (uint8_t) ((c2) >> 16) }},  \
        .str_3[3] = {{ [0] = (uint8_t) (c3), [1] = (uint8_t) ((c3) >> 8), [2] = (uint8_t) ((c3) >> 16) }}   \
    }

#define PWDECL_String_3_4(name, len, c0, c1, c2, c3)  \
    _PW_VALUE_CLEANUP __PWDECL_String_3_4(name, (len), (c0), (c1), (c2), (c3))

#define PwString_3_4(len, c0, c1, c2, c3)  \
    /* make String rvalue, character size 3 bytes, up to 4 chars */  \
    ({  \
        __PWDECL_String_3_4(v, (len), (c0), (c1), (c2), (c3));  \
        static_assert((len) <= PW_LENGTH(v.str_3));  \
        v;  \
    })

#define __PWDECL_String_4_3(name, len, c0, c1, c2)  \
    /* declare String variable, character size 4 bytes, up to 3 chars */  \
    _PwValue name = {  \
        ._emb_string_type_id = PwTypeId_String,  \
        .str_embedded = 1,  \
        .str_embedded_char_size = 3,  \
        .str_embedded_length = (len),  \
        .str_4[0] = (c0),  \
        .str_4[1] = (c1),  \
        .str_4[2] = (c2)   \
    }

#define PWDECL_String_4_3(name, len, c0, c1, c2)  \
    _PW_VALUE_CLEANUP __PWDECL_String_4_3(name, (len), (c0), (c1), (c2))

#define PwString_4_3(len, c0, c1, c2)  \
    /* make String rvalue, character size 4 bytes, up to 3 chars */  \
    ({  \
        __PWDECL_String_4_3(v, (len), (c0), (c1), (c2));  \
        static_assert((len) <= PW_LENGTH(v.str_4));  \
        v;  \
    })

/****************************************************************
 * C strings
 */

typedef char* CStringPtr;

// automatically cleaned value
#define CString [[ gnu::cleanup(pw_destroy_cstring) ]] CStringPtr

// somewhat ugly macro to define a local variable initialized with a copy of pw string:
#define PW_CSTRING_LOCAL(variable_name, pw_str) \
    char variable_name[pw_strlen_in_utf8(pw_str) + 1]; \
    pw_string_to_utf8_buf((pw_str), variable_name)

CStringPtr pw_string_to_utf8(PwValuePtr str);
/*
 * Create C string.
 */

void pw_destroy_cstring(CStringPtr* str);


#ifdef __cplusplus
}
#endif
