#include "include/pw.h"
#include "src/pw_charptr_internal.h"
#include "src/pw_interfaces_internal.h"
#include "src/pw_string_internal.h"
#include "src/pw_struct_internal.h"

typedef struct {
    // line reader iterator data
    _PwValue line;
    _PwValue pushback;
    unsigned line_number;
    unsigned line_position;
} _PwStringIO;

#define get_data_ptr(value)  ((_PwStringIO*) _pw_get_data_ptr((value), PwTypeId_StringIO))

// forward declaration
[[nodiscard]] static bool read_line_inplace(PwValuePtr self, PwValuePtr line);

/****************************************************************
 * Constructors
 */

[[nodiscard]] bool _pw_create_string_io_u8(char8_t* str, PwValuePtr result)
{
    _PwValue v = PW_CHARPTR(str);
    PwStringIOCtorArgs args = { .string = &v };
    return pw_create2(PwTypeId_StringIO, &args, result);
}

[[nodiscard]] bool _pw_create_string_io_u32(char32_t* str, PwValuePtr result)
{
    _PwValue v = PW_CHAR32PTR(str);
    PwStringIOCtorArgs args = { .string = &v };
    return pw_create2(PwTypeId_StringIO, &args, result);
}

[[nodiscard]] bool _pw_create_string_io(PwValuePtr str, PwValuePtr result)
{
    PwStringIOCtorArgs args = { .string = str };
    return pw_create2(PwTypeId_StringIO, &args, result);
}

/****************************************************************
 * StringIO type
 */

PwTypeId PwTypeId_StringIO = 0;

static PwType stringio_type;

[[nodiscard]] static bool stringio_init(PwValuePtr self, void* ctor_args)
{
    PwStringIOCtorArgs* args = ctor_args;

    PwValue str = PW_NULL;
    if (args) {
        if (pw_is_charptr(args->string)) {
            if (!pw_charptr_to_string(args->string, &str)) {
                return false;
            }
        } else {
            pw_clone2(args->string, &str);
        }
    } else {
        str = PwString(0, {});
    }
    if (!pw_is_string(&str)) {
        pw_set_status(PwStatus(PW_ERROR_INCOMPATIBLE_TYPE));
        return false;
    }
    _PwStringIO* sio = get_data_ptr(self);
    pw_move(&str, &sio->line);
    sio->pushback = PwNull();
    return true;
}

static void stringio_fini(PwValuePtr self)
{
    _PwStringIO* sio = get_data_ptr(self);
    pw_destroy(&sio->line);
    pw_destroy(&sio->pushback);
}

static void stringio_hash(PwValuePtr self, PwHashContext* ctx)
{
    _pw_hash_uint64(ctx, self->type_id);

    _PwStringIO* sio = get_data_ptr(self);
    unsigned length = _pw_string_length(&sio->line);
    if (length) {
        get_str_methods(&sio->line)->hash(_pw_string_char_ptr(&sio->line, 0), length, ctx);
    }
}

static void stringio_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _PwStringIO* sio = get_data_ptr(self);

    _pw_dump_start(fp, self, first_indent);
    _pw_dump_struct_data(fp, self);
    _pw_string_dump_data(fp, &sio->line, next_indent);

    _pw_print_indent(fp, next_indent);
    fprintf(fp, "Current position: %u\n", sio->line_position);
    if (pw_is_null(&sio->pushback)) {
        fputs("Pushback: none\n", fp);
    }
    else if (!pw_is_string(&sio->pushback)) {
        fputs("WARNING: bad pushback:\n", fp);
        pw_dump(fp, &sio->pushback);
    } else {
        fputs("Pushback:\n", fp);
        _pw_string_dump_data(fp, &sio->pushback, next_indent);
    }
}

[[nodiscard]] static bool stringio_to_string(PwValuePtr self, PwValuePtr result)
{
    _PwStringIO* sio = get_data_ptr(self);
    pw_clone2(&sio->line, result);
    return true;
}

