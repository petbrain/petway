#include <string.h>

#include "include/pw_to_json.h"

#include "src/pw_charptr_internal.h"
#include "src/pw_string_internal.h"

// forward declarations
static unsigned estimate_length(PwValuePtr value, unsigned indent, unsigned depth, uint8_t* max_char_size);
static bool value_to_json(PwValuePtr value, unsigned indent, unsigned depth, PwValuePtr result);


static unsigned estimate_escaped_length(PwValuePtr str, uint8_t* char_size)
/*
 * Estimate length of escaped string.
 * Only double quotes and characters with codes < 32 to escape.
 */
{
#   define INCREMENT_LENGTH  \
        if (c == '"'  || c == '\\') {  \
            length += 2;  \
        } else if (c < 32) {  \
            if (c == '\b' || c == '\f' || c == '\n' || c == '\r' || c == '\t') {  \
                length += 2;  \
            } else {  \
                length += 6;  \
            }  \
        } else {  \
            length++;  \
        }  \
        width = update_char_width(width, c);

    unsigned length = 0;
    uint8_t  width = 0;

    if (pw_is_charptr(str)) {
        switch (str->charptr_subtype) {
            case PW_CHARPTR: {
                char8_t* ptr = str->charptr;
                while(*ptr != 0) {
                    char32_t c = read_utf8_char(&ptr);
                    if (c != 0xFFFFFFFF) {
                        INCREMENT_LENGTH
                    }
                }
                break;
            }
            case PW_CHAR32PTR: {
                char32_t* ptr = str->char32ptr;
                for (;;) {
                    char32_t c = *ptr++;
                    if (c == 0) {
                        break;
                    }
                    INCREMENT_LENGTH
                }
                break;
            }
            default:
                _pw_panic_bad_charptr_subtype(str);
        }
    } else {
        pw_assert_string(str);

        unsigned n = _pw_string_length(str);
        uint8_t* ptr = _pw_string_start(str);
        uint8_t chr_sz = _pw_string_char_size(str);

        for (unsigned i = 0; i < n; i++) {
            char32_t c = _pw_get_char(ptr, chr_sz);
            INCREMENT_LENGTH
            ptr += chr_sz;
        }
    }
    *char_size = char_width_to_char_size(width);
    return length;

#   undef INCREMENT_LENGTH
}

static PwResult escape_string(PwValuePtr str)
/*
 * Escape only double quotes and characters with codes < 32
 */
{
#   define APPEND_ESCAPED_CHAR  \
        if (c == '"'  || c == '\\') {  \
            if (!pw_string_append(&result, '\\')) {  \
                return PwOOM();  \
            }  \
            if (!pw_string_append(&result, c)) {  \
                return PwOOM();  \
            }  \
        } else if (c < 32) {  \
            if (!pw_string_append(&result, '\\')) {  \
                return PwOOM();  \
            }  \
            bool append_ok = false;  \
            switch (c)  {  \
                case '\b': append_ok = pw_string_append(&result, 'b'); break;  \
                case '\f': append_ok = pw_string_append(&result, 'f'); break;  \
                case '\n': append_ok = pw_string_append(&result, 'n'); break;  \
                case '\r': append_ok = pw_string_append(&result, 'r'); break;  \
                case '\t': append_ok = pw_string_append(&result, 't'); break;  \
                default:  \
                    if (!pw_string_append(&result, '0')) {  \
                        return PwOOM();  \
                    }  \
                    if (!pw_string_append(&result, '0')) {  \
                        return PwOOM();  \
                    }  \
                    if (!pw_string_append(&result, (c >> 4) + '0')) {  \
                        return PwOOM();  \
                    }  \
                    append_ok = pw_string_append(&result, (c & 15) + '0');  \
            }  \
            if (!append_ok) {  \
                return PwOOM();  \
            }  \
        } else {  \
            if (!pw_string_append(&result, c)) {  \
                return PwOOM();  \
            }  \
        }

    uint8_t char_size;
    unsigned estimated_length = estimate_escaped_length(str, &char_size);

    PwValue result = pw_create_empty_string(estimated_length, char_size);
    pw_return_if_error(&result);

    if (pw_is_charptr(str)) {
        switch (str->charptr_subtype) {
            case PW_CHARPTR: {
                char8_t* ptr = str->charptr;
                while(*ptr != 0) {
                    char32_t c = read_utf8_char(&ptr);
                    if (c != 0xFFFFFFFF) {
                        APPEND_ESCAPED_CHAR
                    }
                }
                break;
            }
            case PW_CHAR32PTR: {
                char32_t* ptr = str->char32ptr;
                for (;;) {
                    char32_t c = *ptr++;
                    if (c == 0) {
                        break;
                    }
                    APPEND_ESCAPED_CHAR
                }
                break;
            }
            default:
                _pw_panic_bad_charptr_subtype(str);
        }
    } else {
        pw_assert_string(str);

        unsigned length = _pw_string_length(str);
        uint8_t* ptr = _pw_string_start(str);
        uint8_t char_size = _pw_string_char_size(str);

        for (unsigned i = 0; i < length; i++) {
            char32_t c = _pw_get_char(ptr, char_size);
            APPEND_ESCAPED_CHAR
            ptr += char_size;
        }
    }
    return pw_move(&result);

#   undef APPEND_ESCAPED_CHAR
}

