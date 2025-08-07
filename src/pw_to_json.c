#include <string.h>

#include "include/pw_to_json.h"
#include "include/pw_utf.h"

#include "src/string/pw_string_internal.h"

typedef bool (*AppendMethod)(PwValuePtr self, uint8_t* start_ptr, uint8_t* end_ptr, uint8_t char_size);

// forward declaration
[[nodiscard]] static bool value_to_json(PwValuePtr value, unsigned indent, unsigned depth,
                                        PwValuePtr result, AppendMethod meth_append);

static uint8_t _S_NULL[4]  = "null";
static uint8_t _S_TRUE[4]  = "true";
static uint8_t _S_FALSE[5] = "false";
static uint8_t _S_QUOTE[1] = "\"";
static uint8_t _S_COMMA[1] = ",";
static uint8_t _S_COLON[1] = ":";
static uint8_t _S_SPACE[1] = " ";

static uint8_t _S_B[2] = "\\b";
static uint8_t _S_F[2] = "\\f";
static uint8_t _S_N[2] = "\\n";
static uint8_t _S_R[2] = "\\r";
static uint8_t _S_T[2] = "\\t";

static uint8_t _S_OPEN_SQUARE[1]  = "[";
static uint8_t _S_CLOSE_SQUARE[1] = "]";
static uint8_t _S_OPEN_CURLY[1]   = "{";
static uint8_t _S_CLOSE_CURLY[1]  = "}";

#define END_PTR(s)  ((s) + sizeof(s))

#define APPEND(s)  \
    do {  \
        if (!meth_append(result, (s), END_PTR(s), 1)) {  \
            return false;  \
        }  \
    } while (false)

[[nodiscard]] static bool escape_string(PwValuePtr str, PwValuePtr result, AppendMethod meth_append)
/*
 * Escape only double quotes and characters with codes < 32
 */
{
    APPEND(_S_QUOTE);

    uint8_t* end_ptr;
    uint8_t* start_ptr = _pw_string_start_end(str, &end_ptr);
    uint8_t* ptr = start_ptr;
    uint8_t char_size = str->char_size;

    while (ptr < end_ptr) {
        char32_t c = _pw_get_char(ptr, char_size);
        if (c == '"'  || c == '\\') {
            if (ptr > start_ptr) {
                if (!meth_append(result, start_ptr, ptr, char_size)) { return false; }
            }
            start_ptr = ptr + char_size;

            uint8_t s[2] = {'\\'};
            s[1] = (uint8_t) c;
            APPEND(s);

        } else if (c < 32) {
            if (ptr > start_ptr) {
                if (!meth_append(result, start_ptr, ptr, char_size)) { return false; }
            }
            start_ptr = ptr + char_size;

            switch (c)  {
                case '\b': APPEND(_S_B); break;
                case '\f': APPEND(_S_F); break;
                case '\n': APPEND(_S_N); break;
                case '\r': APPEND(_S_R); break;
                case '\t': APPEND(_S_T); break;
                default: {
                    uint8_t s[5] = "\\0000";
                    s[3] += (c >> 4);
                    s[4] += (c & 15);
                    APPEND(s);
                }
            }
        }
        ptr += char_size;
    }
    if (ptr > start_ptr) {
        if (!meth_append(result, start_ptr, ptr, char_size)) { return false; }
    }
    APPEND(_S_QUOTE);
    return true;
}

[[nodiscard]] static bool array_to_json(PwValuePtr value, unsigned indent, unsigned depth,
                                        PwValuePtr result, AppendMethod meth_append)
{
    unsigned num_items = pw_array_length(value);

    APPEND(_S_OPEN_SQUARE);
    if (num_items == 0) {
        APPEND(_S_CLOSE_SQUARE);
        return true;
    }
    unsigned indent_width = indent * depth;
    uint8_t indent_str[indent_width + 1];
    indent_str[0] = '\n';
    memset(&indent_str[1], ' ', indent_width);

    bool multiline = indent && num_items > 1;
    for (unsigned i = 0; i < num_items; i++) {{
        if (i) {
            APPEND(_S_COMMA);
        }
        if (multiline) {
            APPEND(indent_str);
        }
        PwValue item = PW_NULL;
        if (!pw_array_item(value, i, &item)) {
            return false;
        }
        if (!value_to_json(&item, indent, depth + multiline, result, meth_append)) {
            return false;
        }
    }}
    if (multiline) {
        // dedent closing brace
        if (!meth_append(result, indent_str, indent_str + indent * (depth - 1) + 1, 1)) {
            return false;
        }
    }
    APPEND(_S_CLOSE_SQUARE);
    return true;
}

