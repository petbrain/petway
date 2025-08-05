#ifndef _GNU_SOURCE
//  for vasprintf
#   define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>

#include <libpussy/mmarray.h>

#include "include/pw.h"
#include "include/pw_utf.h"
#include "src/pw_struct_internal.h"

static char* basic_statuses[] = {
    [PW_SUCCESS]                        = "SUCCESS",
    [PW_STATUS_VA_END]                  = "VA_END",
    [PW_ERROR]                          = "ERROR",
    [PW_ERROR_ERRNO]                    = "ERRNO",
    [PW_ERROR_OOM]                      = "OOM",
    [PW_ERROR_NOT_IMPLEMENTED]          = "NOT IMPLEMENTED",
    [PW_ERROR_INCOMPATIBLE_TYPE]        = "INCOMPATIBLE_TYPE",
    [PW_ERROR_EOF]                      = "EOF",
    [PW_ERROR_TIMEOUT]                  = "TIMEOUT",
    [PW_ERROR_STRING_TOO_LONG]          = "PW_ERROR_STRING_TOO_LONG",
    [PW_ERROR_DATA_SIZE_TOO_BIG]        = "DATA_SIZE_TOO_BIG",
    [PW_ERROR_INDEX_OUT_OF_RANGE]       = "INDEX_OUT_OF_RANGE",
    [PW_ERROR_ITERATION_IN_PROGRESS]    = "ITERATION_IN_PROGRESS",
    [PW_ERROR_BAD_NUMBER]               = "BAD_NUMBER",
    [PW_ERROR_BAD_DATETIME]             = "BAD_DATETIME",
    [PW_ERROR_BAD_TIMESTAMP]            = "BAD_TIMESTAMP",
    [PW_ERROR_NUMERIC_OVERFLOW]         = "NUMERIC_OVERFLOW",
    [PW_ERROR_INCOMPLETE_UTF8]          = "INCOMPLETE_UTF8",
    [PW_ERROR_EXTRACT_FROM_EMPTY_ARRAY] = "EXTRACT_FROM_EMPTY_ARRAY",
    [PW_ERROR_KEY_NOT_FOUND]            = "KEY_NOT_FOUND",
    [PW_ERROR_FILE_ALREADY_OPENED]      = "FILE_ALREADY_OPENED",
    [PW_ERROR_FD_ALREADY_SET]           = "FD_ALREADY_SET",
    [PW_ERROR_CANT_SET_FILENAME]        = "CANT_SET_FILENAME",
    [PW_ERROR_FILE_CLOSED]              = "FILE_CLOSED",
    [PW_ERROR_NOT_REGULAR_FILE]         = "NOT_REGULAR_FILE",
    [PW_ERROR_UNBUFFERED_FILE]          = "UNBUFFERED_FILE",
    [PW_ERROR_WRITE]                    = "WRITE",
    [PW_ERROR_UNREAD_FAILED]            = "UNREAD_FAILED"
};

static char** statuses = nullptr;
static uint16_t num_statuses = 0;

[[ gnu::constructor ]]
static void init_statuses()
{
    if (statuses) {
        return;
    }
    num_statuses = PW_LENGTH(basic_statuses);
    statuses = mmarray_allocate(num_statuses, sizeof(char*));
    for(uint16_t i = 0; i < num_statuses; i++) {
        char* status = basic_statuses[i];
        if (!status) {
            fprintf(stderr, "Status %u is not defined\n", i);
            abort();
        }
        statuses[i] = status;
    }
}

uint16_t pw_define_status(char* status)
{
    // the order constructor are called is undefined, make sure statuses are initialized
    init_statuses();

    if (num_statuses == 65535) {
        fprintf(stderr, "Cannot define more statuses than %u\n", num_statuses);
        return PW_ERROR_OOM;
    }
    statuses = mmarray_append_item(statuses, &status);
    uint16_t status_code = num_statuses++;
    return status_code;
}

