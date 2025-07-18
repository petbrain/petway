#include <stdarg.h>

#include <libpussy/mmarray.h>

#include "include/pw.h"
#include "src/pw_array_internal.h"
#include "src/pw_compound_internal.h"
#include "src/pw_interfaces_internal.h"
#include "src/pw_string_internal.h"
#include "src/pw_struct_internal.h"

PwType** _pw_types = nullptr;
static PwTypeId num_pw_types = 0;

/****************************************************************
 * Null type
 */

[[nodiscard]] static bool null_create(PwTypeId type_id, void* ctor_args, PwValuePtr result)
{
    pw_destroy(result);
    return true;
}

static void null_hash(PwValuePtr self, PwHashContext* ctx)
{
    _pw_hash_uint64(ctx, PwTypeId_Null);
}

static void null_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    fputc('\n', fp);
}

[[nodiscard]] static bool null_to_string(PwValuePtr self, PwValuePtr result)
{
    pw_destroy(result);
    *result = PwString("null");
    return true;
}

[[nodiscard]] static bool null_is_true(PwValuePtr self)
{
    return false;
}

[[nodiscard]] static bool null_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    return true;
}

[[nodiscard]] static bool null_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        switch (t) {
            case PwTypeId_Null:
                return true;

            case PwTypeId_Ptr:
                return other->ptr == nullptr;

            default: {
                // check base type
                t = _pw_types[t]->ancestor_id;
                if (t == PwTypeId_Null) {
                    return false;
                }
            }
        }
    }
}

static PwType null_type = {
    .id             = PwTypeId_Null,
    .ancestor_id    = PwTypeId_Null,  // no ancestor; Null can't be an ancestor for any type
    .name           = "Null",
    .create         = null_create,
    .destroy        = nullptr,
    .clone          = nullptr,
    .hash           = null_hash,
    .deepcopy       = nullptr,
    .dump           = null_dump,
    .to_string      = null_to_string,
    .is_true        = null_is_true,
    .equal_sametype = null_equal_sametype,
    .equal          = null_equal
};

/****************************************************************
 * Bool type
 */

[[nodiscard]] static bool bool_create(PwTypeId type_id, void* ctor_args, PwValuePtr result)
{
    // XXX use ctor_args for initializer?
    pw_destroy(result);
    *result = PwBool(false);
    return true;
}

static void bool_hash(PwValuePtr self, PwHashContext* ctx)
{
    // mind maps: the hash should be the same for subtypes, that's why not using self->type_id here
    _pw_hash_uint64(ctx, PwTypeId_Bool);
    _pw_hash_uint64(ctx, self->bool_value);
}

static void bool_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    fprintf(fp, ": %s\n", self->bool_value? "true" : "false");
}

[[nodiscard]] static bool bool_to_string(PwValuePtr self, PwValuePtr result)
{
    pw_destroy(result);
    if (self->bool_value) {
        *result = PwString("true");
    } else {
        *result = PwString("false");
    }
    return true;
}

[[nodiscard]] static bool bool_is_true(PwValuePtr self)
{
    return self->bool_value;
}

[[nodiscard]] static bool bool_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    return self->bool_value == other->bool_value;
}

[[nodiscard]] static bool bool_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        if (t == PwTypeId_Bool) {
            return self->bool_value == other->bool_value;
        }
        // check base type
        t = _pw_types[t]->ancestor_id;
        if (t == PwTypeId_Null) {
            return false;
        }
    }
}

static PwType bool_type = {
    .id             = PwTypeId_Bool,
    .ancestor_id    = PwTypeId_Null,  // no ancestor
    .name           = "Bool",
    .create         = bool_create,
    .destroy        = nullptr,
    .clone          = nullptr,
    .hash           = bool_hash,
    .deepcopy       = nullptr,
    .dump           = bool_dump,
    .to_string      = bool_to_string,
    .is_true        = bool_is_true,
    .equal_sametype = bool_equal_sametype,
    .equal          = bool_equal

    // [PwInterfaceId_Logic] = &bool_type_logic_interface
};

/****************************************************************
 * Abstract Integer type
 */

[[nodiscard]] static bool int_create(PwTypeId type_id, void* ctor_args, PwValuePtr result)
{
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
}

