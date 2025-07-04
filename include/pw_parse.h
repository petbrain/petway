#pragma once

#include <pw.h>

#ifdef __cplusplus
extern "C" {
#endif

[[nodiscard]] bool _pw_parse_unsigned(PwValuePtr str, unsigned start_pos,
                                      unsigned* end_pos, unsigned radix, PwValuePtr result);
/*
 * Parse `str` from `start_pos` as unsigned integer value.
 *
 * Return value and if `end_pos` is not null, write where conversion has stopped.
 */

[[nodiscard]] bool _pw_parse_number(PwValuePtr str, unsigned start_pos,
                                    int sign, unsigned* end_pos, char32_t* allowed_terminators,
                                    PwValuePtr result);
/*
 * `start_pos` points to the sign or first digit;
 * `end_pos` (optional) receives position where conversion is stopped, regardless of result;
 * `allowed_terminators` (optional) contains characters at which conversion may stop without returning error.
 *
 * Normally conversion stops at space character or at end of line. If wrong character is encountered,
 * and it is not among allowed terminators, this function returns error.
 *
 * Return Signed if parsed number fits into its range.
 * Return Unsigned if parsed number is beyond positive range of Signed value.
 * Return Float if string contains float number.
 * Return status on error.
 */

[[nodiscard]] bool pw_parse_number(PwValuePtr str, PwValuePtr result);
/*
 * Convert string to number.
 * Simplified wrapper for _pw_parse_number.
 */

[[nodiscard]] bool _pw_parse_datetime(PwValuePtr str, unsigned start_pos, unsigned* end_pos,
                                      char32_t* allowed_terminators, PwValuePtr result);
/*
 * Parse `str` as date/time in mutual formats of ISO-8601 and RFC 3339
 * (see https://ijmacd.github.io/rfc3339-iso8601/).
 *
 * In addition, T or space separator between date and time and dashes in the date part are optional.
 */

[[nodiscard]] bool pw_parse_datetime(PwValuePtr str, PwValuePtr result);
/*
 * Convert string to date/time.
 * Simplified wrapper for _pw_parse_datetime.
 */

[[nodiscard]] bool _pw_parse_timestamp(PwValuePtr str, unsigned start_pos, unsigned* end_pos,
                                       char32_t* allowed_terminators, PwValuePtr result);
/*
 * Parse `str` as timestamp in the form seconds\[.frac\], up to nanosecond resolution.
 */

[[nodiscard]] bool pw_parse_timestamp(PwValuePtr str, PwValuePtr result);
/*
 * Convert string to timestamp.
 * Simplified wrapper for _pw_parse_timestamp.
 */

#ifdef __cplusplus
}
#endif