[[nodiscard]] static bool stringio_is_true(PwValuePtr self)
{
    _PwStringIO* sio = get_data_ptr(self);
    return pw_is_true(&sio->line);
}

[[nodiscard]] static bool stringio_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    _PwStringIO* sio_self  = get_data_ptr(self);
    _PwStringIO* sio_other = get_data_ptr(other);

    PwMethodEqual fn_cmp = pw_typeof(&sio_self->line)->equal_sametype;
    return fn_cmp(&sio_self->line, &sio_other->line);
}

[[nodiscard]] static bool stringio_equal(PwValuePtr self, PwValuePtr other)
{
    _PwStringIO* sio_self  = get_data_ptr(self);

    PwMethodEqual fn_cmp = pw_typeof(&sio_self->line)->equal;
    return fn_cmp(&sio_self->line, other);
}

/****************************************************************
 * LineReader interface
 */

[[nodiscard]] static bool start_read_lines(PwValuePtr self)
{
    _PwStringIO* sio = get_data_ptr(self);
    sio->line_position = 0;
    sio->line_number = 0;
    pw_destroy(&sio->pushback);
    return true;
}

[[nodiscard]] static bool read_line(PwValuePtr self, PwValuePtr result)
{
    pw_destroy(result);
    *result = PwString(0, {});
    return read_line_inplace(self, result);
}

[[nodiscard]] static bool read_line_inplace(PwValuePtr self, PwValuePtr line)
{
    _PwStringIO* sio = get_data_ptr(self);

    if (!pw_string_truncate(line, 0)) {
        return false;
    }
    if (pw_is_string(&sio->pushback)) {
        if (!pw_string_append(line, &sio->pushback)) {
            return false;
        }
        pw_destroy(&sio->pushback);
        sio->line_number++;
        return true;
    }
    if (!pw_string_index_valid(&sio->line, sio->line_position)) {
        pw_set_status(PwStatus(PW_ERROR_EOF));
        return false;
    }

    unsigned lf_pos;
    if (!pw_strchr(&sio->line, '\n', sio->line_position, &lf_pos)) {
        lf_pos = pw_strlen(&sio->line) - 1;
    }
    if (!pw_string_append_substring(line, &sio->line, sio->line_position, lf_pos + 1)) {
        return false;
    }
    sio->line_position = lf_pos + 1;
    sio->line_number++;
    return true;
}

[[nodiscard]] static bool unread_line(PwValuePtr self, PwValuePtr line)
{
    _PwStringIO* sio = get_data_ptr(self);

    if (pw_is_null(&sio->pushback)) {
        __pw_clone(line, &sio->pushback);  // puchback is already Null, so use __pw_clone here
        sio->line_number--;
        return true;
    } else {
        return false;
    }
}

static unsigned get_line_number(PwValuePtr self)
{
    return get_data_ptr(self)->line_number;
}

static void stop_read_lines(PwValuePtr self)
{
    _PwStringIO* sio = get_data_ptr(self);
    pw_destroy(&sio->pushback);
}

static PwInterface_LineReader line_reader_interface = {
    .start             = start_read_lines,
    .read_line         = read_line,
    .read_line_inplace = read_line_inplace,
    .get_line_number   = get_line_number,
    .unread_line       = unread_line,
    .stop              = stop_read_lines
};


/****************************************************************
 * Initialization
 */

[[ gnu::constructor ]]
static void init_stringio_type()
{
    if (PwTypeId_StringIO == 0) {

        PwTypeId_StringIO = pw_struct_subtype(
            &stringio_type, "StringIO", PwTypeId_Struct, _PwStringIO,
            PwInterfaceId_LineReader, &line_reader_interface
        );
        stringio_type.hash           = stringio_hash;
        stringio_type.dump           = stringio_dump;
        stringio_type.to_string      = stringio_to_string;
        stringio_type.is_true        = stringio_is_true;
        stringio_type.equal_sametype = stringio_equal_sametype;
        stringio_type.equal          = stringio_equal;
        stringio_type.init           = stringio_init;
        stringio_type.fini           = stringio_fini;
    }
}
