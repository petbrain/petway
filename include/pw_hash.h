#pragma once

#include <pw_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 * Hash functions
 */

typedef uint64_t  PwType_Hash;

void _pw_hash_uint64(PwHashContext* ctx, uint64_t data);
void _pw_hash_buffer(PwHashContext* ctx, void* buffer, size_t length);
void _pw_hash_string(PwHashContext* ctx, char* str);
void _pw_hash_string32(PwHashContext* ctx, char32_t* str);

static inline void _pw_call_hash(PwValuePtr value, PwHashContext* ctx)
/*
 * Call hash method of value.
 */
{
    pw_typeof(value)->hash(value, ctx);
}

PwType_Hash pw_hash(PwValuePtr value);
/*
 * Calculate hash of value.
 */

#ifdef __cplusplus
}
#endif