static void int_hash(PwValuePtr self, PwHashContext* ctx)
{
    _pw_hash_uint64(ctx, PwTypeId_Int);
}

static void int_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    fputs(": abstract\n", fp);
}

[[nodiscard]] static bool int_to_string(PwValuePtr self, PwValuePtr result)
{
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
}

[[nodiscard]] static bool int_is_true(PwValuePtr self)
{
    return self->signed_value;
}

[[nodiscard]] static bool int_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    return true;
}

[[nodiscard]] static bool int_equal(PwValuePtr self, PwValuePtr other)
{
    return false;
}

static PwType int_type = {
    .id             = PwTypeId_Int,
    .ancestor_id    = PwTypeId_Null,  // no ancestor
    .name           = "Int",
    .create         = int_create,
    .destroy        = nullptr,
    .clone          = nullptr,
    .hash           = int_hash,
    .deepcopy       = nullptr,
    .dump           = int_dump,
    .to_string      = int_to_string,
    .is_true        = int_is_true,
    .equal_sametype = int_equal_sametype,
    .equal          = int_equal

    // [PwInterfaceId_Logic]      = &int_type_logic_interface,
    // [PwInterfaceId_Arithmetic] = &int_type_arithmetic_interface,
    // [PwInterfaceId_Bitwise]    = &int_type_bitwise_interface,
    // [PwInterfaceId_Comparison] = &int_type_comparison_interface
};

/****************************************************************
 * Signed type
 */

[[nodiscard]] static bool signed_create(PwTypeId type_id, void* ctor_args, PwValuePtr result)
{
    // XXX use ctor_args for initializer?
    pw_destroy(result);
    *result = PwSigned(0);
    return true;
}

static void signed_hash(PwValuePtr self, PwHashContext* ctx)
{
    // mind maps: same signed and unsigned integers must have same hash, so
    if (self->signed_value < 0) {
        _pw_hash_uint64(ctx, PwTypeId_Signed);
    } else {
        _pw_hash_uint64(ctx, PwTypeId_Unsigned);
    }
    _pw_hash_uint64(ctx, self->signed_value);
}

static void signed_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    fprintf(fp, ": %lld\n", (long long) self->signed_value);
}

[[nodiscard]] static bool signed_to_string(PwValuePtr self, PwValuePtr result)
{
    pw_destroy(result);
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
}

[[nodiscard]] static bool signed_is_true(PwValuePtr self)
{
    return self->signed_value;
}

[[nodiscard]] static bool signed_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    return self->signed_value == other->signed_value;
}

[[nodiscard]] static bool signed_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        switch (t) {
            case PwTypeId_Signed:
                return self->signed_value == other->signed_value;

            case PwTypeId_Unsigned:
                if (self->signed_value < 0) {
                    return false;
                } else {
                    return self->signed_value == (PwType_Signed) other->unsigned_value;
                }

            case PwTypeId_Float:
                return self->signed_value == other->float_value;

            default: {
                // check base type
                t = _pw_types[t]->ancestor_id;
                if (t == PwTypeId_Null) {
                    return false;
                }
            }
        }
    }
}

static PwType signed_type = {
    .id             = PwTypeId_Signed,
    .ancestor_id    = PwTypeId_Int,
    .name           = "Signed",
    .create         = signed_create,
    .destroy        = nullptr,
    .clone          = nullptr,
    .hash           = signed_hash,
    .deepcopy       = nullptr,
    .dump           = signed_dump,
    .to_string      = signed_to_string,
    .is_true        = signed_is_true,
    .equal_sametype = signed_equal_sametype,
    .equal          = signed_equal

    // [PwInterfaceId_Logic]      = &int_type_logic_interface,
    // [PwInterfaceId_Arithmetic] = &int_type_arithmetic_interface,
    // [PwInterfaceId_Bitwise]    = &int_type_bitwise_interface,
    // [PwInterfaceId_Comparison] = &int_type_comparison_interface
};

/****************************************************************
 * Unsigned type
 */

[[nodiscard]] static bool unsigned_create(PwTypeId type_id, void* ctor_args, PwValuePtr result)
{
    // XXX use ctor_args for initializer?
    pw_destroy(result);
    *result = PwUnsigned(0);
    return true;
}

