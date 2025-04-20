#pragma once

/*
 * StringIO provides LineReader interface.
 * It's a singleton iterator for self.
 */

#include <pw.h>

#ifdef __cplusplus
extern "C" {
#endif

extern PwTypeId PwTypeId_StringIO;

#define pw_is_stringio(value)      pw_is_subtype((value), PwTypeId_StringIO)
#define pw_assert_stringio(value)  pw_assert(pw_is_stringio(value))


/****************************************************************
 * Constructors
 */

typedef struct {
    PwValuePtr string;
} PwStringIOCtorArgs;

#define pw_create_string_io(str) _Generic((str), \
             char*: _pw_create_string_io_u8_wrapper,  \
          char8_t*: _pw_create_string_io_u8,          \
         char32_t*: _pw_create_string_io_u32,         \
        PwValuePtr: _pw_create_string_io              \
    )((str))

PwResult _pw_create_string_io_u8  (char8_t* str);
PwResult _pw_create_string_io_u32 (char32_t* str);
PwResult _pw_create_string_io     (PwValuePtr str);

static inline PwResult _pw_create_string_io_u8_wrapper(char* str)
{
    return _pw_create_string_io_u8((char8_t*) str);
}

#ifdef __cplusplus
}
#endif
