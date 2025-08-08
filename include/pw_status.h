#pragma once

#include <stdio.h>

#include <pw_types.h>
#include <pw_task.h>

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
#define PW_ERROR_STRING_TOO_LONG        9
#define PW_ERROR_DATA_SIZE_TOO_BIG     10
#define PW_ERROR_INDEX_OUT_OF_RANGE    11
#define PW_ERROR_ITERATION_IN_PROGRESS 12
#define PW_ERROR_BAD_NUMBER            13
#define PW_ERROR_BAD_DATETIME          14
#define PW_ERROR_BAD_TIMESTAMP         15
#define PW_ERROR_NUMERIC_OVERFLOW      16
#define PW_ERROR_INCOMPLETE_UTF8       17

// array errors
#define PW_ERROR_EXTRACT_FROM_EMPTY_ARRAY  18

// map errors
#define PW_ERROR_KEY_NOT_FOUND        19

// File errors
#define PW_ERROR_FILE_ALREADY_OPENED  20
#define PW_ERROR_FD_ALREADY_SET       21
#define PW_ERROR_CANT_SET_FILENAME    22
#define PW_ERROR_FILE_CLOSED          23
#define PW_ERROR_NOT_REGULAR_FILE     24
#define PW_ERROR_UNBUFFERED_FILE      25
#define PW_ERROR_WRITE                26

// StringIO errors
#define PW_ERROR_UNREAD_FAILED        27


struct __PwStatusData {
    /*
     * This structure extends _PwStructData.
     */
    _PwStructData struct_data;

    unsigned line_number;
    char* file_name;
    _PwValue description;  // string
};
typedef struct __PwStatusData _PwStatusData;


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


static inline bool pw_is_error(PwValuePtr value)
{
    if (pw_is_status(value)) {
        return value->is_error;
    } else {
        return false;
    }
}

static inline bool pw_is_eof()
{
    if (!pw_is_status(&current_task->status)) {
        return false;
    }
    return current_task->status.status_code == PW_ERROR_EOF;
}

static inline bool pw_is_errno(int _errno)
{
    if (!pw_is_status(&current_task->status)) {
        return false;
    }
    return current_task->status.status_code == PW_ERROR_ERRNO
        && current_task->status.pw_errno == _errno;
}

static inline bool pw_is_timeout()
{
    if (!pw_is_status(&current_task->status)) {
        return false;
    }
    return current_task->status.status_code == PW_ERROR_TIMEOUT;
}

static inline bool pw_is_va_end(PwValuePtr status)
{
    if (!pw_is_status(status)) {
        return false;
    }
    return status->status_code == PW_STATUS_VA_END;
}


#define pw_set_status(_status, ...)  \
    do {  \
        pw_destroy(&current_task->status);  \
        current_task->status = _status;  \
        __VA_OPT__( _pw_set_status_desc(&current_task->status, __VA_ARGS__); )  \
    } while (false)


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
