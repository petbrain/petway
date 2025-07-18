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

/****************************************************************
 * Basic functions
 */

static inline uint8_t* _pw_string_start(PwValuePtr str)
/*
 * Return pointer to the start of internal string data.
 */
{
    if (str->embedded) {
        return str->str_1;
    } else if (str->allocated) {
        return str->string_data->data;
    } else {
        return str->char_ptr;
    }
}

static inline uint8_t* _pw_string_start_end(PwValuePtr str, uint8_t** end)
/*
 * Return pointer to the start of internal string data
 * and write final pointer to `end`.
 */
{
    uint8_t* start_ptr;
    uint8_t char_size = str->char_size;
    if (str->embedded) {
        start_ptr = str->str_1;
        *end = start_ptr + str->embedded_length * char_size;
    } else if (str->allocated) {
        start_ptr = str->string_data->data;
        *end = start_ptr + str->length * char_size;
    } else {
        // static string
        start_ptr = str->char_ptr;
        *end = start_ptr + str->length * char_size;
    }
    return start_ptr;
}

static inline uint8_t* _pw_string_start_length(PwValuePtr str, unsigned* length)
/*
 * Return pointer to the start of internal string data
 * and write length of string to `length`.
 */
{
   if (str->embedded) {
        *length = str->embedded_length;
        return str->str_1;
    } else if (str->allocated) {
        *length = str->length;
        return str->string_data->data;
    } else {
        *length = str->length;
        return str->char_ptr;
    }
}

[[noreturn]]
void _pw_panic_bad_char_size(uint8_t char_size);

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

static inline uint8_t* _pw_put_char(uint8_t* ptr, char32_t chr, uint8_t char_size)
{
    switch (char_size) {
        case 1: _pw_put_char_uint8_t(ptr, chr); break;
        case 2: _pw_put_char_uint16_t(ptr, chr); break;
        case 3: _pw_put_char_uint24_t(ptr, chr); break;
        case 4: _pw_put_char_uint32_t(ptr, chr); break;
        default: _pw_panic_bad_char_size(char_size);
    }
    return ptr + char_size;
}

char32_t pw_char_at(PwValuePtr str, unsigned position);
/*
 * Return character at `position`.
 * If position is beyond end of line return 0.
 */

// check if `index` is within string length
#define pw_string_index_valid(str, index) ((index) < pw_strlen(str))

static unsigned inline pw_strlen(PwValuePtr str)
/*
 * Return length of string.
 */
{
    pw_assert_string(str);
    if (str->embedded) {
        return str->embedded_length;
    } else {
        return str->length;
    }
}

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
              char8_t*: _pw_create_string_utf8,  \
             char32_t*: _pw_create_string_utf32, \
            PwValuePtr: _pw_create_string        \
    )((initializer), (result))

[[nodiscard]] bool _pw_create_string_ascii(char*      initializer, PwValuePtr result);
[[nodiscard]] bool _pw_create_string_utf8 (char8_t*   initializer, PwValuePtr result);
[[nodiscard]] bool _pw_create_string_utf32(char32_t*  initializer, PwValuePtr result);
[[nodiscard]] bool _pw_create_string      (PwValuePtr initializer, PwValuePtr result);

[[nodiscard]] bool pw_create_empty_string(unsigned capacity, uint8_t char_size, PwValuePtr result);

/****************************************************************
 * Append functions
 */

#define pw_string_append(dest, src) _Generic((src),  \
              char32_t: _pw_string_append_c32,    \
                   int: _pw_string_append_c32,    \
                 char*: _pw_string_append_ascii,  \
              char8_t*: _pw_string_append_utf8,   \
             char32_t*: _pw_string_append_utf32,  \
            PwValuePtr: _pw_string_append         \
    )((dest), (src))

[[nodiscard]] bool _pw_string_append_c32  (PwValuePtr dest, char32_t   c);
[[nodiscard]] bool _pw_string_append_ascii(PwValuePtr dest, char*      src);
[[nodiscard]] bool _pw_string_append_utf8 (PwValuePtr dest, char8_t*   src);
[[nodiscard]] bool _pw_string_append_utf32(PwValuePtr dest, char32_t*  src);
[[nodiscard]] bool _pw_string_append      (PwValuePtr dest, PwValuePtr src);

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
              char8_t*: _pw_string_append_substring_utf8,   \
             char32_t*: _pw_string_append_substring_utf32,  \
            PwValuePtr: _pw_string_append_substring         \
    )((dest), (src), (src_start_pos), (src_end_pos))

