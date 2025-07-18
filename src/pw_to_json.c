#include <string.h>

#include "include/pw_to_json.h"
#include "include/pw_utf.h"

#include "src/pw_string_internal.h"

// forward declarations
static unsigned estimate_length(PwValuePtr value, unsigned indent, unsigned depth, uint8_t* max_char_size);
[[nodiscard]] static bool value_to_json(PwValuePtr value, unsigned indent, unsigned depth, PwValuePtr result);
[[nodiscard]] static bool value_to_json_file(PwValuePtr value, unsigned indent, unsigned depth, PwValuePtr file);


static unsigned estimate_escaped_length(PwValuePtr str, uint8_t* char_size)
/*
 * Estimate length of escaped string.
 * Only double quotes and characters with codes < 32 to escape.
 */
{
    pw_assert_string(str);

    unsigned length = 0;
    char32_t width = 0;

    unsigned n = pw_strlen(str);
    uint8_t* ptr = _pw_string_start(str);
    uint8_t chr_sz = str->char_size;

    for (unsigned i = 0; i < n; i++) {
        char32_t c = _pw_get_char(ptr, chr_sz);
        if (c == '"'  || c == '\\') {
            length += 2;
        } else if (c < 32) {
            if (c == '\b' || c == '\f' || c == '\n' || c == '\r' || c == '\t') {
                length += 2;
            } else {
                length += 6;
            }
        } else {
            length++;
        }
        width |= c;
        ptr += chr_sz;
    }
    *char_size = calc_char_size(width);
    return length;
}

[[nodiscard]] static bool escape_string(PwValuePtr str, PwValuePtr result)
/*
 * Escape only double quotes and characters with codes < 32
 */
{
    uint8_t char_size;
    unsigned estimated_length = estimate_escaped_length(str, &char_size);

    if (!pw_create_empty_string(estimated_length, char_size, result)) {
        return false;
    }

    unsigned length = pw_strlen(str);
    uint8_t* ptr = _pw_string_start(str);

    for (unsigned i = 0; i < length; i++) {
        char32_t c = _pw_get_char(ptr, char_size);
        if (c == '"'  || c == '\\') {
            if (!pw_string_append(result, '\\')) { return false; }
            if (!pw_string_append(result, c)) { return false; }
        } else if (c < 32) {
            if (!pw_string_append(result, '\\')) { return false; }
            switch (c)  {
                case '\b': if (!pw_string_append(result, 'b')) { return false; } break;
                case '\f': if (!pw_string_append(result, 'f')) { return false; } break;
                case '\n': if (!pw_string_append(result, 'n')) { return false; } break;
                case '\r': if (!pw_string_append(result, 'r')) { return false; } break;
                case '\t': if (!pw_string_append(result, 't')) { return false; } break;
                default:
                    if (!pw_string_append(result, '0')) { return false; }
                    if (!pw_string_append(result, '0')) { return false; }
                    if (!pw_string_append(result, (c >> 4) + '0')) { return false; }
                    if (!pw_string_append(result, (c & 15) + '0')) { return false; }
            }
        } else {
            if (!pw_string_append(result, c)) { return false; }
        }
        ptr += char_size;
    }
    return true;
}