static unsigned estimate_array_length(PwValuePtr value, unsigned indent, unsigned depth, uint8_t* max_char_size)
{
    unsigned num_items = pw_array_length(value);

    unsigned length = 2;  // braces
    if (num_items == 0) {
        return length;
    }
    bool multiline = indent && num_items > 1;
    if (multiline) {
        length++; // line break after opening brace
    }
    for (unsigned i = 0; i < num_items; i++) {{
        if (i) {
            length += /* comma */ 1;
        }
        if (multiline) {
            length += /* line break */ 1 + indent * depth;
        }
        PwValue item = pw_array_item(value, i);
        unsigned item_length = estimate_length(&item, indent, depth + multiline, max_char_size);
        if (item_length == 0) {
            return 0;
        }
        length += item_length;
    }}
    if (multiline) {
        length += /* line break: */ 1 + /* indent before closing brace */ indent * (depth - 1);
    }
    return length;
}

static bool array_to_json(PwValuePtr value, unsigned indent, unsigned depth, PwValuePtr result)
{
    unsigned num_items = pw_array_length(value);

    if (!pw_string_append(result, '[')) {
        return false;
    }
    if (num_items == 0) {
        return pw_string_append(result, ']');
    }
    unsigned indent_width = indent * depth;
    char indent_str[indent_width + 2];
    indent_str[0] = '\n';
    memset(&indent_str[1], ' ', indent_width);
    indent_str[indent_width + 1] = 0;

    bool multiline = indent && num_items > 1;
    for (unsigned i = 0; i < num_items; i++) {{
        if (i) {
            if (!pw_string_append(result, ',')) {
                return false;
            }
        }
        if (multiline) {
            if (!pw_string_append(result, indent_str)) {
                return false;
            }
        }
        PwValue item = pw_array_item(value, i);
        if (!value_to_json(&item, indent, depth + multiline, result)) {
            return false;
        }
    }}
    if (multiline) {
        indent_str[indent * (depth - 1) + 1] = 0;  // dedent closing brace
        if (!pw_string_append(result, indent_str)) {
            return false;
        }
    }
    return pw_string_append(result, ']');
}

static unsigned estimate_map_length(PwValuePtr value, unsigned indent, unsigned depth, uint8_t* max_char_size)
{
    unsigned num_items = pw_map_length(value);

    unsigned length = 2;  // braces
    if (num_items == 0) {
        return length;
    }
    bool multiline = indent && num_items > 1;
    if (multiline) {
        length++; // line break after opening brace
    }
    for (unsigned i = 0; i < num_items; i++) {{
        if (i) {
            length += /* comma */ 1;
        }
        if (multiline) {
            length += /* line break */ 1 + indent * depth;
        }
        PwValue k = PwNull();
        PwValue v = PwNull();
        pw_map_item(value, i, &k, &v);

        if (!pw_is_string(&k)) {
            // bad type
            return 0;
        }
        unsigned k_length = estimate_length(&k, indent, depth, max_char_size);
        if (k_length == 0) {
            return 0;
        }
        unsigned v_length = estimate_length(&v, indent, depth + multiline, max_char_size);
        if (v_length == 0) {
            return 0;
        }
        length += k_length + /* separator */ 1 + v_length;
        if (indent) {
            length++;  // extra space
        }
    }}
    if (multiline) {
        length += /* line break: */ 1 + /* indent before closing brace */ indent * (depth - 1);
    }
    return length;
}