[[nodiscard]] bool _pw_string_append_substring_ascii(PwValuePtr dest, char*      src, unsigned src_start_pos, unsigned src_end_pos);
[[nodiscard]] bool _pw_string_append_substring_utf8 (PwValuePtr dest, char8_t*   src, unsigned src_start_pos, unsigned src_end_pos);
[[nodiscard]] bool _pw_string_append_substring_utf32(PwValuePtr dest, char32_t*  src, unsigned src_start_pos, unsigned src_end_pos);
[[nodiscard]] bool _pw_string_append_substring      (PwValuePtr dest, PwValuePtr src, unsigned src_start_pos, unsigned src_end_pos);


/****************************************************************
 * Substring comparison functions.
 *
 * Compare `str_a` from `start_pos` to `end_pos` with `str_b`.
 */

#define pw_substring_eq(a, start_pos, end_pos, b) _Generic((b), \
             char*: _pw_substring_eq_ascii,  \
          char8_t*: _pw_substring_eq_utf8,   \
         char32_t*: _pw_substring_eq_utf32,  \
        PwValuePtr: _pw_substring_eq         \
    )((a), (start_pos), (end_pos), (b))

bool _pw_substring_eq_ascii(PwValuePtr a, unsigned start_pos, unsigned end_pos, char*      b);
bool _pw_substring_eq_utf8 (PwValuePtr a, unsigned start_pos, unsigned end_pos, char8_t*   b);
bool _pw_substring_eq_utf32(PwValuePtr a, unsigned start_pos, unsigned end_pos, char32_t*  b);
bool _pw_substring_eq      (PwValuePtr a, unsigned start_pos, unsigned end_pos, PwValuePtr b);


#define pw_startswith(str, prefix) _Generic((prefix), \
               int: _pw_startswith_c32,    \
          char32_t: _pw_startswith_c32,    \
             char*: _pw_startswith_ascii,  \
          char8_t*: _pw_startswith_utf8,   \
         char32_t*: _pw_startswith_utf32,  \
        PwValuePtr: _pw_startswith         \
    )((str), (prefix))

bool _pw_startswith_c32  (PwValuePtr str, char32_t   prefix);
bool _pw_startswith_ascii(PwValuePtr str, char*      prefix);
bool _pw_startswith_utf8 (PwValuePtr str, char8_t*   prefix);
bool _pw_startswith_utf32(PwValuePtr str, char32_t*  prefix);
bool _pw_startswith      (PwValuePtr str, PwValuePtr prefix);


#define pw_endswith(str, suffix) _Generic((suffix), \
               int: _pw_endswith_c32,    \
          char32_t: _pw_endswith_c32,    \
             char*: _pw_endswith_ascii,  \
          char8_t*: _pw_endswith_utf8,   \
         char32_t*: _pw_endswith_utf32,  \
        PwValuePtr: _pw_endswith         \
    )((str), (suffix))

bool _pw_endswith_c32  (PwValuePtr str, char32_t   suffix);
bool _pw_endswith_ascii(PwValuePtr str, char*      suffix);
bool _pw_endswith_utf8 (PwValuePtr str, char8_t*   suffix);
bool _pw_endswith_utf32(PwValuePtr str, char32_t*  suffix);
bool _pw_endswith      (PwValuePtr str, PwValuePtr suffix);


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
              char8_t*: _pw_string_split_any_utf8,   \
             char32_t*: _pw_string_split_any_utf32,  \
            PwValuePtr: _pw_string_split_any         \
    )((str), (splitters), (maxsplit))

[[nodiscard]] bool _pw_string_split_any_ascii(PwValuePtr str, char*      splitters, unsigned maxsplit, PwValuePtr result);
[[nodiscard]] bool _pw_string_split_any_utf8 (PwValuePtr str, char8_t*   splitters, unsigned maxsplit, PwValuePtr result);
[[nodiscard]] bool _pw_string_split_any_utf32(PwValuePtr str, char32_t*  splitters, unsigned maxsplit, PwValuePtr result);
[[nodiscard]] bool _pw_string_split_any      (PwValuePtr str, PwValuePtr splitters, unsigned maxsplit, PwValuePtr result);

#define pw_string_split_strict(str, splitter, maxsplit) _Generic((splitter),  \
              char32_t: _pw_string_split_c32,           \
                   int: _pw_string_split_c32,           \
                 char*: _pw_string_split_strict_ascii,  \
              char8_t*: _pw_string_split_strict_utf8,   \
             char32_t*: _pw_string_split_strict_utf32,  \
            PwValuePtr: _pw_string_split_strict         \
    )((str), (splitter), (maxsplit))

[[nodiscard]] bool _pw_string_split_strict_ascii(PwValuePtr str, char*      splitter, unsigned maxsplit, PwValuePtr result);
[[nodiscard]] bool _pw_string_split_strict_utf8 (PwValuePtr str, char8_t*   splitter, unsigned maxsplit, PwValuePtr result);
[[nodiscard]] bool _pw_string_split_strict_utf32(PwValuePtr str, char32_t*  splitter, unsigned maxsplit, PwValuePtr result);
[[nodiscard]] bool _pw_string_split_strict      (PwValuePtr str, PwValuePtr splitter, unsigned maxsplit, PwValuePtr result);


