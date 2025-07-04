#include "include/pw.h"
#include "src/pw_interfaces_internal.h"
#include "src/pw_iterator_internal.h"
#include "src/pw_struct_internal.h"

/****************************************************************
 * Iterator type
 */

[[nodiscard]] static bool iterator_init(PwValuePtr self, void* ctor_args)
{
    PwIteratorCtorArgs* args = ctor_args;
    _PwIterator* data = get_iterator_ptr(self);
    pw_clone2(args->iterable, &data->iterable);
    return true;
}

static void iterator_fini(PwValuePtr self)
{
    _PwIterator* data = get_iterator_ptr(self);
    pw_destroy(&data->iterable);
}

static void iterator_hash(PwValuePtr self, PwHashContext* ctx)
{
    pw_panic("Iterators do not support hashing");
}

[[nodiscard]] static bool iterator_deepcopy(PwValuePtr self, PwValuePtr result)
{
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
}

static void iterator_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _PwIterator* data = get_iterator_ptr(self);

    _pw_dump_start(fp, self, first_indent);
    _pw_dump_struct_data(fp, self);
    _pw_print_indent(fp, next_indent);
    fputs("Iterable:", fp);
    _pw_call_dump(fp, &data->iterable, next_indent, next_indent, nullptr);
}

[[nodiscard]] static bool iterator_to_string(PwValuePtr self, PwValuePtr result)
{
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
}

[[nodiscard]] static bool iterator_is_true(PwValuePtr self)
{
    return false;
}

[[nodiscard]] static bool iterator_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    return false;
}

[[nodiscard]] static bool iterator_equal(PwValuePtr self, PwValuePtr other)
{
    return false;
}

PwType _pw_iterator_type = {
    .id             = PwTypeId_Iterator,
    .ancestor_id    = PwTypeId_Struct,
    .name           = "Iterator",
    .allocator      = &default_allocator,

    .create         = _pw_struct_create,
    .destroy        = _pw_struct_destroy,
    .clone          = _pw_struct_clone,
    .hash           = iterator_hash,
    .deepcopy       = iterator_deepcopy,
    .dump           = iterator_dump,
    .to_string      = iterator_to_string,
    .is_true        = iterator_is_true,
    .equal_sametype = iterator_equal_sametype,
    .equal          = iterator_equal,

    .data_offset    = sizeof(_PwStructData),
    .data_size      = sizeof(_PwIterator),

    .init           = iterator_init,
    .fini           = iterator_fini
};

// make sure _PwStructData has correct padding
static_assert((sizeof(_PwStructData) & (alignof(_PwIterator) - 1)) == 0);