[[nodiscard]] static bool write_string_to_file(PwValuePtr file, PwValuePtr str)
{
    PwValue escaped = PW_NULL;
    if (!escape_string(str, &escaped)) {
        return false;
    }
    unsigned n = pw_strlen_in_utf8(&escaped);
    char s[n + 2];
    s[0] = '"';
    pw_string_to_utf8_buf(&escaped, &s[1]);
    s[n + 1] = '"';
    return pw_write_exact(file, s, n + 2);
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
        PwValue item = PW_NULL;
        if (!pw_array_item(value, i, &item)) {
            return false;
        }
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

[[nodiscard]] static bool array_to_json(PwValuePtr value, unsigned indent, unsigned depth, PwValuePtr result)
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
        PwValue item = PW_NULL;
        if (!pw_array_item(value, i, &item)) {
            return false;
        }
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

[[nodiscard]] static bool array_to_json_file(PwValuePtr value, unsigned indent, unsigned depth, PwValuePtr file)
{
    unsigned num_items = pw_array_length(value);

    if (!pw_write_exact(file, "[", 1)) {
        return false;
    }
    if (num_items == 0) {
        return pw_write_exact(file, "]",1);
    }
    unsigned indent_width = indent * depth;
    char indent_str[indent_width + 2];
    indent_str[0] = '\n';
    memset(&indent_str[1], ' ', indent_width);
    indent_str[indent_width + 1] = 0;

    bool multiline = indent && num_items > 1;
    for (unsigned i = 0; i < num_items; i++) {{
        if (i) {
            if (!pw_write_exact(file, ",", 1)) {
                return false;
            }
        }
        if (multiline) {
            if (!pw_write_exact(file, indent_str, indent_width + 1)) {
                return false;
            }
        }
        PwValue item = PW_NULL;
        if (!pw_array_item(value, i, &item)) {
            return false;
        }
        if (!value_to_json_file(&item, indent, depth + multiline, file)) {
            return false;
        }
    }}
    if (multiline) {
        indent_str[indent * (depth - 1) + 1] = 0;  // dedent closing brace
        if (!pw_write_exact(file, indent_str, indent * (depth - 1) + 1)) {
            return false;
        }
    }
    return pw_write_exact(file, "]", 1);
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
        PwValue k = PW_NULL;
        PwValue v = PW_NULL;
        if (!pw_map_item(value, i, &k, &v)) {
            return 0;
        }
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

[[nodiscard]] static bool map_to_json(PwValuePtr value, unsigned indent, unsigned depth, PwValuePtr result)
{
    unsigned num_items = pw_map_length(value);

    if (!pw_string_append(result, '{')) {
        return false;
    }
    if (num_items == 0) {
        return pw_string_append(result, '}');
    }
    unsigned indent_width = indent * depth;
    char indent_str[indent_width + 2];
    indent_str[0] = '\n';
    memset(&indent_str[1], ' ', indent_width);
    indent_str[indent_width + 1] = 0;

    bool multiline = indent && num_items > 1;
    for (unsigned i = 0; i < num_items; i++) {{
        PwValue k = PW_NULL;
        PwValue v = PW_NULL;
        if (!pw_map_item(value, i, &k, &v)) {
            return false;
        }
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
        PwValue escaped = PW_NULL;
        if (!escape_string(&k, &escaped)) {
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

[[nodiscard]] static bool map_to_json_file(PwValuePtr value, unsigned indent, unsigned depth, PwValuePtr file)
{
    unsigned num_items = pw_map_length(value);

    if (!pw_write_exact(file, "{", 1)) {
        return false;
    }
    if (num_items == 0) {
        return pw_write_exact(file, "}", 1);
    }
    unsigned indent_width = indent * depth;
    char indent_str[indent_width + 2];
    indent_str[0] = '\n';
    memset(&indent_str[1], ' ', indent_width);
    indent_str[indent_width + 1] = 0;

    bool multiline = indent && num_items > 1;
    for (unsigned i = 0; i < num_items; i++) {{
        PwValue k = PW_NULL;
        PwValue v = PW_NULL;
        if (!pw_map_item(value, i, &k, &v)) {
            return false;
        }

        if (i) {
            if (!pw_write_exact(file, ",", 1)) {
                return false;
            }
        }
        if (multiline) {
            if (!pw_write_exact(file, indent_str, indent_width + 1)) {
                return false;
            }
        }
        if (!write_string_to_file(file, &k)) {
            return false;
        }
        if (!pw_write_exact(file, ":", 1)) {
            return false;
        }
        if (indent) {
            if (!pw_write_exact(file, " ", 1)) {
                return false;
            }
        }
        if (!value_to_json_file(&v, indent, depth + multiline, file)) {
            return false;
        }
    }}
    if (multiline) {
        indent_str[indent * (depth - 1) + 1] = 0;  // dedent closing brace
        if (!pw_write_exact(file, indent_str, indent * (depth - 1) + 1)) {
            return false;
        }
    }
    return pw_write_exact(file, "}", 1);
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

    } else if (pw_is_string(value)) {
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

[[nodiscard]] static bool value_to_json(PwValuePtr value, unsigned indent, unsigned depth, PwValuePtr result)
/*
 * Append serialized value to `result`.
 *
 * Return status.
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
        int n = snprintf(buf, sizeof(buf), "%zd", value->signed_value);
        if (n < 0) {
            pw_set_status(PwStatus(PW_ERROR));
            return false;
        }
        return pw_string_append(result, buf);
    }
    if (pw_is_unsigned(value)) {
        char buf[24];
        int n = snprintf(buf, sizeof(buf), "%zu", value->unsigned_value);
        if (n < 0) {
            pw_set_status(PwStatus(PW_ERROR));
            return false;
        }
        return pw_string_append(result, buf);
    }
    if (pw_is_float(value)) {
        char buf[320];
        int n = snprintf(buf, sizeof(buf), "%f", value->float_value);
        if (n < 0) {
            pw_set_status(PwStatus(PW_ERROR));
            return false;
        }
        return pw_string_append(result, buf);
    }
    if (pw_is_string(value)) {
        PwValue escaped = PW_NULL;
        if (!escape_string(value, &escaped)) {
            return false;
        }
        if (!pw_string_append(result, '"')) {
            return false;
        }
        if (!pw_string_append(result, &escaped)) {
            return false;
        }
        if (!pw_string_append(result, '"')) {
            return false;
        }
        return true;
    }
    if (pw_is_array(value)) {
        return array_to_json(value, indent, depth, result);
    }
    if (pw_is_map(value)) {
        return map_to_json(value, indent, depth, result);
    }
    pw_set_status(PwStatus(PW_ERROR_INCOMPATIBLE_TYPE));
    return false;
}

[[nodiscard]] static bool value_to_json_file(PwValuePtr value, unsigned indent, unsigned depth, PwValuePtr file)
/*
 * Write serialized value to `file`.
 *
 * Return status.
 */
{
    if (pw_is_null(value)) {
        return pw_write_exact(file, "null", 4);
    }
    if (pw_is_bool(value)) {
        if (value->bool_value) {
            return pw_write_exact(file, "true", 4);
        } else {
            return pw_write_exact(file, "false", 5);
        }
    }
    if (pw_is_signed(value)) {
        char buf[24];
        int n = snprintf(buf, sizeof(buf), "%zd", value->signed_value);
        if (n < 0) {
            pw_set_status(PwStatus(PW_ERROR));
            return false;
        }
        return pw_write_exact(file, buf, n);
    }
    if (pw_is_unsigned(value)) {
        char buf[24];
        int n = snprintf(buf, sizeof(buf), "%zu", value->unsigned_value);
        if (n < 0) {
            pw_set_status(PwStatus(PW_ERROR));
            return false;
        }
        return pw_write_exact(file, buf, n);
    }
    if (pw_is_float(value)) {
        char buf[320];
        int n = snprintf(buf, sizeof(buf), "%f", value->float_value);
        if (n < 0) {
            pw_set_status(PwStatus(PW_ERROR));
            return false;
        }
        return pw_write_exact(file, buf, n);
    }
    if (pw_is_string(value)) {
        return write_string_to_file(file, value);
    }
    if (pw_is_array(value)) {
        return array_to_json_file(value, indent, depth, file);
    }
    if (pw_is_map(value)) {
        return map_to_json_file(value, indent, depth, file);
    }
    pw_set_status(PwStatus(PW_ERROR_INCOMPATIBLE_TYPE));
    return false;
}

[[nodiscard]] bool pw_to_json(PwValuePtr value, unsigned indent, PwValuePtr result)
{
    uint8_t max_char_size = 1;
    unsigned estimated_len = estimate_length(value, indent, 1, &max_char_size);
    if (estimated_len == 0) {
        pw_set_status(PwStatus(PW_ERROR_INCOMPATIBLE_TYPE));
        return false;
    }
    if (!pw_create_empty_string(estimated_len, max_char_size, result)) {
        return false;
    }
    return value_to_json(value, indent, 1, result);
}

[[nodiscard]] bool pw_to_json_file(PwValuePtr value, unsigned indent, PwValuePtr file)
{
    pw_expect(File, file);
    if (!value_to_json_file(value, indent, 1, file)) {
        return false;
    }
    return pw_file_flush(file);
}
