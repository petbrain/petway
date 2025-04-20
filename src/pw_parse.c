#include <errno.h>
#include <stdlib.h>

#include <include/pw_netutils.h>


static inline bool end_of_line(PwValuePtr str, unsigned position)
/*
 * Return true if position is beyond end of line.
 */
{
    return !pw_string_index_valid(str, position);
}

PwResult _pw_parse_unsigned(PwValuePtr str, unsigned start_pos, unsigned* end_pos, unsigned radix)
{
    PWDECL_Unsigned(result, 0);
    bool digit_seen = false;
    bool separator_seen = false;
    unsigned pos = start_pos;
    for(;;) {
        char32_t chr = pw_char_at(str, pos);

        // check separator
        if (chr == '\'' || chr == '_') {
            if (separator_seen) {
                // duplicate separator in the number
                result = PwError(PW_ERROR_BAD_NUMBER);
                break;
            }
            if (!digit_seen) {
                // eparator is not allowed in the beginning of number
                result = PwError(PW_ERROR_BAD_NUMBER);
                break;
            }
            separator_seen = true;
            pos++;
            if (end_of_line(str, pos)) {
                result = PwError(PW_ERROR_BAD_NUMBER);
                break;
            }
            continue;
        }
        separator_seen = false;

        // check digit and convert to number
        if (radix == 16) {
            if (chr >= 'a' && chr <= 'f') {
                chr -= 'a' - 10;
            } else if (chr >= 'A' && chr <= 'F') {
                chr -= 'A' - 10;
            } else if (chr >= '0' && chr <= '9') {
                chr -= '0';
            } else if (!digit_seen) {
                result = PwError(PW_ERROR_BAD_NUMBER);
                break;
            } else {
                // not a digit, end of conversion
                break;
            }
        } else if (chr >= '0' && chr < (char32_t) ('0' + radix)) {
            chr -= '0';
        } else if (!digit_seen) {
            result = PwError(PW_ERROR_BAD_NUMBER);
            break;
        } else {
            // not a digit, end of conversion
            break;
        }
        if (result.unsigned_value > PW_UNSIGNED_MAX / radix) {
            // overflow
            result = PwError(PW_ERROR_NUMERIC_OVERFLOW);
            break;
        }
        PwType_Unsigned new_value = result.unsigned_value * radix + chr;
        if (new_value < result.unsigned_value) {
            // overflow
            result = PwError(PW_ERROR_NUMERIC_OVERFLOW);
            break;
        }
        result.unsigned_value = new_value;

        pos++;
        if (end_of_line(str, pos)) {
            // end of line, end of conversion
            break;
        }
        digit_seen = true;
    }
    if (end_pos) {
        *end_pos = pos;
    }
    return result;
}

static unsigned skip_digits(PwValuePtr str, unsigned pos)
{
    for (;;) {
        if (end_of_line(str, pos)) {
            break;
        }
        char32_t chr = pw_char_at(str, pos);
        if (!('0' <= chr && chr <= '9')) {
            break;
        }
        pos++;
    }
    return pos;
}

