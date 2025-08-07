#pragma once

/*
 * String internals.
 */

#include <string.h>

#include "include/pw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PWSTRING_BLOCK_SIZE    16
/*
 * The string is allocated in blocks.
 * Given that typical allocator's granularity is 16,
 * there's no point to make block size less than that.
 */

extern PwType _pw_string_type;

void _pw_string_destroy(PwValuePtr self);
/*
 * Destructor.
 */

/****************************************************************
 * Low level helper functions.
 */

static unsigned embedded_capacity[5] = {0, 12, 6, 4, 3};

static inline unsigned _pw_string_capacity(PwValuePtr s)
{
    if (s->embedded) {
        return embedded_capacity[s->char_size];
    }
    if (s->allocated) {
        return s->string_data->capacity;
    }
    return 0;
}

static inline void _pw_string_set_length(PwValuePtr s, unsigned length)
{
    if (s->embedded) {
        s->embedded_length = length;
    } else if (s->allocated) {
        s->length = length;
    } else {
        pw_panic("Cannot set length for static string\n");
    }
}

static inline unsigned _pw_string_inc_length(PwValuePtr s, unsigned increment)
/*
 * Increment length, return previous value.
 */
{
    unsigned length;
    if (s->embedded) {
        length = s->embedded_length;
        s->embedded_length = length + increment;
    } else if (s->allocated) {
        length = s->length;
        s->length = length + increment;
    } else {
        pw_panic("Cannot increase length of static string\n");
    }
    return length;
}

static inline uint8_t* _pw_string_char_ptr(PwValuePtr str, unsigned position)
/*
 * Return address of character in string at `position`.
 * If position is beyond end of line return nullptr.
 */
{
    unsigned offset = position * str->char_size;
    if (str->embedded) {
        return &str->str_1[offset];
    } else if (str->allocated) {
        return &str->string_data->data[offset];
    } else {
        return ((uint8_t*) str->char_ptr) + offset;
    }
    return nullptr;
}

unsigned _pw_calc_string_data_size(uint8_t char_size, unsigned desired_capacity, unsigned* real_capacity);
/*
 * Calculate memory size for allocated string data.
 */

static inline unsigned _pw_allocated_string_data_size(PwValuePtr str)
/*
 * Get memory size occupied by allocated string data.
 */
{
    return _pw_calc_string_data_size(str->char_size, _pw_string_capacity(str), nullptr);
}

[[ nodiscard]] bool _pw_make_empty_string(PwTypeId type_id, unsigned capacity, uint8_t char_size, PwValuePtr result);
/*
 * Create empty string with desired parameters.
 * `type_id` is important for allocator selection.
 */

[[ nodiscard]] bool _pw_expand_string(PwValuePtr str, unsigned increment, uint8_t new_char_size);
/*
 * Expand string in place, if necessary, replacing `str->string_data`.
 *
 * If string refcount is greater than 1, always make a copy of `str->string_data`
 * because the string is about to be updated.
 *
 * Do not increase length of string, just ensure capacity is sufficient for increment.
 */

[[ nodiscard]] static inline bool _pw_string_need_copy_on_write(PwValuePtr str)
{
    if (str->embedded) {
        return false;
    }
    if (str->allocated) {
        _PwStringData* sdata = str->string_data;
        return sdata->refcount > 1;
    }
    // static string needs to be copied
    return true;
}

[[ nodiscard]] bool _pw_string_do_copy_on_write(PwValuePtr str);
/*
 * non-inline helper for _pw_string_copy_on_write
 */

[[ nodiscard]] static inline bool _pw_string_copy_on_write(PwValuePtr str)
/*
 * If `str` is not embedded and its refcount is greater than 1,
 * make a copy of allocated string data and decrement refcount
 * of original data.
 *
 * This function is called when a string is going to be modified inplace
 * without expanding or increasing char size.
 */
{
    if (_pw_string_need_copy_on_write(str)) {
        return _pw_string_do_copy_on_write(str);
    }
    return true;
}

/****************************************************************
 * Character width functions
 */

static inline uint8_t calc_char_size(char32_t c)
{
    return (32 - __builtin_clz(c | 1) + 7) >> 3;
/*
    if (c < 256) {
        return 1;
    } else if (c < 65536) {
        return 2;
    } else if (c < 16777216) {
        return 3;
    } else {
        return 4;
    }
*/
}

