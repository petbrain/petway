#include <libpussy/dump.h>

#include "include/pw.h"
#include "src/pw_alloc.h"
#include "src/string/pw_string_internal.h"

/****************************************************************
 * Basic interface methods
 */

[[nodiscard]] static bool string_create(PwTypeId type_id, void* ctor_args, PwValuePtr result)
{
    // XXX use ctor_args for initializer?

    pw_destroy(result);
    *result = PwString("");
    return true;
}

void _pw_string_destroy(PwValuePtr self)
{
    if (self->allocated) {
        if (0 == --self->string_data->refcount) {
            _pw_free(self->type_id, (void**) &self->string_data, _pw_allocated_string_data_size(self));
        }
    }
}

static void string_clone(PwValuePtr self)
{
    if (self->allocated) {
        self->string_data->refcount++;
    }
}

[[nodiscard]] static bool string_deepcopy(PwValuePtr self, PwValuePtr result)
{
    if (_pw_likely(!self->allocated)) {
        pw_destroy(result);
        *result = *self;
    } else {
        unsigned length = pw_strlen(self);
        uint8_t char_size = self->char_size;
        if (!_pw_make_empty_string(self->type_id, length, char_size, result)) {
            return false;
        }
        StrCopy fn_copy = _pw_strcopy_variants[char_size][char_size];
        uint8_t* src_end_ptr;
        uint8_t* src_start_ptr = _pw_string_start_end(self, &src_end_ptr);
        fn_copy(_pw_string_start(result), src_start_ptr, src_end_ptr);
        _pw_string_set_length(result, length);
    }
    return true;
}

static void string_hash(PwValuePtr self, PwHashContext* ctx)
{
    // mind maps: the hash should be the same for subtypes, that's why not using self->type_id here
    _pw_hash_uint64(ctx, PwTypeId_String);

    unsigned length;
    uint8_t* char_ptr = _pw_string_start_length(self, &length);
    if (length) {
        StrHash fn_hash = _pw_strhash_variants[self->char_size];
        fn_hash(char_ptr, length, ctx);
    }
}

static void string_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    _pw_string_dump_data(fp, self, next_indent);
}

void _pw_string_dump_data(FILE* fp, PwValuePtr str, int indent)
{
    if (str->embedded) {
        fprintf(fp, " embedded,");
    } else if (str->allocated) {
        fprintf(fp, " data=%p, refcount=%u, data size=%u, ptr=%p",
                (void*) str->string_data, str->string_data->refcount,
                _pw_allocated_string_data_size(str), (void*) _pw_string_start(str));
    } else {
        fprintf(fp, " static,");
    }

    unsigned capacity = _pw_string_capacity(str);
    unsigned length = pw_strlen(str);
    uint8_t char_size = str->char_size;

    fprintf(fp, " length=%u, capacity=%u, char size=%u\n", length, capacity, char_size);
    indent += 4;

    if (length) {
        // print first 80 characters
        _pw_print_indent(fp, indent);
        uint8_t* rem_addr;
        uint8_t* start = _pw_string_start_end(str, &rem_addr);
        uint8_t* ptr = start;
        for(unsigned i = 0; i < length; i++) {
            _pw_putchar32_utf8(fp, _pw_get_char(ptr, char_size));
            ptr += char_size;
            if (i == 80) {
                fprintf(fp, "...");
                break;
            }
        }
        fputc('\n', fp);

        dump_hex(fp, indent, start, length * char_size, (uint8_t*) (((ptrdiff_t) start) & 15), true, true);
        if (length < capacity) {
            _pw_print_indent(fp, indent);
            fputs("capacity remainder:\n", fp);
            dump_hex(fp, indent, rem_addr, (capacity - length) * char_size,
                     (uint8_t*) ((((ptrdiff_t) start) & 15) + length * char_size), true, true);
        }
    }
}

[[nodiscard]] static bool string_is_true(PwValuePtr self)
{
    return pw_strlen(self);
}

[[nodiscard]] static bool string_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    if (_pw_unlikely(self == other)) {
        return true;
    }

    unsigned self_length  = pw_strlen(self);
    unsigned other_length = pw_strlen(other);
    if (self_length != other_length) {
        return false;
    }
    if (_pw_unlikely(self_length == 0)) {
        return true;
    }
    uint8_t* other_end_ptr;
    uint8_t* other_start_ptr = _pw_string_start_end(other, &other_end_ptr);

    SubstrEq fn_substreq = _pw_substreq_variants[self->char_size][other->char_size];

    return fn_substreq(_pw_string_start(self), other_start_ptr, other_end_ptr);
}

[[nodiscard]] static bool string_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        if (t == PwTypeId_String) {
            return string_equal_sametype(self, other);
        }
        // check base type
        t = _pw_types[t]->ancestor_id;
        if (t == PwTypeId_Null) {
            return false;
        }
    }
}


/****************************************************************
 * Append interface
 */

static bool append_string_data(PwValuePtr self, uint8_t* start_ptr, uint8_t* end_ptr, uint8_t char_size)
{
    unsigned length = end_ptr - start_ptr;
    if (!length) {
        return true;
    }
    if (!_pw_expand_string(self, length, self->char_size)) {
        return false;
    }
    unsigned pos = _pw_string_inc_length(self, length);

    StrAppend fn_append = _pw_str_append_variants[self->char_size][char_size];
    return fn_append(self, pos, start_ptr, end_ptr);
}

static PwInterface_Append append_interface = {
    .append = _pw_string_append,
    .append_string_data = append_string_data
};

/****************************************************************
 * String type
 */

//static PwInterface_RandomAccess random_access_interface;


static _PwInterface string_interfaces[] = {
    {
        .interface_id      = PwInterfaceId_Append,
        .interface_methods = (void**) &append_interface
    }
    /*
    {
        .interface_id      = PwInterfaceId_RandomAccess,
        .interface_methods = (void**) &random_access_interface
    }
    */
};

PwType _pw_string_type = {
    .id             = PwTypeId_String,
    .ancestor_id    = PwTypeId_Null,  // no ancestor
    .name           = "String",
    .allocator      = &default_allocator,
    .create         = string_create,
    .destroy        = _pw_string_destroy,
    .clone          = string_clone,
    .hash           = string_hash,
    .deepcopy       = string_deepcopy,
    .dump           = string_dump,
    .to_string      = string_deepcopy,  // yes, simply make a copy
    .is_true        = string_is_true,
    .equal_sametype = string_equal_sametype,
    .equal          = string_equal,

    .num_interfaces = PW_LENGTH(string_interfaces),
    .interfaces     = string_interfaces
};
