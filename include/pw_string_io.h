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

#define pw_create_string_io(str, result) _Generic((str), \
             char*: _pw_create_string_io_ascii,  \
          char8_t*: _pw_create_string_io_utf8,   \
         char32_t*: _pw_create_string_io_utf32,  \
        PwValuePtr: _pw_create_string_io         \
    )((str), (result))

[[nodiscard]] static inline bool _pw_create_string_io_ascii(char* str, PwValuePtr result)
{
    PwValue v = PwStringAscii(str);
    PwStringIOCtorArgs args = { .string = &v };
    return pw_create2(PwTypeId_StringIO, &args, result);
}

[[nodiscard]] static inline bool _pw_create_string_io_utf8(char8_t* str, PwValuePtr result)
{
    PwValue v = PwStringUtf8(str);
    PwStringIOCtorArgs args = { .string = &v };
    return pw_create2(PwTypeId_StringIO, &args, result);
}

[[nodiscard]] static inline bool _pw_create_string_io_utf32(char32_t* str, PwValuePtr result)
{
    PwValue v = PwStringUtf32(str);
    PwStringIOCtorArgs args = { .string = &v };
    return pw_create2(PwTypeId_StringIO, &args, result);
}

[[nodiscard]] static inline bool _pw_create_string_io(PwValuePtr str, PwValuePtr result)
{
    PwStringIOCtorArgs args = { .string = str };
    return pw_create2(PwTypeId_StringIO, &args, result);
}



#ifdef __cplusplus
}
#endif