static void unsigned_hash(PwValuePtr self, PwHashContext* ctx)
{
    // mind maps: same signed and unsigned integers must have same hash,
    // so using PwTypeId_Unsigned, not self->type_id
    _pw_hash_uint64(ctx, PwTypeId_Unsigned);
    _pw_hash_uint64(ctx, self->unsigned_value);
}

static void unsigned_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    fprintf(fp, ": %llu\n", (unsigned long long) self->unsigned_value);
}

[[nodiscard]] static bool unsigned_to_string(PwValuePtr self, PwValuePtr result)
{
    pw_destroy(result);
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
}

[[nodiscard]] static bool unsigned_is_true(PwValuePtr self)
{
    return self->unsigned_value;
}

[[nodiscard]] static bool unsigned_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    return self->unsigned_value == other->unsigned_value;
}

[[nodiscard]] static bool unsigned_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        switch (t) {
            case PwTypeId_Signed:
                if (other->signed_value < 0) {
                    return false;
                } else {
                    return ((PwType_Signed) self->unsigned_value) == other->signed_value;
                }

            case PwTypeId_Unsigned:
                return self->unsigned_value == other->unsigned_value;

            case PwTypeId_Float:
                return self->unsigned_value == other->float_value;

            default: {
                // check base type
                t = _pw_types[t]->ancestor_id;
                if (t == PwTypeId_Null) {
                    return false;
                }
            }
        }
    }
}

static PwType unsigned_type = {
    .id             = PwTypeId_Unsigned,
    .ancestor_id    = PwTypeId_Int,
    .name           = "Unsigned",
    .create         = unsigned_create,
    .destroy        = nullptr,
    .clone          = nullptr,
    .hash           = unsigned_hash,
    .deepcopy       = nullptr,
    .dump           = unsigned_dump,
    .to_string      = unsigned_to_string,
    .is_true        = unsigned_is_true,
    .equal_sametype = unsigned_equal_sametype,
    .equal          = unsigned_equal

    // [PwInterfaceId_Logic]      = &int_type_logic_interface,
    // [PwInterfaceId_Arithmetic] = &int_type_arithmetic_interface,
    // [PwInterfaceId_Bitwise]    = &int_type_bitwise_interface,
    // [PwInterfaceId_Comparison] = &int_type_comparison_interface
};

/****************************************************************
 * Float type
 */

[[nodiscard]] static bool float_create(PwTypeId type_id, void* ctor_args, PwValuePtr result)
{
    // XXX use ctor_args for initializer?
    pw_destroy(result);
    *result = PwFloat(0.0);
    return true;
}

static void float_hash(PwValuePtr self, PwHashContext* ctx)
{
    // mind maps: the hash should be the same for subtypes, that's why not using self->type_id here
    _pw_hash_uint64(ctx, PwTypeId_Float);
    _pw_hash_buffer(ctx, &self->float_value, sizeof(self->float_value));
}

static void float_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    fprintf(fp, ": %f\n", self->float_value);
}

[[nodiscard]] static bool float_to_string(PwValuePtr self, PwValuePtr result)
{
    pw_destroy(result);
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
}

[[nodiscard]] static bool float_is_true(PwValuePtr self)
{
    return self->float_value;
}

[[nodiscard]] static bool float_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    return self->float_value == other->float_value;
}

[[nodiscard]] static bool float_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        switch (t) {
            case PwTypeId_Signed:   return self->float_value == (PwType_Float) other->signed_value;
            case PwTypeId_Unsigned: return self->float_value == (PwType_Float) other->unsigned_value;
            case PwTypeId_Float:    return self->float_value == other->float_value;
            default: {
                // check base type
                t = _pw_types[t]->ancestor_id;
                if (t == PwTypeId_Null) {
                    return false;
                }
            }
        }
    }
}

static PwType float_type = {
    .id             = PwTypeId_Float,
    .ancestor_id    = PwTypeId_Null,  // no ancestor
    .name           = "Float",
    .create         = float_create,
    .destroy        = nullptr,
    .clone          = nullptr,
    .hash           = float_hash,
    .deepcopy       = nullptr,
    .dump           = float_dump,
    .to_string      = float_to_string,
    .is_true        = float_is_true,
    .equal_sametype = float_equal_sametype,
    .equal          = float_equal

    // [PwInterfaceId_Logic]      = &float_type_logic_interface,
    // [PwInterfaceId_Arithmetic] = &float_type_arithmetic_interface,
    // [PwInterfaceId_Comparison] = &float_type_comparison_interface
};

