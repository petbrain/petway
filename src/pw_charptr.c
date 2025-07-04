#include <string.h>

#include "include/pw.h"

#include "src/pw_charptr_internal.h"
#include "src/pw_string_internal.h"

[[noreturn]]
void _pw_panic_bad_charptr_subtype(PwValuePtr v)
{
    pw_dump(stderr, v);
    pw_panic("Bad charptr subtype %u\n", v->charptr_subtype);
}

[[nodiscard]] static bool charptr_create(PwTypeId type_id, void* ctor_args, PwValuePtr result)
{
    // XXX use ctor_args for initializer?

    static char8_t emptystr[1] = {0};

    *result = PwCharPtr(emptystr);
    return true;
}

static void charptr_hash(PwValuePtr self, PwHashContext* ctx)
{
    /*
     * To be able to use CharPtr as arguments to map functions,
     * make hashes same as for PW string.
     */
    _pw_hash_uint64(ctx, PwTypeId_String);

    switch (self->charptr_subtype) {
        case PwSubType_CharPtr:
            if (self->charptr) {
                char8_t* ptr = self->charptr;
                for (;;) {
                    union {
                        struct {
                            char32_t a;
                            char32_t b;
                        };
                        uint64_t i64;
                    } data;
                    data.a = read_utf8_char(&ptr);
                    if (data.a == 0) {
                        break;
                    }
                    data.b = read_utf8_char(&ptr);
                    _pw_hash_uint64(ctx, data.i64);
                    if (data.b == 0) {
                        break;
                    }
                }
            }
            break;

        case PwSubType_Char32Ptr:
            if (self->char32ptr) {
                char32_t* ptr = self->char32ptr;
                for (;;) {
                    union {
                        struct {
                            char32_t a;
                            char32_t b;
                        };
                        uint64_t i64;
                    } data;
                    data.a = *ptr++;
                    if (data.a == 0) {
                        break;
                    }
                    data.b = *ptr++;
                    _pw_hash_uint64(ctx, data.i64);
                    if (data.b == 0) {
                        break;
                    }
                }
            }
            break;

        default:
            _pw_panic_bad_charptr_subtype(self);
    }
}

static void charptr_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    fputc('\n', fp);
    _pw_print_indent(fp, next_indent);

    switch (self->charptr_subtype) {
        case PwSubType_CharPtr: {
            fputs("char8_t*: ", fp);
            char8_t* ptr = self->charptr;
            for (unsigned i = 0;; i++) {
                char32_t c = read_utf8_char(&ptr);
                if (c == 0) {
                    break;
                }
                _pw_putchar32_utf8(fp, c);
                if (i == 80) {
                    fprintf(fp, "...");
                    break;
                }
            }
            break;
        }
        case PwSubType_Char32Ptr: {
            fputs("char32_t*: ", fp);
            char32_t* ptr = self->char32ptr;
            for (unsigned i = 0;; i++) {
                char32_t c = *ptr++;
                if (c == 0) {
                    break;
                }
                _pw_putchar32_utf8(fp, c);
                if (i == 80) {
                    fprintf(fp, "...");
                    break;
                }
            }
            break;
        }
        default:
            _pw_panic_bad_charptr_subtype(self);
    }
    fputc('\n', fp);
}

[[nodiscard]] bool pw_charptr_to_string(PwValuePtr self, PwValuePtr result)
{
    switch (self->charptr_subtype) {
        case PwSubType_CharPtr:   return _pw_create_string_u8  (self->charptr, result);
        case PwSubType_Char32Ptr: return _pw_create_string_u32 (self->char32ptr, result);
        default:
            _pw_panic_bad_charptr_subtype(self);
    }
}

[[nodiscard]] static bool charptr_is_true(PwValuePtr self)
{
    switch (self->charptr_subtype) {
        case PwSubType_CharPtr:   return self->charptr != nullptr && *self->charptr;
        case PwSubType_Char32Ptr: return self->char32ptr != nullptr && *self->char32ptr;
        default:
            _pw_panic_bad_charptr_subtype(self);
    }
}

