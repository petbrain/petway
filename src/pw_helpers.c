#include <stdarg.h>
#include <string.h>

#include "include/pw.h"

PwResult _pw_get(PwValuePtr container, ...)
{
    PwValue obj = pw_clone(container);
    PwValue result = PwNull();
    va_list ap;
    va_start(ap);
    for (;;) {{
        char* key = va_arg(ap, char*);
        if (key == nullptr) {
            break;
        }
        pw_destroy(&result);

        // get RandomAccess interface
        _PwInterface* interface = _pw_lookup_interface(obj.type_id, PwInterfaceId_RandomAccess);
        if (!interface) {
            result = PwError(PW_ERROR_KEY_NOT_FOUND);
            break;
        }
        PwInterface_RandomAccess* methods = (PwInterface_RandomAccess*) interface->interface_methods;

        // get value by key
        PwValue k = PwCharPtr(key);
        result = methods->get_item(&obj, &k);
        if (pw_error(&result)) {
            break;
        }

        // go to the deeper object
        pw_destroy(&obj);
        obj = pw_clone(&result);
    }}
    va_end(ap);
    return pw_move(&result);
}

extern char** environ;

PwResult pw_read_environment()
{
    PwValue result = PwMap();
    for (char** env = environ;;) {{
        char* var = *env++;
        if (var == nullptr) {
            break;
        }
        char* separator = strchr(var, '=');
        if (!separator) {
            continue;
        }
        PwValue key = PwString();
        pw_expect_true( pw_string_append_substring(&key, var, 0, separator - var) );

        PwValue value = pw_create_string(separator + 1);
        pw_return_if_error(&value);

        pw_expect_ok( pw_map_update(&result, &key, &value) );
    }}
    return pw_move(&result);
}
