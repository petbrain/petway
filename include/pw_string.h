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

[[nodiscard]] bool pw_substr(PwValuePtr str, unsigned start_pos, unsigned end_pos, PwValuePtr result);
/*
 * Get substring from `start_pos` to `end_pos`.
 */

[[nodiscard]] bool pw_string_erase(PwValuePtr str, unsigned start_pos, unsigned end_pos);
/*
 * Erase characters from `start_pos` to `end_pos`.
 * This may make a copy of string, so checking return value is mandatory.
 */

[[nodiscard]] bool pw_string_truncate(PwValuePtr str, unsigned position);
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

[[nodiscard]] bool pw_string_ltrim(PwValuePtr str);
[[nodiscard]] bool pw_string_rtrim(PwValuePtr str);
[[nodiscard]] bool pw_string_trim(PwValuePtr str);

[[nodiscard]] bool pw_string_lower(PwValuePtr str);
[[nodiscard]] bool pw_string_upper(PwValuePtr str);

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

#define pw_strcat(result, str, ...)  \
    _pw_strcat_va((result), (str) __VA_OPT__(,) __VA_ARGS__, PwVaEnd())

[[nodiscard]] bool _pw_strcat_va(PwValuePtr result, ...);
[[nodiscard]] bool _pw_strcat_ap(PwValuePtr result, va_list ap);

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
#   define pw_isblank(c)  u_isblank(c)
#   define pw_isalpha(c)  u_isalpha(c)
#   define pw_isdigit(c)  u_isdigit(c)
#   define pw_isalnum(c)  u_isalnum(c)
#   define pw_ispunct(c)  u_ispunct(c)
#   define pw_iscntrl(c)  u_iscntrl(c)
#   define pw_isgraph(c)  u_isgraph(c)
#   define pw_isprint(c)  u_isprint(c)
#else
#   define pw_isspace(c)  isspace(c)
#   define pw_isblank(c)  isblank(c)
#   define pw_isalpha(c)  isalpha(c)
#   define pw_isdigit(c)  isdigit(c)
#   define pw_isalnum(c)  isalnum(c)
#   define pw_ispunct(c)  ispunct(c)
#   define pw_iscntrl(c)  iscntrl(c)
#   define pw_isgraph(c)  isgraph(c)
#   define pw_isprint(c)  isprint(c)
#endif
/*
 * Return true if `c` is a whitespace character.
 */

#define pw_is_ascii_digit(c)  isdigit(c)
/*
 * Return true if `c` is an ASCII digit.
 * Do not consider any other unicode digits because this function
 * is used in conjunction with standard C library (string to number conversion)
 * that does not support unicode character classification.
 */

bool pw_string_isspace(PwValuePtr str);
bool pw_string_isdigit(PwValuePtr str);
bool pw_string_is_ascii_digit(PwValuePtr str);
/*
 * Return true if `str` is not empty and contains all digits.
 */

/****************************************************************
 * Constructors
 */

#define pw_create_string(initializer, result) _Generic((initializer),  \
                 char*: _pw_create_string_ascii, \
              char8_t*: _pw_create_string_u8,    \
             char32_t*: _pw_create_string_u32,   \
            PwValuePtr: _pw_create_string        \
    )((initializer), (result))

[[nodiscard]] bool _pw_create_string_u8 (char8_t*   initializer, PwValuePtr result);
[[nodiscard]] bool _pw_create_string_u32(char32_t*  initializer, PwValuePtr result);
[[nodiscard]] bool _pw_create_string    (PwValuePtr initializer, PwValuePtr result);

[[nodiscard]] static inline bool _pw_create_string_ascii(char* initializer, PwValuePtr result)
{
    return _pw_create_string_u8((char8_t*) initializer, result);
}

[[nodiscard]] bool pw_create_empty_string(unsigned capacity, uint8_t char_size, PwValuePtr result);

/****************************************************************
 * Append functions
 */

#define pw_string_append(dest, src) _Generic((src),  \
              char32_t: _pw_string_append_c32,    \
                   int: _pw_string_append_c32,    \
                 char*: _pw_string_append_ascii,  \
              char8_t*: _pw_string_append_u8,     \
             char32_t*: _pw_string_append_u32,    \
            PwValuePtr: _pw_string_append         \
    )((dest), (src))

