#include <stdarg.h>
#include <string.h>

#include "include/pw.h"

bool _pw_get(PwValuePtr result, PwValuePtr container, ...)
{
    PwValue obj = pw_clone(container);
    va_list ap;
    va_start(ap);
    for (;;) {{
        char8_t* key = va_arg(ap, char8_t*);
        if (key == nullptr) {
            va_end(ap);
            return true;
        }
        pw_destroy(result);

        // get RandomAccess interface
        _PwInterface* interface = _pw_lookup_interface(obj.type_id, PwInterfaceId_RandomAccess);
        if (!interface) {
            va_end(ap);
            pw_set_status(PwStatus(PW_ERROR_KEY_NOT_FOUND));
            return false;
        }
        PwInterface_RandomAccess* methods = (PwInterface_RandomAccess*) interface->interface_methods;

        // get value by key
        PwValue k = PW_NULL;
        if (!pw_create_string(key, &k)) {
            va_end(ap);
            return false;
        }
        if (!methods->get_item(&obj, &k, result)) {
            va_end(ap);
            return false;
        }

        // go to the deeper object
        pw_clone2(result, &obj);
    }}
}

bool _pw_set(PwValuePtr value, PwValuePtr container, ...)
{
    PwValue obj = pw_clone(container);
    va_list ap;
    va_start(ap);

    // get first key
    char8_t* key = va_arg(ap, char8_t*);
    if (key == nullptr) {
        va_end(ap);
        pw_set_status(PwStatus(PW_ERROR_KEY_NOT_FOUND));
        return false;
    }
    for (;;) {{
        // get next key
        char8_t* next_key = va_arg(ap, char8_t*);
        if (next_key == nullptr) {
            break;
        }
        // get RandomAccess interface
        _PwInterface* interface = _pw_lookup_interface(obj.type_id, PwInterfaceId_RandomAccess);
        if (!interface) {
            pw_set_status(PwStatus(PW_ERROR_KEY_NOT_FOUND));
            va_end(ap);
            return false;
        }
        PwInterface_RandomAccess* methods = (PwInterface_RandomAccess*) interface->interface_methods;

        // get nested object by key
        PwValue k = PW_NULL;
        if (!pw_create_string(key, &k)) {
            va_end(ap);
            return false;
        }
        PwValue nested_obj = PW_NULL;
        if (!methods->get_item(&obj, &k, &nested_obj)) {
            va_end(ap);
            return false;
        }
        // go to the deeper object
        pw_move(&nested_obj, &obj);
        key = next_key;
    }}
    va_end(ap);

    // set value
    _PwInterface* interface = _pw_lookup_interface(obj.type_id, PwInterfaceId_RandomAccess);
    if (!interface) {
        pw_set_status(PwStatus(PW_ERROR_KEY_NOT_FOUND));
        return false;
    }
    PwInterface_RandomAccess* methods = (PwInterface_RandomAccess*) interface->interface_methods;
    PwValue k = PW_NULL;
    if (!pw_create_string(key, &k)) {
        return false;
    }
    return methods->set_item(&obj, &k, value);
}

extern char** environ;

bool pw_read_environment(PwValuePtr result)
{
    if (!pw_create_map(result)) {
        return false;
    }
    for (char** env = environ;;) {{
        char8_t* var = (char8_t*) *env++;
        if (var == nullptr) {
            return true;
        }
        char8_t* separator = (char8_t*) strchr((char*) var, '=');
        if (!separator) {
            continue;
        }
        PwValue key = PW_STRING("");
        if (!pw_string_append(&key, var, separator)) {
            return false;
        }
        PwValue value = PW_NULL;
        if (!pw_create_string(separator + 1, &value)) {
            return false;
        }
        if (!pw_map_update(result, &key, &value)) {
            return false;
        }
    }}
}
