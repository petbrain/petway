#include "include/pw.h"
#include "src/string/pw_string_internal.h"

static bool is_space_1(uint8_t* start_ptr, uint8_t* end_ptr)
{
    while (start_ptr < end_ptr) {
        uint8_t c = *start_ptr++;
        if (!isspace(c)) {
            return false;
        }
    }
    return true;
}

#define IS_SPACE(CHAR_TYPE, CHAR_SIZE)  \
    static bool is_space_##CHAR_SIZE(uint8_t* start_ptr, uint8_t* end_ptr)  \
    {  \
        while (start_ptr < end_ptr) {  \
            char32_t c = *(CHAR_TYPE*) start_ptr;  \
            if (!pw_isspace(c)) {  \
                return false;  \
            }  \
            start_ptr += CHAR_SIZE;  \
        }  \
        return true;  \
    }
IS_SPACE(uint16_t, 2)
IS_SPACE(char32_t, 4)

static bool is_space_3(uint8_t* start_ptr, uint8_t* end_ptr)
{
    while (start_ptr < end_ptr) {
        char32_t c = *start_ptr++;
        c |= (*start_ptr++) << 8;
        c |= (*start_ptr++) << 16;
        if (!pw_isspace(c)) {
            return false;
        }
    }
    return true;
}

typedef bool (*StrIsSpace)(uint8_t* start_ptr, uint8_t* end_ptr);

static StrIsSpace is_space_variants[5] = {
    nullptr,
    is_space_1,
    is_space_2,
    is_space_3,
    is_space_4
};

bool pw_string_isspace(PwValuePtr str)
{
    pw_assert_string(str);
    uint8_t* end_ptr;
    uint8_t* start_ptr = _pw_string_start_end(str, &end_ptr);
    if (_pw_unlikely(start_ptr == end_ptr)) {
        return false;
    }
    StrIsSpace fn_is_space = is_space_variants[str->char_size];
    return fn_is_space(start_ptr, end_ptr);
}