/****************************************************************
 * DateTime type
 */

[[nodiscard]] static bool datetime_create(PwTypeId type_id, void* ctor_args, PwValuePtr result)
{
    // XXX use ctor_args for initializer?
    pw_destroy(result);
    *result = PwDateTime();
    return true;
}

static void datetime_hash(PwValuePtr self, PwHashContext* ctx)
{
    // mind maps: the hash should be the same for subtypes, that's why not using self->type_id here
    _pw_hash_uint64(ctx, PwTypeId_DateTime);
    _pw_hash_uint64(ctx, self->year);
    _pw_hash_uint64(ctx, self->month);
    _pw_hash_uint64(ctx, self->day);
    _pw_hash_uint64(ctx, self->hour);
    _pw_hash_uint64(ctx, self->minute);
    _pw_hash_uint64(ctx, self->second);
    _pw_hash_uint64(ctx, self->nanosecond);
    _pw_hash_uint64(ctx, self->gmt_offset + (1L << 8 * sizeof(self->gmt_offset)));  // make positive (biased)
}

static void datetime_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);

    fprintf(fp, ": %04u-%02u-%02u %02u:%02u:%02u",
            self->year, self->month, self->day,
            self->hour, self->minute, self->second);

    if (self->nanosecond) {
        // format fractional part and print &frac[1] later
        char frac[12];
        snprintf(frac, sizeof(frac), "%u", self->nanosecond + 1000'000'000);
        fputs(&frac[1], fp);
    }
    if (self->gmt_offset) {
        // gmt_offset can be negative
        int offset_hours = self->gmt_offset / 60;
        int offset_minutes = self->gmt_offset % 60;
        // make sure minutes are positive
        if (offset_minutes < 0) {
            offset_minutes = -offset_minutes;
        }
        fprintf(fp, "%c%02d:%02d", (offset_hours< 0)? '-' : '+', offset_hours, offset_minutes);
    }
    fputc('\n', fp);
}

[[nodiscard]] static bool datetime_to_string(PwValuePtr self, PwValuePtr result)
{
    pw_destroy(result);
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
}

[[nodiscard]] static bool datetime_is_true(PwValuePtr self)
{
    return self->year && self->month && self->day
           && self->hour && self->minute && self->second && self->nanosecond;
}

[[nodiscard]] static bool datetime_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    return self->year       == other->year &&
           self->month      == other->month &&
           self->day        == other->day &&
           self->hour       == other->hour &&
           self->minute     == other->minute &&
           self->second     == other->second &&
           self->nanosecond == other->nanosecond &&
           self->gmt_offset == other->gmt_offset;
}

[[nodiscard]] static bool datetime_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        if (t == PwTypeId_DateTime) {
            return datetime_equal_sametype(self, other);
        }
        // check base type
        t = _pw_types[t]->ancestor_id;
        if (t == PwTypeId_Null) {
            return false;
        }
    }
}

static PwType datetime_type = {
    .id             = PwTypeId_DateTime,
    .ancestor_id    = PwTypeId_Null,  // no ancestor
    .name           = "DateTime",
    .create         = datetime_create,
    .destroy        = nullptr,
    .clone          = nullptr,
    .hash           = datetime_hash,
    .deepcopy       = datetime_to_string,
    .dump           = datetime_dump,
    .to_string      = datetime_to_string,
    .is_true        = datetime_is_true,
    .equal_sametype = datetime_equal_sametype,
    .equal          = datetime_equal
};

/****************************************************************
 * Timestamp type
 */

[[nodiscard]] static bool timestamp_create(PwTypeId type_id, void* ctor_args, PwValuePtr result)
{
    // XXX use ctor_args for initializer?
    pw_destroy(result);
    *result = PwTimestamp();
    return true;
}

