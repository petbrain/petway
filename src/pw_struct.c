#include "src/pw_alloc.h"
#include "src/pw_struct_internal.h"

[[nodiscard]] static bool call_init(PwValuePtr self, void* ctor_args, PwTypeId type_id)
{
    if (type_id == PwTypeId_Struct) {
        // reached the bottom
        return true;
    }

    PwType* type = _pw_types[type_id];
    PwTypeId ancestor_id = type->ancestor_id;

    if (!call_init(self, ctor_args, ancestor_id)) {
        return false;
    }
    // initialized all ancestors, okay to call init for this type_id
    PwMethodInit init = type->init;
    if (init) {
        if (!init(self, ctor_args)) {
            // ancestor init failed, call fini for already called init
            while (type->id != PwTypeId_Struct) {
                PwMethodFini fini = type->fini;
                if (fini) {
                    fini(self);
                }
                type = pw_ancestor_of(type->id);
            }
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool _pw_struct_alloc(PwValuePtr self, void* ctor_args)
{
    PwType* type = pw_typeof(self);

    // allocate memory

    unsigned memsize = type->data_offset + type->data_size;
    self->struct_data = _pw_alloc(self->type_id, memsize, true);
    if (!self->struct_data) {
        return false;
    }

    self->struct_data->refcount = 1;

    return call_init(self, ctor_args, self->type_id);
}

void _pw_struct_release(PwValuePtr self)
{
    // call fini methods

    PwType* type = pw_typeof(self);
    while (type->id != PwTypeId_Struct) {
        PwMethodFini fini = type->fini;
        if (fini) {
            fini(self);
        }
        type = pw_ancestor_of(type->id);
    }

    // release memory

    type = pw_typeof(self);

    unsigned memsize = type->data_offset + type->data_size;
    _pw_free(self->type_id, (void**) &self->struct_data, memsize);

    // reset value to Null
    self->type_id = PwTypeId_Null;
}

[[nodiscard]] bool _pw_struct_create(PwTypeId type_id, void* ctor_args, PwValuePtr result)
{
    pw_destroy(result);
    result->type_id = type_id;
    return _pw_struct_alloc(result, ctor_args);
}

void _pw_struct_destroy(PwValuePtr self)
{
    _PwStructData* struct_data = self->struct_data;

    if (!struct_data) {
        return;
    }
    if (struct_data->refcount) {
        struct_data->refcount--;
    }
    if (struct_data->refcount) {
        return;
    }
    _pw_struct_release(self);
}

void _pw_struct_clone(PwValuePtr self)
{
    if (self->struct_data) {
        self->struct_data->refcount++;
    }
}

void _pw_struct_hash(PwValuePtr self, PwHashContext* ctx)
{
    _pw_hash_uint64(ctx, self->type_id);
    _pw_hash_uint64(ctx, (uint64_t) self->struct_data);
}

[[nodiscard]] bool _pw_struct_deepcopy(PwValuePtr self, PwValuePtr result)
{
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
}

void _pw_dump_struct_data(FILE* fp, PwValuePtr value)
{
    if (value->struct_data) {
        fprintf(fp, " data=%p refcount=%u;", (void*) value->struct_data, value->struct_data->refcount);
    } else {
        fprintf(fp, " data=NULL;");
    }
}

static void struct_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    _pw_dump_struct_data(fp, self);
}

[[nodiscard]] bool _pw_struct_to_string(PwValuePtr self, PwValuePtr result)
{
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
}

[[nodiscard]] bool _pw_struct_is_true(PwValuePtr self)
{
    return false;
}

[[nodiscard]] bool _pw_struct_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    // basic Structs are empty and empty always equals empty
    return true;
}

[[nodiscard]] bool _pw_struct_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        if (t == PwTypeId_Struct) {
            // basic Structs are empty and empty always equals empty
            return true;
        }
        // check base type
        t = _pw_types[t]->ancestor_id;
        if (t == PwTypeId_Null) {
            return false;
        }
    }
}

PwType _pw_struct_type = {
    .id             = PwTypeId_Struct,
    .ancestor_id    = PwTypeId_Null,  // no ancestor
    .name           = "Struct",
    .allocator      = &default_allocator,

    .create         = _pw_struct_create,
    .destroy        = _pw_struct_destroy,
    .clone          = _pw_struct_clone,
    .hash           = _pw_struct_hash,
    .deepcopy       = _pw_struct_deepcopy,
    .dump           = struct_dump,
    .to_string      = _pw_struct_to_string,
    .is_true        = _pw_struct_is_true,
    .equal_sametype = _pw_struct_equal_sametype,
    .equal          = _pw_struct_equal,

    .data_offset    = 0,
    .data_size      = sizeof(_PwStructData),
};
