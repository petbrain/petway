#pragma once

#include <stdio.h>

#include <pw_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PW_SUCCESS                     0
#define PW_STATUS_VA_END               1  // used as a terminator for variadic arguments
#define PW_ERROR_ERRNO                 2
#define PW_ERROR_OOM                   3
#define PW_ERROR_NOT_IMPLEMENTED       4
#define PW_ERROR_INCOMPATIBLE_TYPE     5
#define PW_ERROR_EOF                   6
#define PW_ERROR_DATA_SIZE_TOO_BIG     7
#define PW_ERROR_INDEX_OUT_OF_RANGE    8
#define PW_ERROR_ITERATION_IN_PROGRESS 9
#define PW_ERROR_BAD_NUMBER           10
#define PW_ERROR_BAD_DATETIME         11
#define PW_ERROR_BAD_TIMESTAMP        12
#define PW_ERROR_NUMERIC_OVERFLOW     13

// array errors
#define PW_ERROR_EXTRACT_FROM_EMPTY_ARRAY  14

// map errors
#define PW_ERROR_KEY_NOT_FOUND        15

// File errors
#define PW_ERROR_FILE_ALREADY_OPENED  16
#define PW_ERROR_FD_ALREADY_SET       17
#define PW_ERROR_CANT_SET_FILENAME    18
#define PW_ERROR_FILE_CLOSED          19
#define PW_ERROR_NOT_REGULAR_FILE     20

// StringIO errors
#define PW_ERROR_UNREAD_FAILED        21

uint16_t pw_define_status(char* status);
/*
 * Define status in the global table.
 * Return status code or PW_ERROR_OOM
 *
 * This function should be called from the very beginning of main() function
 * or from constructors that are called before main().
 */

char* pw_status_str(uint16_t status_code);
/*
 * Get status string by status code.
 */

static inline bool pw_ok(PwValuePtr status)
{
    if (!status) {
        return false;
    }
    if (!pw_is_status(status)) {
        // any other type means okay
        return true;
    }
    return !status->is_error;
}

static inline bool pw_error(PwValuePtr status)
{
    return !pw_ok(status);
}

#define pw_return_if_error(value_ptr)  \
    do {  \
        if (pw_error(value_ptr)) {  \
            return pw_move(value_ptr);  \
        }  \
    } while (false)

#define pw_expect_ok( function_call )  \
    do {  \
        PwValue status = function_call;  \
        pw_return_if_error(&status);  \
    } while (false)


static inline bool pw_eof(PwValuePtr status)
{
    if (!status) {
        return false;
    }
    if (!pw_is_status(status)) {
        return false;
    }
    return status->status_code == PW_ERROR_EOF;
}

static inline bool pw_va_end(PwValuePtr status)
{
    if (!status) {
        return false;
    }
    if (!pw_is_status(status)) {
        return false;
    }
    return status->status_code == PW_STATUS_VA_END;
}

void _pw_set_status_location(PwValuePtr status, char* file_name, unsigned line_number);
void _pw_set_status_desc(PwValuePtr status, char* fmt, ...);
void _pw_set_status_desc_ap(PwValuePtr status, char* fmt, va_list ap);
/*
 * Set description for status.
 * If out of memory assign PW_ERROR_OOM to status.
 */

void pw_print_status(FILE* fp, PwValuePtr status);


#ifdef __cplusplus
}
#endif