static void timestamp_hash(PwValuePtr self, PwHashContext* ctx)
{
    // mind maps: the hash should be the same for subtypes, that's why not using self->type_id here
    _pw_hash_uint64(ctx, PwTypeId_Timestamp);
    _pw_hash_uint64(ctx, self->ts_seconds);
    _pw_hash_uint64(ctx, self->ts_nanoseconds);
}

static void timestamp_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    fprintf(fp, ": %zu", self->ts_seconds);
    if (self->ts_nanoseconds) {
        // format fractional part and print &frac[1] later
        char frac[12];
        snprintf(frac, sizeof(frac), "%u", self->ts_nanoseconds + 1000'000'000);
        fputs(&frac[1], fp);
    }
    fputc('\n', fp);
}

[[nodiscard]] static bool timestamp_to_string(PwValuePtr self, PwValuePtr result)
{
    pw_destroy(result);
    char buf[32];
    snprintf(buf, sizeof(buf), "%zu.%09u", self->ts_seconds, self->ts_nanoseconds);
    return pw_create_string(buf, result);
}

[[nodiscard]] static bool timestamp_is_true(PwValuePtr self)
{
    return self->ts_seconds && self->ts_nanoseconds;
}

[[nodiscard]] static bool timestamp_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    return self->ts_seconds == other->ts_seconds
        && self->ts_nanoseconds == other->ts_nanoseconds;
}

[[nodiscard]] static bool timestamp_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        if (t == PwTypeId_Timestamp) {
            return timestamp_equal_sametype(self, other);
        }
        // check base type
        t = _pw_types[t]->ancestor_id;
        if (t == PwTypeId_Null) {
            return false;
        }
    }
}

static PwType timestamp_type = {
    .id             = PwTypeId_Timestamp,
    .ancestor_id    = PwTypeId_Null,  // no ancestor
    .name           = "Timestamp",
    .create         = timestamp_create,
    .destroy        = nullptr,
    .clone          = nullptr,
    .hash           = timestamp_hash,
    .deepcopy       = timestamp_to_string,
    .dump           = timestamp_dump,
    .to_string      = timestamp_to_string,
    .is_true        = timestamp_is_true,
    .equal_sametype = timestamp_equal_sametype,
    .equal          = timestamp_equal
};

/****************************************************************
 * Pointer type
 */

[[nodiscard]] static bool ptr_create(PwTypeId type_id, void* ctor_args, PwValuePtr result)
{
    // XXX use ctor_args for initializer?
    pw_destroy(result);
    *result = PwPtr(nullptr);
    return true;
}

static void ptr_hash(PwValuePtr self, PwHashContext* ctx)
{
    // mind maps: the hash should be the same for subtypes, that's why not using self->type_id here
    _pw_hash_uint64(ctx, PwTypeId_Ptr);
    _pw_hash_buffer(ctx, &self->ptr, sizeof(self->ptr));
}

static void ptr_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    fprintf(fp, ": %p\n", self->ptr);
}

[[nodiscard]] static bool ptr_to_string(PwValuePtr self, PwValuePtr result)
{
    pw_destroy(result);
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
}

[[nodiscard]] static bool ptr_is_true(PwValuePtr self)
{
    return self->ptr;
}

[[nodiscard]] static bool ptr_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    return self->ptr == other->ptr;
}

[[nodiscard]] static bool ptr_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        switch (t) {
            case PwTypeId_Null:
                return self->ptr == nullptr;

            case PwTypeId_Ptr:
                return self->ptr == other->ptr;

            default: {
                // check base type
                t = _pw_types[t]->ancestor_id;
                if (t == PwTypeId_Null) {
                    return false;
                }
            }
        }
    }
}

static PwType ptr_type = {
    .id             = PwTypeId_Ptr,
    .ancestor_id    = PwTypeId_Null,  // no ancestor
    .name           = "Ptr",
    .create         = ptr_create,
    .destroy        = nullptr,
    .clone          = nullptr,
    .hash           = ptr_hash,
    .deepcopy       = ptr_to_string,
    .dump           = ptr_dump,
    .to_string      = ptr_to_string,
    .is_true        = ptr_is_true,
    .equal_sametype = ptr_equal_sametype,
    .equal          = ptr_equal
};

/****************************************************************
 * Type system initialization.
 */