[[nodiscard]] static bool charptr_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    switch (self->charptr_subtype) {
        case PwSubType_CharPtr:
            switch (other->charptr_subtype) {
                case PwSubType_CharPtr:
                    if (self->charptr == nullptr) {
                        return other->charptr == nullptr;
                    } else {
                        return strcmp((char*) self->charptr, (char*) other->charptr) == 0;
                    }
                case PwSubType_Char32Ptr:
                    if (self->char32ptr == nullptr) {
                        return other->char32ptr == nullptr;
                    } else {
                        return u32_strcmp_u8(other->char32ptr, self->charptr) == 0;
                    }
                default:
                    _pw_panic_bad_charptr_subtype(other);
            }

        case PwSubType_Char32Ptr:
            switch (other->charptr_subtype) {
                case PwSubType_CharPtr:
                    if (self->char32ptr == nullptr) {
                        return other->charptr == nullptr;
                    } else {
                        return u32_strcmp_u8(self->char32ptr, other->charptr) == 0;
                    }
                case PwSubType_Char32Ptr:
                    if (self->char32ptr == nullptr) {
                        return other->char32ptr == nullptr;
                    } else {
                        return u32_strcmp(self->char32ptr, other->char32ptr) == 0;
                    }
                default:
                    _pw_panic_bad_charptr_subtype(other);
            }

        default:
            _pw_panic_bad_charptr_subtype(self);
    }
}

[[nodiscard]] static bool charptr_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        switch (t) {
            case PwTypeId_Null:
                return self->charptr == nullptr;

            case PwTypeId_Ptr:
                return self->charptr == other->ptr;

            case PwTypeId_String:
                return _pw_charptr_equal_string(self, other);

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

PwType _pw_charptr_type = {
    .id             = PwTypeId_CharPtr,
    .ancestor_id    = PwTypeId_Null,  // no ancestor
    .name           = "CharPtr",
    .allocator      = nullptr,
    .create         = charptr_create,
    .destroy        = nullptr,
    .clone          = nullptr,
    .hash           = charptr_hash,
    .deepcopy       = pw_charptr_to_string,
    .dump           = charptr_dump,
    .to_string      = pw_charptr_to_string,
    .is_true        = charptr_is_true,
    .equal_sametype = charptr_equal_sametype,
    .equal          = charptr_equal,
};

[[nodiscard]] bool pw_charptr_to_string_inplace(PwValuePtr v)
{
    if (!pw_is_charptr(v)) {
        return true;
    }
    PwValue result = PW_NULL;
    if (!pw_charptr_to_string(v, &result)) {
        return false;
    }
    pw_move(&result, v);
    return true;
}

[[nodiscard]] bool _pw_charptr_equal_string(PwValuePtr charptr, PwValuePtr str)
{
    switch (charptr->charptr_subtype) {
        case PwSubType_CharPtr:
            if (charptr->charptr == nullptr) {
                return false;
            } else {
                return _pw_equal_u8(str, charptr->charptr);
            }
        case PwSubType_Char32Ptr:
            if (charptr->char32ptr == nullptr) {
                return false;
            } else {
                return _pw_equal_u32(str, charptr->char32ptr);
            }
        default:
            _pw_panic_bad_charptr_subtype(charptr);
    }
}

[[nodiscard]] unsigned _pw_charptr_strlen2(PwValuePtr charptr, uint8_t* char_size)
{
    switch (charptr->charptr_subtype) {
        case PwSubType_CharPtr:   return utf8_strlen2(charptr->charptr, char_size);
        case PwSubType_Char32Ptr: return u32_strlen2(charptr->char32ptr, char_size);
        default:
            _pw_panic_bad_charptr_subtype(charptr);
    }
}