[[nodiscard]] bool _pw_string_append_c32(PwValuePtr dest, char32_t   c);
[[nodiscard]] bool _pw_string_append_u8 (PwValuePtr dest, char8_t*   src);
[[nodiscard]] bool _pw_string_append_u32(PwValuePtr dest, char32_t*  src);
[[nodiscard]] bool _pw_string_append    (PwValuePtr dest, PwValuePtr src);

[[nodiscard]] static inline bool _pw_string_append_ascii(PwValuePtr dest, char* src)
{
    return _pw_string_append_u8(dest, (char8_t*) src);
}

[[nodiscard]] bool pw_string_append_utf8(PwValuePtr dest, char8_t* buffer, unsigned size, unsigned* bytes_processed);
/*
 * Append UTF-8-encoded characters from `buffer`.
 * Write the number of bytes processed to `bytes_processed`, which can be less
 * than `size` if buffer ends with incomplete UTF-8 sequence.
 *
 * Return false if out of memory.
 */

[[nodiscard]] bool pw_string_append_buffer(PwValuePtr dest, uint8_t* buffer, unsigned size);
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

[[nodiscard]] bool _pw_string_insert_many_c32(PwValuePtr str, unsigned position, char32_t chr, unsigned n);

/****************************************************************
 * Append substring functions.
 *
 * Append `src` substring starting from `src_start_pos` to `src_end_pos`.
 */

#define pw_string_append_substring(dest, src, src_start_pos, src_end_pos) _Generic((src), \
                 char*: _pw_string_append_substring_ascii,  \
              char8_t*: _pw_string_append_substring_u8,     \
             char32_t*: _pw_string_append_substring_u32,    \
            PwValuePtr: _pw_string_append_substring         \
    )((dest), (src), (src_start_pos), (src_end_pos))

[[nodiscard]] bool _pw_string_append_substring_u8 (PwValuePtr dest, char8_t*   src, unsigned src_start_pos, unsigned src_end_pos);
[[nodiscard]] bool _pw_string_append_substring_u32(PwValuePtr dest, char32_t*  src, unsigned src_start_pos, unsigned src_end_pos);
[[nodiscard]] bool _pw_string_append_substring    (PwValuePtr dest, PwValuePtr src, unsigned src_start_pos, unsigned src_end_pos);

[[nodiscard]] static inline bool
_pw_string_append_substring_ascii(PwValuePtr dest, char* src, unsigned src_start_pos, unsigned src_end_pos)
{
    return _pw_string_append_substring_u8(dest, (char8_t*) src, src_start_pos, src_end_pos);
}

/****************************************************************
 * Substring comparison functions.
 *
 * Compare `str_a` from `start_pos` to `end_pos` with `str_b`.
 */

#define pw_substring_eq(a, start_pos, end_pos, b) _Generic((b), \
             char*: _pw_substring_eq_ascii,  \
          char8_t*: _pw_substring_eq_u8,     \
         char32_t*: _pw_substring_eq_u32,    \
        PwValuePtr: _pw_substring_eq         \
    )((a), (start_pos), (end_pos), (b))

bool _pw_substring_eq_u8 (PwValuePtr a, unsigned start_pos, unsigned end_pos, char8_t*   b);
bool _pw_substring_eq_u32(PwValuePtr a, unsigned start_pos, unsigned end_pos, char32_t*  b);
bool _pw_substring_eq    (PwValuePtr a, unsigned start_pos, unsigned end_pos, PwValuePtr b);

static inline bool _pw_substring_eq_ascii(PwValuePtr a, unsigned start_pos, unsigned end_pos, char* b)
{
    return _pw_substring_eq_u8(a, start_pos, end_pos, (char8_t*) b);
}


#define pw_startswith(str, prefix) _Generic((prefix), \
               int: _pw_startswith_c32,    \
          char32_t: _pw_startswith_c32,    \
             char*: _pw_startswith_ascii,  \
          char8_t*: _pw_startswith_u8,     \
         char32_t*: _pw_startswith_u32,    \
        PwValuePtr: _pw_startswith         \
    )((str), (prefix))