char* pw_status_str(uint16_t status_code)
{
    if (status_code < num_statuses) {
        return statuses[status_code];
    } else {
        static char unknown[] = "(unknown)";
        return unknown;
    }
}

void _pw_set_status_location(PwValuePtr status, char* file_name, unsigned line_number)
{
    if (status->has_status_data) {
        status->status_data->file_name = file_name;
        status->status_data->line_number = line_number;
    } else {
        status->file_name = file_name;
        status->line_number = line_number;
    }
}

void _pw_set_status_desc(PwValuePtr status, char* fmt, ...)
{
    va_list ap;
    va_start(ap);
    _pw_set_status_desc_ap(status, fmt, ap);
    va_end(ap);
}

void _pw_set_status_desc_ap(PwValuePtr status, char* fmt, va_list ap)
{
    pw_assert_status(status);
    if (!status->is_error) {
        // do not set description for PW_SUCCESS
        // such status must not have any allocated data because it is allowed
        // to be overwritten without calling pw_destroy on it, similar to null value
        return;
    }

    if (status->has_status_data) {
        pw_destroy(&status->status_data->description);
    } else {
        char* file_name = status->file_name;  // file_name will be overwritten with status_data, save it
        if (!_pw_struct_alloc(status, nullptr)) {
            return;
        }
        status->has_status_data = 1;
        status->status_data->file_name = file_name;
        status->status_data->line_number = status->line_number;
    }
    char* desc;
    if (vasprintf(&desc, fmt, ap) == -1) {
        return;
    }
    PwValue s = PW_NULL;
    if (pw_create_string(desc, &s)) {
        pw_move(&s, &status->status_data->description);
    }
    free(desc);
}

void pw_print_status(FILE* fp, PwValuePtr status)
{
    // XXX rewrite in error-free way

    PwValue desc = PW_NULL;
    if (!pw_typeof(status)->to_string(status, &desc)) {
        // XXX
        pw_dump(fp, &desc);
    } else {
        PW_CSTRING_LOCAL(desc_cstr, &desc);
        fputs(desc_cstr, fp);
        fputc('\n', fp);
    }
}

/****************************************************************
 * Basic interface methods
 */

[[nodiscard]] static bool status_create(PwTypeId type_id, void* ctor_args, PwValuePtr result)
{
    // XXX use ctor_args for initializer?

    pw_destroy(result);
    *result = PwStatus(PW_SUCCESS);
    result->type_id = type_id;
    return true;
}

static void status_destroy(PwValuePtr self)
{
    if (self->has_status_data) {
        _pw_struct_destroy(self);
    } else {
        self->type_id = PwTypeId_Null;
    }
}

static void status_clone(PwValuePtr self)
{
    if (self->has_status_data) {
        // call super method to increment refcount
        _pw_types[PwTypeId_Struct]->clone(self);
    }
}

static void status_hash(PwValuePtr self, PwHashContext* ctx)
{
    _pw_hash_uint64(ctx, self->type_id);
    _pw_hash_uint64(ctx, self->status_code);
    if (self->status_code == PW_ERROR_ERRNO) {
        _pw_hash_uint64(ctx, self->pw_errno);
    }
    // XXX do not hash description?
}

[[nodiscard]] static bool status_deepcopy(PwValuePtr self, PwValuePtr result)
{
    if (!self->has_status_data) {
        pw_move(self, result);
        return true;
    }
    pw_destroy(result);
    result->type_id = PW_SUCCESS;
    result->status_data = nullptr;

    if (!_pw_struct_alloc(result, nullptr)) {
        return false;
    }
    result->status_data->file_name = self->status_data->file_name;
    result->status_data->line_number = self->status_data->line_number;
    return pw_deepcopy(&self->status_data->description, &result->status_data->description);
}

