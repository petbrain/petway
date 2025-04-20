#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/pw_assert.h"

[[noreturn]]
void pw_panic(char* fmt, ...)
{
    va_list ap;
    va_start(ap);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    abort();
}
