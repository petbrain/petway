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


/****************************************************************
 * Character width functions
 */

static inline uint8_t calc_char_size(char32_t c)
{
    if (c < 256) {
        return 1;
    } else if (c < 65536) {
        return 2;
    } else if (c < 16777216) {
        return 3;
    } else {
        return 4;
    }
}

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
