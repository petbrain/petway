#include "include/pw.h"

[[nodiscard]]
bool pw_parse_kvargs(int argc, char* argv[], PwValuePtr result)
{
    if (!pw_create_map(result)) {
        return false;
    }
    if (argc == 0) {
        return true;
    }

    // add argv[0] to the result
    PwValue zero = PwUnsigned(0);
    PwValue argv0 = PW_NULL;
    if (!pw_create_string((char8_t*) argv[0], &argv0)) {
        return false;
    }
    if (!pw_map_update(result, &zero, &argv0)) {
        return false;
    }

    for(int i = 1; i < argc; i++) {{

        // convert arg to PW string
        PwValue arg = PW_NULL;
        if (!_pw_create_string_utf8((char8_t*) argv[i], &arg)) {
            return false;
        }
        // split by =
        PwValue kv = PW_NULL;
        if (!pw_string_split_chr(&arg, '=', 1, &kv)) {
            return false;
        }
        PwValue key = PW_NULL;
        if (!pw_array_item(&kv, 0, &key)) {
            return false;
        }
        if (pw_array_length(&kv) == 1) {
            // `=` is missing, value is null
            PwValue null = PW_NULL;
            if (!pw_map_update(result, &key, &null)) {
                return false;
            }
        } else {
            // add key-value, overwriting previous one
            PwValue value = PW_NULL;
            if (!pw_array_item(&kv, 1, &value)) {
                return false;
            }
            if (!pw_map_update(result, &key, &value)) {
                return false;
            }
        }
    }}
    return true;
}
