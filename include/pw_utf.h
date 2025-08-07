#pragma once

/*
 * UTF-8/UTF-32 functions
 */

#include <pw_helpers.h>

#ifdef __cplusplus
extern "C" {
#endif

char32_t _pw_decode_utf8_char(char8_t** str);
/*
 * Decode UTF-8 character, update `*str`.
 *
 * Return decoded character or 0xFFFFFFFF if UTF-8 sequence is invalid.
 */

char32_t _pw_decode_utf8_char_reverse(char8_t** ptr);
/*
 * Decode UTF-8 character from `*ptr` downwards.
 *
 * Result is decoded character or 0xFFFFFFFF if UTF-8 sequence is invalid.
 */

bool _pw_decode_utf8_buffer(char8_t** ptr, unsigned* bytes_remaining, char32_t* result);
/*
 * Decode UTF-8 character from buffer, update `*ptr`.
 *
 * Null charaters are returned as zero codepoints.
 *
 * Return false if UTF-8 sequence is incomplete or `bytes_remaining` is zero.
 * Otherwise return true.
 * If character is invalid, write 0xFFFFFFFF to `result`.
 */

void pw_string_to_utf8_buf(PwValuePtr str, char* buffer);
void pw_substr_to_utf8_buf(PwValuePtr str, unsigned start_pos, unsigned end_pos, char* buffer);
/*
 * Copy string to buffer, appending terminating 0.
 * Use carefully. The caller is responsible to allocate the buffer.
 * Encode multibyte chars to UTF-8.
 */

unsigned pw_char32_to_utf8(char32_t codepoint, char* buffer);
/*
 * Write up to 4 characters to buffer.
 * Return number of characters written.
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

unsigned utf8_strlen3(char8_t* str, uint8_t* char_size, char8_t** end_ptr);
/*
 * Same as utf8_strlen2, plus writes the pointer to the terminating 0 character to `end_ptr`.
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
 * If `n` is greater than length of string, returned value points to terminating null character.
 */

unsigned utf32_strlen(char32_t* str);
/*
 * Find length of null-terminated `str`.
 */

unsigned utf32_strlen2(char32_t* str, uint8_t* char_size);
/*
 * Find both length of null-terminated `str` and max char size in one go.
 */

int utf32_strcmp     (char32_t* a, char32_t* b);
int utf32_strcmp_utf8(char32_t* a, char8_t*  b);
/*
 * Compare  null-terminated strings.
 */

char32_t* utf32_strchr(char32_t* str, char32_t chr);
/*
 * Find the first occurrence of `chr` in the null-terminated `str`.
 */

uint8_t utf32_char_size(char32_t* str, unsigned max_len);
/*
 * Find the maximal size of character in `str`, up to `max_len` or null terminator.
 */


#ifdef __cplusplus
}
#endif
