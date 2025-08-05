#include "include/pw.h"
#include "src/string/pw_string_internal.h"


[[nodiscard]] bool _pw_strcat_va(PwValuePtr result, ...)
{
    va_list ap;
    va_start(ap);
    bool ret = _pw_strcat_ap(result, ap);
    va_end(ap);
    return ret;
}

[[nodiscard]] bool _pw_strcat_ap(PwValuePtr result, va_list ap)
{
    // count the number of args, check their types,
    // calculate total length and max char width
    unsigned result_len = 0;
    uint8_t max_char_size = 1;
    va_list temp_ap;
    va_copy(temp_ap, ap);
    for (unsigned arg_no = 0;;) {
        // arg is not auto-cleaned here because we don't consume it yet
        _PwValue arg = va_arg(temp_ap, _PwValue);
        arg_no++;
        if (pw_is_status(&arg)) {
            if (pw_is_va_end(&arg)) {
                break;
            }
            pw_set_status(pw_clone(&arg));
            va_end(temp_ap);
            _pw_destroy_args(ap);
            return false;
        }
        uint8_t char_size;
        if (pw_is_string(&arg)) {
            result_len += pw_strlen(&arg);
            char_size = arg.char_size;

        } else {
            // XXX to_string?
            pw_set_status(PwStatus(PW_ERROR_INCOMPATIBLE_TYPE),
                          "Bad argument %u type for pw_strcat: %u, %s",
                          arg_no, arg.type_id, pw_get_type_name(arg.type_id));
            va_end(temp_ap);
            _pw_destroy_args(ap);
            return false;
        }
        if (max_char_size < char_size) {
            max_char_size = char_size;
        }
    }
    va_end(temp_ap);

    // allocate resulting string
    if (!pw_create_empty_string(result_len, max_char_size, result)) {
        return false;
    }
    if (result_len == 0) {
        return true;
    }

    // concatenate
    for (;;) {{
        PwValue arg = va_arg(ap, _PwValue);
        if (pw_is_va_end(&arg)) {
            return true;
        }
        if (!_pw_string_append(result, &arg)) {
            _pw_destroy_args(ap);
            return false;
        }
    }}
}
