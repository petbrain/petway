#include "include/pw.h"
#include "src/string/pw_string_internal.h"

/*
 * Implementation of hash methods.
 *
 * Calculate hash of codepoints regardless of char size.
 */

#define HASH_IMPL(type_name)  \
{  \
    type_name* ptr = (type_name*) self_ptr;  \
    while (length--) {  \
        union {  \
            struct {  \
                char32_t a;  \
                char32_t b;  \
            };  \
            uint64_t i64;  \
        } data;  \
        \
        data.a = *ptr++;  \
        \
        if (0 == length--) {  \
            data.b = 0;  \
            _pw_hash_uint64(ctx, data.i64);  \
            break;  \
        }  \
        data.b = *ptr++;  \
        _pw_hash_uint64(ctx, data.i64);  \
    }  \
}

static void strhash_1(uint8_t* self_ptr, unsigned length, PwHashContext* ctx) HASH_IMPL(uint8_t)
static void strhash_2(uint8_t* self_ptr, unsigned length, PwHashContext* ctx) HASH_IMPL(uint16_t)
static void strhash_4(uint8_t* self_ptr, unsigned length, PwHashContext* ctx) HASH_IMPL(uint32_t)

static void strhash_3(uint8_t* self_ptr, unsigned length, PwHashContext* ctx)
{
    while (length--) {
        union {
            struct {
                char32_t a;
                char32_t b;
            };
            uint64_t i64;
        } data;

        data.a = *self_ptr++;
        data.a |= (*self_ptr++) << 8;
        data.a |= (*self_ptr++) << 16;
        if (0 == length--) {
            data.b = 0;
            _pw_hash_uint64(ctx, data.i64);
            break;
        }
        data.b = *self_ptr++;
        data.b |= (*self_ptr++) << 8;
        data.b |= (*self_ptr++) << 16;
        _pw_hash_uint64(ctx, data.i64);
    }
}

StrHash _pw_strhash_variants[5] = {
    nullptr,
    strhash_1,
    strhash_2,
    strhash_3,
    strhash_4
};