[[nodiscard]] static bool status_to_string(PwValuePtr status, PwValuePtr result)
{
    if (!status) {
        pw_destroy(result);
        *result = PwString("(null)");
        return false;
    }
    if (!pw_is_status(status)) {
        pw_destroy(result);
        *result = PwString("(not status)");
        return false;
    }
    if (!status->is_error) {
        pw_destroy(result);
        *result = PwString("Success");
        return false;
    }
    char* status_str = pw_status_str(status->status_code);
    char* file_name;
    unsigned line_number;
    unsigned description_length = 0;
    uint8_t char_size = 1;

    if (status->has_status_data) {
        file_name = status->status_data->file_name;
        line_number = status->status_data->line_number;
        PwValuePtr desc = &status->status_data->description;
        if (pw_is_string(desc)) {
            description_length = pw_strlen(desc);
            char_size = desc->char_size;
        }
    } else {
        file_name = status->file_name;
        line_number = status->line_number;
    }

    unsigned errno_length = 0;
    static char errno_fmt[] = "; errno %d: %s";
    char* errno_str = "";
    if (status->status_code == PW_ERROR_ERRNO) {
        errno_str = strerror(status->pw_errno);
    }
    char errno_desc[strlen(errno_fmt) + 16 + strlen(errno_str)];
    if (status->status_code == PW_ERROR_ERRNO) {
        errno_length = snprintf(errno_desc, sizeof(errno_desc), errno_fmt, status->pw_errno, errno_str);
    } else {
        errno_desc[0] = 0;
    }

    static char fmt[] = "%s; %s:%u%s";
    char desc[sizeof(fmt) + 16 + strlen(status_str) + strlen(file_name) + errno_length];
    unsigned length = snprintf(desc, sizeof(desc), fmt, status_str, file_name, line_number, errno_desc);

    if (!pw_create_empty_string(length + description_length + 2, char_size, result)) {
        goto error;
    }
    if (!pw_string_append(result, desc, desc + length)) {
        goto error;
    }
    if (description_length) {
        if (!pw_string_append(result, "; ", nullptr)) {
            goto error;
        }
        if (!pw_string_append(result, &status->status_data->description)) {
            goto error;
        }
    }
    return true;

error:
    pw_destroy(result);
    *result = PwString("(error)");
    return false;
}

static void status_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);

    if (self->has_status_data) {
        _pw_dump_struct_data(fp, self);
    }
    PwValue desc = PW_NULL;
    if (!status_to_string(self, &desc)) {
        return;
    }
    PW_CSTRING_LOCAL(cdesc, &desc);
    fputc(' ', fp);
    fputs(cdesc, fp);
    fputc('\n', fp);
}

static bool status_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    if (self->status_code == PW_ERROR_ERRNO) {
        return other->status_code == PW_ERROR_ERRNO && self->pw_errno == other->pw_errno;
    }
    else {
        return self->status_code == other->status_code;
    }
}

static bool status_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        if (t == PwTypeId_Status) {
            return status_equal_sametype(self, other);
        }
        // check base type
        t = _pw_types[t]->ancestor_id;
        if (t == PwTypeId_Null) {
            return false;
        }
    }
}

static void status_fini(PwValuePtr self)
{
    if (self->has_status_data) {
        pw_destroy(&self->status_data->description);
    }
}

PwType _pw_status_type = {
    .id             = PwTypeId_Status,
    .ancestor_id    = PwTypeId_Struct,
    .name           = "Status",
    .allocator      = &default_allocator,
    .create         = status_create,
    .destroy        = status_destroy,
    .clone          = status_clone,
    .hash           = status_hash,
    .deepcopy       = status_deepcopy,
    .dump           = status_dump,
    .to_string      = status_to_string,
    .is_true        = _pw_struct_is_true,
    .equal_sametype = status_equal_sametype,
    .equal          = status_equal,

    .data_offset    = sizeof(_PwStructData),
    .data_size      = sizeof(_PwStatusData),

    .fini           = status_fini
};

// make sure _PwStructData has correct padding
static_assert((sizeof(_PwStructData) & (alignof(_PwStatusData) - 1)) == 0);
