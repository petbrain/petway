#pragma once

#include <stdio.h>

#include <pw_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PW_SUCCESS                      0
#define PW_STATUS_VA_END                1  // used as a terminator for variadic arguments
#define PW_ERROR                        2  // basic error for use as custom error in pw_status macro; description is recommended
#define PW_ERROR_ERRNO                  3
#define PW_ERROR_OOM                    4
#define PW_ERROR_NOT_IMPLEMENTED        5
#define PW_ERROR_INCOMPATIBLE_TYPE      6
#define PW_ERROR_EOF                    7
#define PW_ERROR_TIMEOUT                8
#define PW_ERROR_DATA_SIZE_TOO_BIG      9
#define PW_ERROR_INDEX_OUT_OF_RANGE    10
#define PW_ERROR_ITERATION_IN_PROGRESS 11
#define PW_ERROR_BAD_NUMBER            12
#define PW_ERROR_BAD_DATETIME          13
#define PW_ERROR_BAD_TIMESTAMP         14
#define PW_ERROR_NUMERIC_OVERFLOW      15
#define PW_ERROR_INCOMPLETE_UTF8       16

// array errors
#define PW_ERROR_EXTRACT_FROM_EMPTY_ARRAY  17

// map errors
#define PW_ERROR_KEY_NOT_FOUND        18

// File errors
#define PW_ERROR_FILE_ALREADY_OPENED  19
#define PW_ERROR_FD_ALREADY_SET       20
#define PW_ERROR_CANT_SET_FILENAME    21
#define PW_ERROR_FILE_CLOSED          22
#define PW_ERROR_NOT_REGULAR_FILE     23
#define PW_ERROR_UNBUFFERED_FILE      24
#define PW_ERROR_WRITE                25

// StringIO errors
#define PW_ERROR_UNREAD_FAILED        26

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

#define pw_status(status_code, ...)  \
    /* make status with optional description */ \
    ({  \
        __PWDECL_Status(status, (status_code));  \
        __VA_OPT__( _pw_set_status_desc(&status, __VA_ARGS__); )  \
        status;  \
    })

static inline bool pw_ok(PwValuePtr status)
{
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

#define pw_return_if_error(value_ptr, ...)  \
    /* emit return statement if value is an error; set description, if provided by variadic arguments */ \
    do {  \
        if (pw_error(value_ptr)) {  \
            __VA_OPT__( _pw_set_status_desc(value_ptr, __VA_ARGS__); )  \
            return pw_move(value_ptr);  \
        }  \
    } while (false)

#define pw_return_ok_or_oom( function_call )  \
    do {  \
        if (function_call) {  \
            return PwOK();  \
        } else {  \
            return PwOOM();  \
        }  \
    } while (false)

#define pw_expect_ok( function_call )  \
    do {  \
        PwValue __status = function_call;  \
        pw_return_if_error(&__status);  \
    } while (false)

#define pw_expect_true( function_call )  \
    do {  \
        if (!(function_call)) {  \
            return PwOOM();  \
        }  \
    } while (false)


static inline bool pw_eof(PwValuePtr status)
{
    if (!pw_is_status(status)) {
        return false;
    }
    return status->status_code == PW_ERROR_EOF;
}

static inline bool pw_errno(PwValuePtr status, int _errno)
{
    if (!pw_is_status(status)) {
        return false;
    }
    return status->status_code == PW_ERROR_ERRNO && status->pw_errno == _errno;
}

static inline bool pw_timeout(PwValuePtr status)
{
    if (!pw_is_status(status)) {
        return false;
    }
    return status->status_code == PW_ERROR_TIMEOUT;
}

static inline bool pw_va_end(PwValuePtr status)
{
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