static bool map_to_json(PwValuePtr value, unsigned indent, unsigned depth, PwValuePtr result)
{
    unsigned num_items = pw_map_length(value);

    if (!pw_string_append(result, '{')) {
        return false;
    }
    if (num_items == 0) {
        return pw_string_append(result, ']');
    }
    unsigned indent_width = indent * depth;
    char indent_str[indent_width + 2];
    indent_str[0] = '\n';
    memset(&indent_str[1], ' ', indent_width);
    indent_str[indent_width + 1] = 0;

    bool multiline = indent && num_items > 1;
    for (unsigned i = 0; i < num_items; i++) {{
        PwValue k = PwNull();
        PwValue v = PwNull();
        pw_map_item(value, i, &k, &v);

        if (i) {
            if (!pw_string_append(result, ',')) {
                return false;
            }
        }
        if (multiline) {
            if (!pw_string_append(result, indent_str)) {
                return false;
            }
        }
        if (!pw_string_append(result, '"')) {
            return false;
        }
        PwValue escaped = escape_string(&k);
        if (pw_error(&escaped)) {
            return false;
        }
        if (!pw_string_append(result, &escaped)) {
            return false;
        }
        if (!pw_string_append(result, "\":")) {
            return false;
        }
        if (indent) {
            if (!pw_string_append(result, ' ')) {
                return false;
            }
        }
        if (!value_to_json(&v, indent, depth + multiline, result)) {
            return false;
        }
    }}
    if (multiline) {
        indent_str[indent * (depth - 1) + 1] = 0;  // dedent closing brace
        if (!pw_string_append(result, indent_str)) {
            return false;
        }
    }
    return pw_string_append(result, '}');
}

static unsigned estimate_length(PwValuePtr value, unsigned indent, unsigned depth, uint8_t* max_char_size)
/*
 * Estimate length of JSON representation of result.
 * As a side effect detect incompatible values.
 *
 * Return estimated length on success or zero on error.
 */
{
    if (pw_is_null(value)) {
        return 4;

    } else if (pw_is_bool(value)) {
        return (value->bool_value)? 4 : 5;

    } else if (pw_is_int(value)) {
        return 20; // max unsigned: 18446744073709551615

    } else if (pw_is_float(value)) {
        return 16; // no idea how many to reserve, .f conversion may generate very long string

    } else if (pw_is_charptr(value) || pw_is_string(value)) {
        uint8_t char_size;
        unsigned length = estimate_escaped_length(value, &char_size);
        if (*max_char_size < char_size) {
            *max_char_size = char_size;
        }
        return length + /* quotes: */ 2;

    } else if (pw_is_array(value)) {
        return estimate_array_length(value, indent, depth, max_char_size);

    } else if (pw_is_map(value)) {
        return estimate_map_length(value, indent, depth, max_char_size);
    }
    // bad type
    return 0;
}

static bool value_to_json(PwValuePtr value, unsigned indent, unsigned depth, PwValuePtr result)
/*
 * Append serialized value to `result`.
 *
 * Return false if OOM.
 */
{
    if (pw_is_null(value)) {
        return pw_string_append(result, "null");
    }
    if (pw_is_bool(value)) {
        return pw_string_append(result, (value->bool_value)? "true" : "false");
    }
    if (pw_is_signed(value)) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%zd", value->signed_value);
        return pw_string_append(result, buf);
    }
    if (pw_is_unsigned(value)) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%zu", value->unsigned_value);
        return pw_string_append(result, buf);
    }
    if (pw_is_float(value)) {
        char buf[320];
        snprintf(buf, sizeof(buf), "%f", value->float_value);
        return pw_string_append(result, buf);
    }
    if (pw_is_charptr(value) || pw_is_string(value)) {
        PwValue escaped = escape_string(value);
        if (pw_error(&escaped)) {
            return false;
        }
        if (pw_string_append(result, '"')) {
            if (pw_string_append(result, &escaped)) {
                return pw_string_append(result, '"');
            }
        }
        return false;
    }
    if (pw_is_array(value)) {
        return array_to_json(value, indent, depth, result);
    }
    if (pw_is_map(value)) {
        return map_to_json(value, indent, depth, result);
    }
    pw_panic("Incompatible type for JSON");
}

PwResult pw_to_json(PwValuePtr value, unsigned indent)
{
    uint8_t max_char_size = 1;
    unsigned estimated_len = estimate_length(value, indent, 1, &max_char_size);
    if (estimated_len == 0) {
        return PwError(PW_ERROR_INCOMPATIBLE_TYPE);
    }

    PwValue result = pw_create_empty_string(estimated_len, max_char_size);
    pw_return_if_error(&result);

    if (!value_to_json(value, indent, 1, &result)) {
        return PwOOM();
    }
    return pw_move(&result);
}
