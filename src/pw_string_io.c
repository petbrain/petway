#include "include/pw.h"
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
static PwResult read_line_inplace(PwValuePtr self, PwValuePtr line);

/****************************************************************
 * Constructors
 */

PwResult _pw_create_string_io_u8(char8_t* str)
{
    __PWDECL_CharPtr(v, str);
    PwStringIOCtorArgs args = { .string = &v };
    return pw_create2(PwTypeId_StringIO, &args);
}

PwResult _pw_create_string_io_u32(char32_t* str)
{
    __PWDECL_Char32Ptr(v, str);
    PwStringIOCtorArgs args = { .string = &v };
    return pw_create2(PwTypeId_StringIO, &args);
}

PwResult _pw_create_string_io(PwValuePtr str)
{
    PwStringIOCtorArgs args = { .string = str };
    return pw_create2(PwTypeId_StringIO, &args);
}

/****************************************************************
 * Basic interface methods
 */

static PwResult stringio_init(PwValuePtr self, void* ctor_args)
{
    PwStringIOCtorArgs* args = ctor_args;

    PwValue str = PwNull();
    if (args) {
        str = pw_clone(args->string);  // this converts CharPtr to string
    } else {
        str = PwString();
    }
    if (!pw_is_string(&str)) {
        return PwError(PW_ERROR_INCOMPATIBLE_TYPE);
    }
    _PwStringIO* sio = get_data_ptr(self);
    sio->line = pw_move(&str);
    sio->pushback = PwNull();
    return PwOK();
}

static void stringio_fini(PwValuePtr self)
{
    _PwStringIO* sio = get_data_ptr(self);
    pw_destroy(&sio->line);
    pw_destroy(&sio->pushback);
}

static PwResult stringio_deepcopy(PwValuePtr self)
{
    return PwError(PW_ERROR_NOT_IMPLEMENTED);
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

static PwResult stringio_to_string(PwValuePtr self)
{
    _PwStringIO* sio = get_data_ptr(self);
    return pw_clone(&sio->line);
}

static bool stringio_is_true(PwValuePtr self)
{
    _PwStringIO* sio = get_data_ptr(self);
    return pw_is_true(&sio->line);
}

static bool stringio_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    _PwStringIO* sio_self  = get_data_ptr(self);
    _PwStringIO* sio_other = get_data_ptr(other);

    PwMethodEqual fn_cmp = pw_typeof(&sio_self->line)->equal_sametype;
    return fn_cmp(&sio_self->line, &sio_other->line);
}

static bool stringio_equal(PwValuePtr self, PwValuePtr other)
{
    _PwStringIO* sio_self  = get_data_ptr(self);

    PwMethodEqual fn_cmp = pw_typeof(&sio_self->line)->equal;
    return fn_cmp(&sio_self->line, other);
}

/****************************************************************
 * LineReader interface methods
 */

static PwResult start_read_lines(PwValuePtr self)
{
    _PwStringIO* sio = get_data_ptr(self);
    sio->line_position = 0;
    sio->line_number = 0;
    pw_destroy(&sio->pushback);
    return PwOK();
}

static PwResult read_line(PwValuePtr self)
{
    PwValue result = PwString();
    pw_expect_ok( read_line_inplace(self, &result) );
    return pw_move(&result);
}

static PwResult read_line_inplace(PwValuePtr self, PwValuePtr line)
{
    _PwStringIO* sio = get_data_ptr(self);

    pw_string_truncate(line, 0);

    if (pw_is_string(&sio->pushback)) {
        if (!pw_string_append(line, &sio->pushback)) {
            return PwOOM();
        }
        pw_destroy(&sio->pushback);
        sio->line_number++;
        return PwOK();
    }

    if (!pw_string_index_valid(&sio->line, sio->line_position)) {
        return PwError(PW_ERROR_EOF);
    }

    unsigned lf_pos;
    if (!pw_strchr(&sio->line, '\n', sio->line_position, &lf_pos)) {
        lf_pos = pw_strlen(&sio->line) - 1;
    }
    pw_string_append_substring(line, &sio->line, sio->line_position, lf_pos + 1);
    sio->line_position = lf_pos + 1;
    sio->line_number++;
    return PwOK();
}

static bool unread_line(PwValuePtr self, PwValuePtr line)
{
    _PwStringIO* sio = get_data_ptr(self);

    if (pw_is_null(&sio->pushback)) {
        sio->pushback = pw_clone(line);
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

/****************************************************************
 * StringIO type and interfaces
 */

PwTypeId PwTypeId_StringIO = 0;

static PwInterface_LineReader line_reader_interface = {
    .start             = start_read_lines,
    .read_line         = read_line,
    .read_line_inplace = read_line_inplace,
    .get_line_number   = get_line_number,
    .unread_line       = unread_line,
    .stop              = stop_read_lines
};

static PwType stringio_type = {
    .id             = 0,
    .ancestor_id    = PwTypeId_Struct,
    .name           = "StringIO",
    .allocator      = &default_allocator,

    .create         = _pw_struct_create,
    .destroy        = _pw_struct_destroy,
    .clone          = _pw_struct_clone,
    .hash           = stringio_hash,
    .deepcopy       = stringio_deepcopy,
    .dump           = stringio_dump,
    .to_string      = stringio_to_string,
    .is_true        = stringio_is_true,
    .equal_sametype = stringio_equal_sametype,
    .equal          = stringio_equal,

    .data_offset    = sizeof(_PwStructData),
    .data_size      = sizeof(_PwStringIO),

    .init           = stringio_init,
    .fini           = stringio_fini
};

// make sure _PwStructData has correct padding
static_assert((sizeof(_PwStructData) & (alignof(_PwStringIO) - 1)) == 0);


[[ gnu::constructor ]]
static void init_stringio_type()
{
    PwTypeId_StringIO = pw_add_type(
        &stringio_type,
        PwInterfaceId_LineReader, &line_reader_interface
    );
}
