#include <stdarg.h>

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