bool _pw_startswith_c32(PwValuePtr str, char32_t   prefix);
bool _pw_startswith_u8 (PwValuePtr str, char8_t*   prefix);
bool _pw_startswith_u32(PwValuePtr str, char32_t*  prefix);
bool _pw_startswith    (PwValuePtr str, PwValuePtr prefix);

static inline bool _pw_startswith_ascii(PwValuePtr str, char* prefix)
{
    return _pw_startswith_u8(str, (char8_t*) prefix);
}


#define pw_endswith(str, suffix) _Generic((suffix), \
               int: _pw_endswith_c32,    \
          char32_t: _pw_endswith_c32,    \
             char*: _pw_endswith_ascii,  \
          char8_t*: _pw_endswith_u8,     \
         char32_t*: _pw_endswith_u32,    \
        PwValuePtr: _pw_endswith         \
    )((str), (suffix))

bool _pw_endswith_c32(PwValuePtr str, char32_t   suffix);
bool _pw_endswith_u8 (PwValuePtr str, char8_t*   suffix);
bool _pw_endswith_u32(PwValuePtr str, char32_t*  suffix);
bool _pw_endswith    (PwValuePtr str, PwValuePtr suffix);

static inline bool _pw_endswith_ascii(PwValuePtr str, char* suffix)
{
    return _pw_endswith_u8(str, (char8_t*) suffix);
}


/****************************************************************
 * Split functions.
 * Return array of strings.
 * maxsplit == 0 imposes no limit
 */

[[nodiscard]] bool pw_string_split(PwValuePtr str, unsigned maxsplit, PwValuePtr result);  // split by spaces
[[nodiscard]] bool pw_string_split_chr(PwValuePtr str, char32_t splitter, unsigned maxsplit, PwValuePtr result);
[[nodiscard]] bool pw_string_rsplit_chr(PwValuePtr str, char32_t splitter, unsigned maxsplit, PwValuePtr result);

#define pw_string_split_any(str, splitters, maxsplit) _Generic((splitters),  \
                 char*: _pw_string_split_any_ascii,  \
              char8_t*: _pw_string_split_any_u8,     \
             char32_t*: _pw_string_split_any_u32,    \
            PwValuePtr: _pw_string_split_any         \
    )((str), (splitters), (maxsplit))

[[nodiscard]] bool _pw_string_split_any_u8 (PwValuePtr str, char8_t*   splitters, unsigned maxsplit, PwValuePtr result);
[[nodiscard]] bool _pw_string_split_any_u32(PwValuePtr str, char32_t*  splitters, unsigned maxsplit, PwValuePtr result);
[[nodiscard]] bool _pw_string_split_any    (PwValuePtr str, PwValuePtr splitters, unsigned maxsplit, PwValuePtr result);

[[nodiscard]] static inline bool _pw_string_split_any_ascii(PwValuePtr str, char* splitters, unsigned maxsplit, PwValuePtr result)
{
    return _pw_string_split_any_u8(str, (char8_t*) splitters, maxsplit, result);
}

#define pw_string_split_strict(str, splitter, maxsplit) _Generic((splitter),  \
              char32_t: _pw_string_split_c32,           \
                   int: _pw_string_split_c32,           \
                 char*: _pw_string_split_strict_ascii,  \
              char8_t*: _pw_string_split_strict_u8,     \
             char32_t*: _pw_string_split_strict_u32,    \
            PwValuePtr: _pw_string_split_strict         \
    )((str), (splitter), (maxsplit))

[[nodiscard]] bool _pw_string_split_strict_u8 (PwValuePtr str, char8_t*   splitter, unsigned maxsplit, PwValuePtr result);
[[nodiscard]] bool _pw_string_split_strict_u32(PwValuePtr str, char32_t*  splitter, unsigned maxsplit, PwValuePtr result);
[[nodiscard]] bool _pw_string_split_strict    (PwValuePtr str, PwValuePtr splitter, unsigned maxsplit, PwValuePtr result);

[[nodiscard]] static inline bool _pw_string_split_strict_ascii(PwValuePtr str, char* splitter, unsigned maxsplit, PwValuePtr result)
{
    return _pw_string_split_strict_u8(str, (char8_t*) splitter, maxsplit, result);
}


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
