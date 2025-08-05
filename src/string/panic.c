#include "include/pw.h"
#include "src/string/pw_string_internal.h"

[[noreturn]]
void _pw_panic_bad_char_size(uint8_t char_size)
{
    pw_panic("Bad char size: %u\n", char_size);
}