PwResult _pw_parse_number(PwValuePtr str, unsigned start_pos, int sign, unsigned* end_pos, char32_t* allowed_terminators)
{
    unsigned pos = start_pos;
    unsigned radix = 10;
    bool is_float = false;
    PWDECL_Unsigned(base, 0);
    PWDECL_Signed(result, 0);

    char32_t chr = pw_char_at(str, pos);
    if (chr == '0') {
        // check radix specifier
        if (end_of_line(str, pos)) {
            goto done;
        }
        switch (pw_char_at(str, pos + 1)) {
            case 'b':
            case 'B':
                radix = 2;
                pos += 2;
                break;
            case 'o':
            case 'O':
                radix = 8;
                pos += 2;
                break;
            case 'x':
            case 'X':
                radix = 16;
                pos += 2;
                break;
            default:
                break;
        }
        if (end_of_line(str, pos)) {
            result = PwError(PW_ERROR_BAD_NUMBER);
            goto out;
        }
    }

    base = _pw_parse_unsigned(str, pos, &pos, radix);
    pw_return_if_error(&base);

    if (end_of_line(str, pos)) {
        goto done;
    }

    // check for fraction
    chr = pw_char_at(str, pos);
    if (chr == '.') {
        if (radix != 10) {
decimal_float_only:
            // only decimal representation is supported for floating point numbers
            result = PwError(PW_ERROR_BAD_NUMBER);
            goto out;
        }
        is_float = true;
        pos = skip_digits(str, pos + 1);
        if (end_of_line(str, pos)) {
            goto done;
        }
        chr = pw_char_at(str, pos);
    }
    // check for exponent
    if (chr == 'e' || chr == 'E') {
        if (radix != 10) {
            goto decimal_float_only;
        }
        is_float = true;
        pos++;
        if (end_of_line(str, pos)) {
            goto done;
        }
        chr = pw_char_at(str, pos);
        if (chr == '-' || chr == '+') {
            pos++;
        }
        unsigned next_pos = skip_digits(str, pos);
        if (next_pos == pos) {
            // bad exponent
            result = PwError(PW_ERROR_BAD_NUMBER);
            goto out;
        }
        pos = next_pos;

    } else if ( ! (pw_isspace(chr) || (allowed_terminators && u32_strchr(allowed_terminators, chr)))) {
        result = PwError(PW_ERROR_BAD_NUMBER);
        goto out;
    }

done:
    if (is_float) {
        // parse float
        unsigned len = pos - start_pos;
        char number[len + 1];
        pw_substr_to_utf8_buf(str, start_pos, pos, number);
        errno = 0;
        double n = strtod(number, nullptr);
        if (errno == ERANGE) {
            result = PwError(PW_ERROR_NUMERIC_OVERFLOW);
            goto out;
        } else if (errno) {
            // floating point conversion error
            result = PwError(PW_ERROR_BAD_NUMBER);
            goto out;
        }
        if (sign < 0 && n != 0.0) {
            n = -n;
        }
        result = PwFloat(n);
    } else {
        // make integer
        if (base.unsigned_value > PW_SIGNED_MAX) {
            if (sign < 0) {
                result = PwError(PW_ERROR_NUMERIC_OVERFLOW);
                goto out;
            } else {
                result = PwUnsigned(base.unsigned_value);
            }
        } else {
            if (sign < 0 && base.unsigned_value) {
                result = PwSigned(-base.unsigned_value);
            } else {
                result = PwSigned(base.unsigned_value);
            }
        }
    }
out:
    if (end_pos) {
        *end_pos = pos;
    }
    return pw_move(&result);
}

PwResult pw_parse_number(PwValuePtr str)
{
    int sign = 1;
    unsigned start_pos = pw_string_skip_spaces(str, 0);
    char32_t chr = pw_char_at(str, start_pos);
    if (chr == '+') {
        // no op
        start_pos++;
    } else if (chr == '-') {
        sign = -1;
        start_pos++;
    }
    return _pw_parse_number(str, start_pos, sign, nullptr, nullptr);
}

static bool parse_nanosecond_frac(PwValuePtr str, unsigned* pos, uint32_t* result)
/*
 * Parse fractional nanoseconds part in `str` starting from `pos`.
 * Always update `pos` upon return.
 * Return true on success and write parsed value to `result`.
 * On error return false.
 */
{
    unsigned p = *pos;
    uint32_t nanoseconds = 0;
    unsigned i = 0;
    while (!end_of_line(str, p)) {
        char32_t chr = pw_char_at(str, p);
        if (!pw_isdigit(chr)) {
            break;
        }
        if (i == 9) {
            *pos = p;
            return false;
        }
        nanoseconds *= 10;
        nanoseconds += chr - '0';
        i++;
        p++;
    }
    if (i == 0) {
    }
    static unsigned order[] = {
        1000'000'000,  // unused, i starts from 1 here
        100'000'000,
        10'000'000,
        1000'000,
        100'000,
        10'000,
        1000,
        100,
        10,
        1
    };
    *result = nanoseconds * order[i];
    *pos = p;
    return true;
}

