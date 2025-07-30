#include "include/pw.h"
#include "include/pw_utf.h"
#include "src/string/pw_string_internal.h"

char32_t _pw_decode_utf8_char(char8_t** str)
{
    char8_t* p = *str;
    char8_t c = *p++;
    if (c < 0x80) {
        *str = p;
        return  c;
    }

    char32_t codepoint;
    char8_t next;

#   define APPEND_NEXT         \
        next = *p++;           \
        if (_pw_unlikely(next == 0)) goto end_of_string; \
        if (_pw_unlikely((next & 0b1100'0000) != 0b1000'0000)) goto bad_utf8; \
        codepoint <<= 6;       \
        codepoint |= next & 0x3F;

    if ((c & 0b1110'0000) == 0b1100'0000) {
        codepoint = c & 0b0011'1111;
        APPEND_NEXT
    } else if ((c & 0b1111'0000) == 0b1110'0000) {
        codepoint = c & 0b0001'1111;
        APPEND_NEXT
        APPEND_NEXT
    } else if ((c & 0b1111'1000) == 0b1111'0000) {
        codepoint = c & 0b0000'1111;
        APPEND_NEXT
        APPEND_NEXT
        APPEND_NEXT
    } else {
        goto bad_utf8;
    }
    if (codepoint == 0) {
        // zero codepoint encoded with 2 or more bytes,
        // make it invalid to avoid mixing up with 1-byte null character
        codepoint = 0xFFFFFFFF;
/*
    } else if (0xD800 <= codepoint && codepoint <= 0xDFFF) {
        // surrogate pairs are prohibited, return inalid codepoint for them
        codepoint = 0xFFFFFFFF;
*/
    }
    *str = p;
    return codepoint;

end_of_string:
    *str = p;
    return 0;

bad_utf8:
    *str = --p;  // rollback to bad octet, will process it on the next call
    return 0xFFFFFFFF;

#   undef APPEND_NEXT
}

bool _pw_decode_utf8_char_reverse(char8_t** ptr, char8_t* str_start, char32_t* result)
{
    // XXX work in progress

    char32_t codepoint;
    char8_t c;
    char8_t next;
    char8_t* p = *ptr;
    char8_t* end_ptr = p;

    // seek to the start of UTF-8 sequence
    for (;;) {
        if (p <= str_start) {
            return false;
        }
        c = *--p;
        if (c < 0x80) {
            codepoint = c;
            goto done;
        }
        if ((c & 0b1100'0000) != 0b1000'0000) {
            break;
        }
    }
    char8_t* np = p + 1;

#   define APPEND_NEXT         \
        if (_pw_unlikely(np >= end_ptr)) goto bad_utf8; \
        next = *np++;          \
        if (_pw_unlikely((next & 0b1100'0000) != 0b1000'0000)) goto bad_utf8; \
        codepoint <<= 6;       \
        codepoint |= next & 0x3F;

    if ((c & 0b1110'0000) == 0b1100'0000) {
        codepoint = c & 0b0011'1111;
        APPEND_NEXT
    } else if ((c & 0b1111'0000) == 0b1110'0000) {
        codepoint = c & 0b0001'1111;
        APPEND_NEXT
        APPEND_NEXT
    } else if ((c & 0b1111'1000) == 0b1111'0000) {
        codepoint = c & 0b0000'1111;
        APPEND_NEXT
        APPEND_NEXT
        APPEND_NEXT
    } else {
        goto bad_utf8;
    }
    if (codepoint == 0) {
        // zero codepoint encoded with 2 or more bytes,
        // make it invalid to avoid mixing up with 1-byte null character
        codepoint = 0xFFFFFFFF;
        goto done;
    }
/*
    if (0xD800 <= codepoint && codepoint <= 0xDFFF) {
        // surrogate pairs are prohibited, return inalid codepoint for them
        codepoint = 0xFFFFFFFF;
        goto done;
    }
*/

done:
    *ptr = p;
    *result = codepoint;
    return true;

bad_utf8:
    *ptr = p;  // XXX ???
    return 0xFFFFFFFF;

#   undef APPEND_NEXT
}

bool _pw_decode_utf8_buffer(char8_t** ptr, unsigned* bytes_remaining, char32_t* result)
{
    char8_t* p = *ptr;
    unsigned remaining = *bytes_remaining;
    if (!remaining) {
        return false;
    }

    char32_t codepoint;
    char8_t next;

#   define APPEND_NEXT      \
        next = *p++;        \
        remaining--;        \
        if (_pw_unlikely((next & 0b1100'0000) != 0b1000'0000)) goto bad_utf8; \
        codepoint <<= 6;       \
        codepoint |= next & 0x3F;

    char8_t c = *p++;
    remaining--;
    if (c < 0x80) {
        codepoint = c;
        goto done;
    }
    if ((c & 0b1110'0000) == 0b1100'0000) {
        if (_pw_unlikely(!remaining)) return false;
        codepoint = c & 0b0011'1111;
        APPEND_NEXT
    } else if ((c & 0b1111'0000) == 0b1110'0000) {
        if (_pw_unlikely(remaining < 2)) return false;
        codepoint = c & 0b0001'1111;
        APPEND_NEXT
        APPEND_NEXT
    } else if ((c & 0b1111'1000) == 0b1111'0000) {
        if (_pw_unlikely(remaining < 3)) return false;
        codepoint = c & 0b0000'1111;
        APPEND_NEXT
        APPEND_NEXT
        APPEND_NEXT
    } else {
        goto bad_utf8;
    }
    if (codepoint == 0) {
        // zero codepoint encoded with 2 or more bytes,
        // make it invalid to avoid mixing up with 1-byte null character
        codepoint = 0xFFFFFFFF;
/*
    } else if (0xD800 <= codepoint && codepoint <= 0xDFFF) {
        // surrogate pairs are prohibited, return inalid codepoint for them
        codepoint = 0xFFFFFFFF;
*/
    }

done:
    *ptr = p;
    *bytes_remaining = remaining;
    *result = codepoint;
    return true;

bad_utf8:
    p--;  // rollback to bad octet, will process it on the next call
    codepoint = 0xFFFFFFFF;
    goto done;

#   undef APPEND_NEXT
}

char* pw_char32_to_utf8(char32_t codepoint, char* buffer)
{
    /*
     * U+0000 - U+007F      0xxxxxxx
     * U+0080 - U+07FF      110xxxxx  10xxxxxx
     * U+0800 - U+FFFF      1110xxxx  10xxxxxx  10xxxxxx
     * U+010000 - U+10FFFF  11110xxx  10xxxxxx  10xxxxxx  10xxxxxx
     */
    if (codepoint < 0x80) {
        *buffer++ = (char) codepoint;
        return buffer;
    }
    if (codepoint < 0b1'00000'000000) {
        *buffer++ = (char) (0xC0 | (codepoint >> 6));
        *buffer++ = (char) (0x80 | (codepoint & 0x3F));
        return buffer;
    }
    if (codepoint < 0b1'0000'000000'000000) {
        *buffer++ = (char) (0xE0 | (codepoint >> 12));
        *buffer++ = (char) (0x80 | ((codepoint >> 6) & 0x3F));
        *buffer++ = (char) (0x80 | (codepoint & 0x3F));
        return buffer;
    }
    *buffer++ = (char) (0xF0 | ((codepoint >> 18) & 0x07));
    *buffer++ = (char) (0x80 | ((codepoint >> 12) & 0x3F));
    *buffer++ = (char) (0x80 | ((codepoint >> 6) & 0x3F));
    *buffer++ = (char) (0x80 | (codepoint & 0x3F));
    return buffer;
}

unsigned utf8_strlen(char8_t* str)
{
    unsigned length = 0;
    while(*str != 0) {
        char32_t c = _pw_decode_utf8_char(&str);
        if (c != 0xFFFFFFFF) {
            length++;
        }
    }
    return length;
}

unsigned utf8_strlen2(char8_t* str, uint8_t* char_size)
{
    unsigned length = 0;
    char32_t width = 0;
    while(*str != 0) {
        char32_t c = _pw_decode_utf8_char(&str);
        if (c != 0xFFFFFFFF) {
            width |= c;
            length++;
        }
    }
    *char_size = calc_char_size(width);
    return length;
}

unsigned utf8_strlen2_buf(char8_t* buffer, unsigned* size, uint8_t* char_size)
{
    char8_t* ptr = buffer;
    unsigned bytes_remaining = *size;
    unsigned length = 0;
    char32_t width = 0;

    while (bytes_remaining) {
        char32_t c;
        if (!_pw_decode_utf8_buffer(&ptr, &bytes_remaining, &c)) {
            break;
        }
        if (c != 0xFFFFFFFF) {
            width |= c;
            length++;
        }
    }
    *size -= bytes_remaining;

    if (char_size) {
        *char_size = calc_char_size(width);
    }

    return length;
}

uint8_t utf8_char_size(char8_t* str, unsigned max_len)
{
    char32_t width = 0;
    while(*str != 0) {
        char32_t c = _pw_decode_utf8_char(&str);
        if (c != 0xFFFFFFFF) {
            width |= c;
        }
    }
    return calc_char_size(width);
}

char8_t* utf8_skip(char8_t* str, unsigned n)
{
    while(n--) {
        _pw_decode_utf8_char(&str);
        if (*str == 0) {
            break;
        }
    }
    return str;
}

void _pw_putchar32_utf8(FILE* fp, char32_t codepoint)
{
    char buffer[5];
    char* start = buffer;
    char* end = pw_char32_to_utf8(codepoint, buffer);
    while (start < end) {
        fputc(*start++, fp);
    }
}

unsigned utf32_strlen(char32_t* str)
{
    unsigned length = 0;
    while (*str++) {
        length++;
    }
    return length;
}

unsigned utf32_strlen2(char32_t* str, uint8_t* char_size)
{
    unsigned length = 0;
    char32_t width = 0;
    char32_t c;
    while ((c = *str++) != 0) {
        width |= c;
        length++;
    }
    *char_size = calc_char_size(width);
    return length;
}

int utf32_strcmp(char32_t* a, char32_t* b)
{
    if (a == b) {
        return 0;
    }
    for (;;) {
        char32_t ca = *a++;
        char32_t cb = *b++;
        if (ca < cb) {
            return -1;
        } else if (ca > cb) {
            return 1;
        } else if (ca == 0) {
            return 0;
        }
    }
}

int utf32_strcmp_utf8(char32_t* a, char8_t* b)
{
    for (;;) {
        char32_t ca = *a++;
        char32_t cb = _pw_decode_utf8_char(&b);
        if (ca < cb) {
            return -1;
        } else if (ca > cb) {
            return 1;
        } else if (ca == 0) {
            return 0;
        }
    }
}

char32_t* utf32_strchr(char32_t* str, char32_t chr)
{
    char32_t c;
    while ((c = *str) != 0) {
        if (c == chr) {
            return str;
        }
        str++;
    }
    return nullptr;
}

uint8_t utf32_char_size(char32_t* str, unsigned max_len)
{
    char32_t width = 0;
    while (max_len--) {
        char32_t c = *str++;
        if (c == 0) {
            break;
        }
        width |= c;
    }
    return calc_char_size(width);
}