extern PwType _pw_status_type;    // defined in pw_status.c
extern PwType _pw_iterator_type;  // defined in pw_iterator.c
extern PwType _pw_map_type;       // defined in pw_map.c

static PwType* basic_types[] = {
    [PwTypeId_Null]      = &null_type,
    [PwTypeId_Bool]      = &bool_type,
    [PwTypeId_Int]       = &int_type,
    [PwTypeId_Signed]    = &signed_type,
    [PwTypeId_Unsigned]  = &unsigned_type,
    [PwTypeId_Float]     = &float_type,
    [PwTypeId_DateTime]  = &datetime_type,
    [PwTypeId_Timestamp] = &timestamp_type,
    [PwTypeId_Ptr]       = &ptr_type,
    [PwTypeId_String]    = &_pw_string_type,
    [PwTypeId_Struct]    = &_pw_struct_type,
    [PwTypeId_Compound]  = &_pw_compound_type,
    [PwTypeId_Status]    = &_pw_status_type,
    [PwTypeId_Iterator]  = &_pw_iterator_type,
    [PwTypeId_Array]     = &_pw_array_type,
    [PwTypeId_Map]       = &_pw_map_type
};

[[ gnu::constructor ]]
void _pw_init_types()
{
    _pw_init_interfaces();

    if (_pw_types) {
        return;
    }

    num_pw_types = PW_LENGTH(basic_types);
    _pw_types = mmarray_allocate(num_pw_types, sizeof(PwType*));

    for(PwTypeId i = 0; i < num_pw_types; i++) {
        PwType* t = basic_types[i];
        if (!t) {
            pw_panic("Type %u is not defined\n", i);
        }
        _pw_types[i] = t;
    }
}

static PwTypeId add_type(PwType* type)
{
    if (num_pw_types == ((1 << 8 * sizeof(PwTypeId)) - 1)) {
        pw_panic("Cannot define more types than %u\n", num_pw_types);
    }
    _pw_types = mmarray_append_item(_pw_types, &type);
    PwTypeId type_id = num_pw_types++;
    type->id = type_id;
    return type_id;
}

PwTypeId _pw_add_type(PwType* type, ...)
{
    // the order constructor are called is undefined, make sure the type system is initialized
    _pw_init_types();

    // add type

    PwTypeId type_id = add_type(type);

    va_list ap;
    va_start(ap);
    _pw_create_interfaces(type, ap);
    va_end(ap);

    return type_id;
}

PwTypeId _pw_subtype(PwType* type, char* name, PwTypeId ancestor_id,
                     unsigned data_size, unsigned alignment, ...)
{
    // the order constructor are called is undefined, make sure the type system is initialized
    _pw_init_types();

    pw_assert(ancestor_id != PwTypeId_Null);

    PwType* ancestor = _pw_types[ancestor_id];

    *type = *ancestor;  // copy type structure
    type->init = nullptr;
    type->fini = nullptr;

    type->ancestor_id = ancestor_id;
    type->name = name;
    type->data_offset = align_unsigned(ancestor->data_offset + ancestor->data_size, alignment);
    type->data_size = data_size;

    PwTypeId type_id = add_type(type);

    va_list ap;
    va_start(ap);
    _pw_update_interfaces(type, ancestor, ap);
    va_end(ap);

    return type_id;
}

void pw_dump_types(FILE* fp)
{
    fputs( "=== PW types ===\n", fp);
    for (PwTypeId i = 0; i < num_pw_types; i++) {
        PwType* t = _pw_types[i];
        fprintf(fp, "%u: %s; ancestor=%u (%s)\n",
                t->id, t->name, t->ancestor_id, _pw_types[t->ancestor_id]->name);
        if (t->num_interfaces) {
            _PwInterface* iface = t->interfaces;
            for (unsigned j = 0; j < t->num_interfaces; j++, iface++) {
                unsigned num_methods = _pw_get_num_interface_methods(iface->interface_id);
                void** methods = iface->interface_methods;
                fprintf(fp, "    interface %u (%s):\n        ",
                        iface->interface_id, pw_get_interface_name(iface->interface_id));
                for (unsigned k = 0; k < num_methods; k++, methods++) {
                    fprintf(fp, "%p ", *methods);
                }
                fputc('\n', fp);
            }
        }
    }
}