PwResult _pw_parse_datetime(PwValuePtr str, unsigned start_pos, unsigned* end_pos, char32_t* allowed_terminators)
{
    PWDECL_DateTime(result);
    unsigned pos = start_pos;
    char32_t chr;

    // parse YYYY part
    for (unsigned i = 0; i < 4; i++, pos++) {
        chr = pw_char_at(str, pos);
        if (!pw_isdigit(chr)) { goto bad_datetime; }
        result.year *= 10;
        result.year += chr - '0';
    }
    // skip optional separator
    if (pw_char_at(str, pos) == '-') {
        pos++;
    }
    // parse MM part
    for (unsigned i = 0; i < 2; i++, pos++) {
        chr = pw_char_at(str, pos);
        if (!pw_isdigit(chr)) { goto bad_datetime; }
        result.month *= 10;
        result.month += chr - '0';
    }
    // skip optional separator
    if (pw_char_at(str, pos) == '-') {
        pos++;
    }
    // parse DD part
    for (unsigned i = 0; i < 2; i++, pos++) {
        chr = pw_char_at(str, pos);
        if (!pw_isdigit(chr)) { goto bad_datetime; }
        result.day *= 10;
        result.day += chr - '0';
    }
    // skip optional separator
    chr = pw_char_at(str, pos);
    if (chr == 'T') {
        pos++;
    } else {
        pos = pw_string_skip_spaces(str, pos);
        if (end_of_line(str, pos)) { goto out; }
        chr = pw_char_at(str, pos);
        if (allowed_terminators && u32_strchr(allowed_terminators, chr)) { goto out; }
    }
    // parse HH part
    for (unsigned i = 0; i < 2; i++, pos++) {
        chr = pw_char_at(str, pos);
        if (!pw_isdigit(chr)) { goto bad_datetime; }
        result.hour *= 10;
        result.hour += chr - '0';
    }
    // skip optional separator
    if (pw_char_at(str, pos) == ':') {
        pos++;
    }
    // parse MM part
    for (unsigned i = 0; i < 2; i++, pos++) {
        chr = pw_char_at(str, pos);
        if (!pw_isdigit(chr)) { goto bad_datetime; }
        result.minute *= 10;
        result.minute += chr - '0';
    }
    // skip optional separator
    if (pw_char_at(str, pos) == ':') {
        pos++;
    }
    // parse SS part
    for (unsigned i = 0; i < 2; i++, pos++) {
        chr = pw_char_at(str, pos);
        if (!pw_isdigit(chr)) { goto bad_datetime; }
        result.second *= 10;
        result.second += chr - '0';
    }
    // check optional parts
    chr = pw_char_at(str, pos);
    if (chr == 'Z') {
        pos++;
        goto end_of_datetime;
    }
    if ( chr == '.') {
        // parse nanoseconds
        pos++;
        if (!parse_nanosecond_frac(str, &pos, &result.nanosecond)) {
            goto bad_datetime;
        }
        chr = pw_char_at(str, pos);
    }
    if (chr == 'Z') {
        pos++;

    } else if (chr == '+' || chr == '-') {
        // parse GMT offset
        int sign = (chr == '-')? -1 : 1;
        pos++;
        // parse HH part
        unsigned offset_hour = 0;
        for (unsigned i = 0; i < 2; i++, pos++) {
            chr = pw_char_at(str, pos);
            if (!pw_isdigit(chr)) { goto bad_datetime; }
            offset_hour *= 10;
            offset_hour += chr - '0';
        }
        // skip optional separator
        if (pw_char_at(str, pos) == ':') {
            pos++;
        }
        // parse optional MM part
        unsigned offset_minute = 0;
        if (!end_of_line(str, pos)) {
            chr = pw_char_at(str, pos);
            if (pw_isdigit(chr)) {
                for (unsigned i = 0; i < 2; i++, pos++) {
                    chr = pw_char_at(str, pos);
                    if (!pw_isdigit(chr)) { goto bad_datetime; }
                    offset_minute *= 10;
                    offset_minute += chr - '0';
                }
            }
        }
        result.gmt_offset = sign * offset_hour * 60 + offset_minute;
    }

end_of_datetime:
    if (end_of_line(str, pos)) {
        goto out;
    }
    chr = pw_char_at(str, pos);
    if ( ! (pw_isspace(chr) || (allowed_terminators && u32_strchr(allowed_terminators, chr)))) {
        goto bad_datetime;
    }
    goto out;

bad_datetime:
    result = PwError(PW_ERROR_BAD_DATETIME);

out:
    if (end_pos) {
        *end_pos = pos;
    }
    return pw_move(&result);
}


PwResult pw_parse_datetime(PwValuePtr str)
{
    return _pw_parse_datetime(str, pw_string_skip_spaces(str, 0), nullptr, nullptr);
}


PwResult _pw_parse_timestamp(PwValuePtr str, unsigned start_pos, unsigned* end_pos, char32_t* allowed_terminators)
{
    PWDECL_Timestamp(result);

    unsigned pos;
    PwValue seconds = _pw_parse_unsigned(str, start_pos, &pos, 10);
    pw_return_if_error(&seconds);

    result.ts_seconds = seconds.unsigned_value;

    if (end_of_line(str, pos)) {
        goto out;
    }
    char32_t chr = pw_char_at(str, pos);
    if ( chr == '.') {
        // parse nanoseconds
        pos++;
        if (!parse_nanosecond_frac(str, &pos, &result.ts_nanoseconds)) {
            goto bad_timestamp;
        }
    }
    if (end_of_line(str, pos)) {
        goto out;
    }
    chr = pw_char_at(str, pos);
    if ( ! (pw_isspace(chr) || (allowed_terminators && u32_strchr(allowed_terminators, chr)))) {
        goto bad_timestamp;
    }
    goto out;

bad_timestamp:
    result = PwError(PW_ERROR_BAD_TIMESTAMP);

out:
    if (end_pos) {
        *end_pos = pos;
    }
    return pw_move(&result);
}


PwResult pw_parse_timestamp(PwValuePtr str)
{
    return _pw_parse_timestamp(str, pw_string_skip_spaces(str, 0), nullptr, nullptr);
}