/****************************************************************
 * hash variants
 */

typedef void (*StrHash)(uint8_t* self_ptr, unsigned length, PwHashContext* ctx);

extern StrHash _pw_strhash_variants[5];

/****************************************************************
 * copy variants
 *
 * The caller must ensure destination string has sufficient capacity.
 * All pointers can be unaligned.
 */

typedef void (*StrCopy)(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);

extern StrCopy _pw_strcopy_variants[5][5];  // [dest_char_size][src_char_size]

void _pw_strcopy_n_n(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);
void _pw_strcopy_1_2(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);
void _pw_strcopy_1_3(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);
void _pw_strcopy_1_4(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);
void _pw_strcopy_2_1(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);
void _pw_strcopy_2_3(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);
void _pw_strcopy_2_4(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);
void _pw_strcopy_3_1(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);
void _pw_strcopy_3_2(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);
void _pw_strcopy_3_4(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);
void _pw_strcopy_4_1(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);
void _pw_strcopy_4_2(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);
void _pw_strcopy_4_3(uint8_t* dest_ptr, uint8_t* src_start_ptr, uint8_t* src_end_ptr);


/****************************************************************
 * append variants
 *
 * The caller must ensure destination string has sufficient capacity.
 * All pointers can be unaligned.
 */

typedef bool (*StrAppend)(PwValuePtr dest, unsigned dest_pos, uint8_t* src_start_ptr, uint8_t* src_end_ptr);

extern StrAppend _pw_str_append_variants[5][5];  // [dest_char_size][src_char_size]

/****************************************************************
 * strchr variants
 *
 * The caller must ensure start_ptr < end_ptr and codepoint is less than 1 << 8 * char_size.
 */

typedef uint8_t* (*StrChr)(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint);

extern StrChr _pw_strchr_variants[5];
extern StrChr _pw_strchri_variants[5];

/****************************************************************
 * strchr2 variants
 *
 * Same as strchr, but also returnd maximal char size.
 *
 * The caller must ensure start_ptr < end_ptr and codepoint is less than 1 << 8 * char_size.
 */

typedef uint8_t* (*StrChr2)(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint, char8_t* char_size);

extern StrChr2 _pw_strchr2_variants[5];

/****************************************************************
 * strrchr2 variants
 *
 * The caller must ensure start_ptr < end_ptr and codepoint is less than 1 << 8 * char_size.
 */

typedef uint8_t* (*StrRChr2)(uint8_t* start_ptr, uint8_t* end_ptr, char32_t codepoint, char8_t* char_size);

extern StrRChr2 _pw_strrchr2_variants[5];

/****************************************************************
 * equalz variants
 */

typedef int (*StrEqualZ)(uint8_t* a_start_ptr, unsigned a_length, uint8_t* b_start_ptr);

#define PW_NEQ  0  // a != b
#define PW_EQ   1  // exact match
#define PW_EQ_PARTIAL  2  // a == b, but a is longer than b

extern StrEqualZ _pw_str_equalz_variants[5][5];  // [a_char_size][b_char_size]
extern StrEqualZ _pw_str_equalzi_variants[5][5];

/****************************************************************
 * substreq variants
 *
 * The caller must ensure substr is within str.
 */

typedef bool (*SubstrEq)(uint8_t* str_start_ptr, uint8_t* substr_start_ptr, uint8_t* substr_end_ptr);

extern SubstrEq _pw_substreq_variants[5][5];  // [str_char_size][substr_char_size]
extern SubstrEq _pw_substreqi_variants[5][5];


/****************************************************************
 * skip_chars variants
 */

typedef uint8_t* (*StrSkipChars)(uint8_t* start_ptr, uint8_t* end_ptr, char32_t* skipchars);

extern StrSkipChars _pw_skip_chars_variants[5];

/****************************************************************
 * skip_spaces variants
 */

typedef uint8_t* (*StrSkipSpaces)(uint8_t* start_ptr, uint8_t* end_ptr);

extern StrSkipSpaces _pw_skip_spaces_variants[5];

/****************************************************************
 * Misc. functions
 */

void _pw_string_dump_data(FILE* fp, PwValuePtr str, int indent);
/*
 * Helper function for _pw_string_dump.
 */

#ifdef __cplusplus
}
#endif
