#include "include/pw.h"

PwResult pw_parse_kvargs(int argc, char* argv[])
{
    PwValue kwargs = pw_create(PwTypeId_Map);
    if (argc == 0) {
        return pw_move(&kwargs);
    }

    // add argv[0] to kwargs
    PwValue zero = PwUnsigned(0);
    PwValue argv0 = PwCharPtr((char8_t*) argv[0]);
    pw_expect_ok( pw_map_update(&kwargs, &zero, &argv0) );

    for(int i = 1; i < argc; i++) {{

        // convert arg to PW string
        PwValue arg = _pw_create_string_u8((char8_t*) argv[i]);
        pw_return_if_error(&arg);

        // split by =
        PwValue kv = pw_string_split_chr(&arg, '=', 1);
        pw_return_if_error(&kv);

        PwValue key = pw_array_item(&kv, 0);
        if (pw_array_length(&kv) == 1) {
            // `=` is missing, value is null
            PwValue null = PwNull();
            pw_expect_ok( pw_map_update(&kwargs, &key, &null) );
        } else {
            // add key-value, overwriting previous one
            PwValue value = pw_array_item(&kv, 1);
            pw_expect_ok( pw_map_update(&kwargs, &key, &value) );
        }
    }}
    return pw_move(&kwargs);
}