/****************************************************************
 * Low level string iterator.
 *
 * The algorithm is implemented as macros and local variables
 * instead of functions and structures to give the compiler
 * more optimization freedom.
 *
 * How to use:
 *
 * PwStringPtr str = whatever;
 * PW_STRING_ITER(src, str)
 * while (!PW_STRING_ITER_DONE(src)) {
 *     char32_t chr = PW_STRING_ITER_NEXT(src);
 *     // ...
 * }
 */

#define PW_STRING_ITER_DONE(name)  \
    (_char_ptr_##name == _end_ptr_##name)

#define PW_STRING_ITER_GETCHAR(name)  \
    _pw_get_char(_char_ptr_##name, _char_size_##name)

#define PW_STRING_ITER_PUTCHAR(name, chr)  \
    _pw_put_char(_char_ptr_##name, (chr), _char_size_##name)

#define PW_STRING_ITER_CHARPTR(name)  \
    _char_ptr_##name

#define PW_STRING_ITER_ENDPTR(name)  \
    _end_ptr_##name

#define PW_STRING_ITER_INC(name);  \
    do {  \
        _char_ptr_##name += _char_size_##name;  \
        if (_pw_unlikely(_char_ptr_##name > _end_ptr_##name)) {  \
            fprintf(stderr, "PW_STRING_ITER_NEXT %s:%d char_ptr went above end_ptr\n", __FILE__, __LINE__);  \
            _char_ptr_##name = _end_ptr_##name;  \
        }  \
    } while (false)

#define PW_STRING_ITER_DEC(name);  \
        _char_ptr_##name -= _char_size_##name;  \
        if (_pw_unlikely(_char_ptr_##name < _end_ptr_##name)) {  \
            fprintf(stderr, "PW_STRING_ITER_PREV %s:%d char_ptr went below end_ptr\n", __FILE__, __LINE__);  \
            _char_ptr_##name = _end_ptr_##name;  \
        }  \

#define PW_STRING_ITER_NEXT(name)  \
    ({  \
        char32_t chr = PW_STRING_ITER_GETCHAR(name);  \
        PW_STRING_ITER_INC(name);  \
        chr;  \
    })

#define PW_STRING_ITER_PREV(name)  \
    ({  \
        PW_STRING_ITER_DEC(name);  \
        PW_STRING_ITER_GETCHAR(name);  \
    })

#define PW_STRING_ITER_RESET(name, s)  \
    do {  \
        _char_size_##name = (s)->char_size;  \
        _char_ptr_##name = _pw_string_start_end((s), &_end_ptr_##name);  \
    } while (false)

#define PW_STRING_ITER(name, s)  \
    /* Init forward string iterator */  \
    uint8_t _char_size_##name;  \
    uint8_t* _char_ptr_##name;  \
    uint8_t* _end_ptr_##name;  \
    PW_STRING_ITER_RESET(name, (s))

#define PW_STRING_ITER_RESET_FROM(name, s, start_index)  \
    do {  \
        _char_size_##name = (s)->char_size;  \
        _char_ptr_##name = _pw_string_start_end((s), &_end_ptr_##name);  \
        _char_ptr_##name += (start_index) * _char_size_##name;  \
        if (_char_ptr_##name > _end_ptr_##name) {  \
            /* start_index too big */  \
            _char_ptr_##name = _end_ptr_##name;  \
        }  \
    } while (false)

#define PW_STRING_ITER_FROM(name, s, start_index)  \
    /* Init forward string iterator from start_index */  \
    uint8_t _char_size_##name;  \
    uint8_t* _char_ptr_##name;  \
    uint8_t* _end_ptr_##name;  \
    PW_STRING_ITER_RESET_FROM(name, (s), (start_index))

#define PW_STRING_ITER_RESET_REVERSE(name, s)  \
    do {  \
        _char_size_##name = (s)->char_size;  \
        _end_ptr_##name = _pw_string_start_end((s), &_char_ptr_##name);  \
    } while (false)

#define PW_STRING_ITER_REVERSE(name, s)  \
    /* Init reverse string iterator */  \
    uint8_t _char_size_##name = (s)->char_size;  \
    uint8_t* _char_ptr_##name;  \
    uint8_t* _end_ptr_##name; \
    PW_STRING_ITER_RESET_REVERSE(name, (s));


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

unsigned pw_strlen_in_utf8(PwValuePtr str);
/*
 * Return length of str as if was encoded in UTF-8.
 */

CStringPtr pw_string_to_utf8(PwValuePtr str);
/*
 * Create C string.
 */

void pw_destroy_cstring(CStringPtr* str);


#ifdef __cplusplus
}
#endif
