/*
 * PW hash based on https://github.com/Nicoshev/rapidhash
 *
 * Rapidhash has the following advantages to be adopted:
 *   - Clean maintainable code.
 *   - Simple algorithm.
 *
 * Implementation differences:
 *   - Tail is padded with zeroes and the code is simplified.
 *   - Although data length is known for all PW values, it is excluded
 *     from hash calculations to make PW code simpler and less error prone.
 *     But it can be uncommented for testing against the original implementation.
 */

#include "include/pw_hash.h"

#include "src/rapidhash.h"

//#define TRACE(...)  printf(__VA_ARGS__)
#define TRACE(...)

struct _PwHashContext {
    uint64_t seed;
    uint64_t see1;
    uint64_t see2;
    uint64_t buffer[6];
    int buf_size;
};

void _pw_hash_init(PwHashContext* ctx)
{
    ctx->seed = RAPID_SEED ^ rapid_mix(RAPID_SEED ^ RAPID_SECRET_0, RAPID_SECRET_1) /* ^ len */;
    ctx->see1 = ctx->seed;
    ctx->see2 = ctx->seed;
    //ctx->len = len;
    ctx->buf_size = 0;
}

void _pw_hash_uint64(PwHashContext* ctx, uint64_t data)
{
    if (ctx->buf_size == 6) {
        ctx->buf_size = 0;

        ctx->seed = rapid_mix(ctx->buffer[0] ^ RAPID_SECRET_0, ctx->buffer[1] ^ ctx->seed);
        ctx->see1 = rapid_mix(ctx->buffer[2] ^ RAPID_SECRET_1, ctx->buffer[3] ^ ctx->see1);
        ctx->see2 = rapid_mix(ctx->buffer[4] ^ RAPID_SECRET_2, ctx->buffer[5] ^ ctx->see2);
        TRACE("PW round seed=%llx\n", (unsigned long long) ctx->seed);
    }
    ctx->buffer[ctx->buf_size++] = data;
}

void _pw_hash_buffer(PwHashContext* ctx, void* buffer, size_t length)
{
    if ( ! (((ptrdiff_t) buffer) & 7)) {
        // aligned
        uint64_t* data_ptr = (uint64_t*) buffer;
        while (length >= 8) {
            _pw_hash_uint64(ctx, *data_ptr++);
            length -= 8;
        }
        buffer = (void*) data_ptr;
    }
    // remainder or not aligned buffer
    uint8_t* data_ptr = (uint8_t*) buffer;
    while (length) {
        uint64_t v = 0;
        for (size_t i = 0; i < 8; i++) {
            v <<= 8;
            v += *data_ptr++;
            if (0 == --length) {
                break;
            }
        }
        _pw_hash_uint64(ctx, v);
    }
}

void _pw_hash_string(PwHashContext* ctx, char* str)
{
    for(;;) {
        uint64_t v = 0;
        for (size_t i = 0; i < 8; i++) {
            uint8_t c = *str++;
            if (c == 0) {
                _pw_hash_uint64(ctx, v);
                return;
            }
            v <<= 8;
            v += c;
        }
        _pw_hash_uint64(ctx, v);
    }
}

void _pw_hash_string32(PwHashContext* ctx, char32_t* str)
{
    for(;;) {
        uint64_t v = 0;
        for (size_t i = 0; i < 2; i++) {
            uint32_t c = *str++;
            if (c == 0) {
                _pw_hash_uint64(ctx, v);
                return;
            }
            v <<= 32;
            v += c;
        }
        _pw_hash_uint64(ctx, v);
    }
}

PwType_Hash _pw_hash_finish(PwHashContext* ctx)
{
    ctx->seed ^= ctx->see1 ^ ctx->see2;

    while (ctx->buf_size < 2) {
        TRACE("PAD %d\n", ctx->buf_size);
        ctx->buffer[ctx->buf_size++] = 0;
    }

    if (ctx->buf_size > 2) {
        ctx->seed = rapid_mix(ctx->buffer[0] ^ RAPID_SECRET_2, ctx->buffer[1] ^ ctx->seed ^ RAPID_SECRET_1);
        TRACE("PW %d seed=%llx\n", ctx->buf_size, (unsigned long long) ctx->seed);
        if (ctx->buf_size > 4) {
            ctx->seed = rapid_mix(ctx->buffer[2] ^ RAPID_SECRET_2, ctx->buffer[3] ^ ctx->seed);
            TRACE("PW %d seed=%llx\n", ctx->buf_size, (unsigned long long) ctx->seed);
        }
    }

    uint64_t a = ctx->buffer[ctx->buf_size - 2] ^ RAPID_SECRET_1;
    uint64_t b = ctx->buffer[ctx->buf_size - 1] ^ ctx->seed;
    TRACE("PW a=%llx b=%llx\n", (unsigned long long) a, (unsigned long long) b);
    rapid_mum(&a, &b);

    return rapid_mix(a ^ RAPID_SECRET_0 /* ^ ctx->len */, b ^ RAPID_SECRET_1);
}

PwType_Hash pw_hash(PwValuePtr value)
{
    PwHashContext ctx;
    _pw_hash_init(&ctx);
    _pw_call_hash(value, &ctx);
    return _pw_hash_finish(&ctx);
}