[[nodiscard]] static bool map_to_json(PwValuePtr value, unsigned indent, unsigned depth,
                                      PwValuePtr result, AppendMethod meth_append)
{
    unsigned num_items = pw_map_length(value);

    APPEND(_S_OPEN_CURLY);
    if (num_items == 0) {
        APPEND(_S_CLOSE_CURLY);
        return true;
    }
    unsigned indent_width = indent * depth;
    uint8_t indent_str[indent_width + 1];
    indent_str[0] = '\n';
    memset(&indent_str[1], ' ', indent_width);

    bool multiline = indent && num_items > 1;
    for (unsigned i = 0; i < num_items; i++) {{
        PwValue k = PW_NULL;
        PwValue v = PW_NULL;
        if (!pw_map_item(value, i, &k, &v)) {
            return false;
        }
        if (i) {
            APPEND(_S_COMMA);
        }
        if (multiline) {
            APPEND(indent_str);
        }
        if (!escape_string(&k, result, meth_append)) {
            return false;
        }
        APPEND(_S_COLON);
        if (indent) {
            APPEND(_S_SPACE);
        }
        if (!value_to_json(&v, indent, depth + multiline, result, meth_append)) {
            return false;
        }
    }}
    if (multiline) {
        // dedent closing brace
        if (!meth_append(result, indent_str, indent_str + indent * (depth - 1) + 1, 1)) {
            return false;
        }
    }
    APPEND(_S_CLOSE_CURLY);
    return true;
}

[[nodiscard]] static bool append_printf(PwValuePtr result, AppendMethod meth_append, char* format, ...)
{
    uint8_t buffer[512];
    va_list ap;
    va_start(ap);
    int n = vsnprintf((char*) buffer, sizeof(buffer), format, ap);
    va_end(ap);
    if (n < 0 || n >= (int) sizeof(buffer)) {
        pw_set_status(PwStatus(PW_ERROR));
        return false;
    }
    return meth_append(result, buffer, buffer + n, 1);
}

[[nodiscard]] static bool value_to_json(PwValuePtr value, unsigned indent, unsigned depth,
                                        PwValuePtr result, AppendMethod meth_append)
/*
 * Append serialized value to `result`.
 *
 * Return status.
 */
{
    if (pw_is_null(value)) {
        APPEND(_S_NULL);
        return true;\
    }
    if (pw_is_bool(value)) {
        if (value->bool_value) {
            APPEND(_S_TRUE);
        } else {
            APPEND(_S_FALSE);
        }
        return true;
    }
    if (pw_is_signed(value)) {
        PwValue s = PW_NULL;
        return append_printf(result, meth_append, "%zd", value->signed_value);
    }
    if (pw_is_unsigned(value)) {
        return append_printf(result, meth_append, "%zu", value->unsigned_value);
    }
    if (pw_is_float(value)) {
        return append_printf(result, meth_append, "%f", value->float_value);
    }
    if (pw_is_string(value)) {
        return escape_string(value, result, meth_append);
    }
    if (pw_is_array(value)) {
        return array_to_json(value, indent, depth, result, meth_append);
    }
    if (pw_is_map(value)) {
        return map_to_json(value, indent, depth, result, meth_append);
    }
    pw_set_status(PwStatus(PW_ERROR_INCOMPATIBLE_TYPE));
    return false;
}

[[nodiscard]] bool pw_to_json(PwValuePtr value, unsigned indent, PwValuePtr result)
{
    PwValue string = PW_NULL;
    PwValuePtr output;
    if (pw_is_null(result)) {
        // force allocator to allocate one page for string to have plenty of space for appending
        if (!pw_create_empty_string(sys_page_size - 2 * sizeof(_PwStringData), 1, &string)) {
            return false;
        }
        output = &string;
    } else {
        output = result;
    }

    AppendMethod meth_append = pw_interface(output->type_id, Append)->append_string_data;

    if (!value_to_json(value, indent, 1, output, meth_append)) {
        return false;
    }
    if (pw_is_null(result)) {
        // if resulting string is smaller than half of page size, reallocate
        if (string.length * string.char_size < (sys_page_size >> 1)) {
            return pw_deepcopy(&string, result);
        }
        pw_move(&string, result);
    }
    return true;
}
