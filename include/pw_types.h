#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <uchar.h>

#include <libpussy/allocator.h>

#include <pw_helpers.h>

#ifdef __cplusplus
extern "C" {
#endif

// automatically cleaned value
#define _PW_VALUE_CLEANUP [[ gnu::cleanup(pw_destroy) ]]
#define PwValue _PW_VALUE_CLEANUP _PwValue

// Built-in types
#define PwTypeId_Null        0
#define PwTypeId_Bool        1U
#define PwTypeId_Int         2U  // abstract integer
#define PwTypeId_Signed      3U  // subtype of int, signed integer
#define PwTypeId_Unsigned    4U  // subtype of int, unsigned integer
#define PwTypeId_Float       5U
#define PwTypeId_DateTime    6U
#define PwTypeId_Timestamp   7U
#define PwTypeId_Ptr         8U  // container for void*
#define PwTypeId_CharPtr     9U  // container for pointers to static C strings
#define PwTypeId_String     10U
#define PwTypeId_Struct     11U  // the base for reference counted data
#define PwTypeId_Compound   12U  // the base for values that may contain circular references
#define PwTypeId_Status     13U  // value_data is optional
#define PwTypeId_Iterator   14U
#define PwTypeId_Array      15U
#define PwTypeId_Map        16U

// char* sub-types
#define PW_CHARPTR    0
#define PW_CHAR32PTR  1

// limits
#define PW_SIGNED_MAX  0x7fff'ffff'ffff'ffffLL
#define PW_UNSIGNED_MAX  0xffff'ffff'ffff'ffffULL

// type checking
#define pw_is_null(value)      pw_is_subtype((value), PwTypeId_Null)
#define pw_is_bool(value)      pw_is_subtype((value), PwTypeId_Bool)
#define pw_is_int(value)       pw_is_subtype((value), PwTypeId_Int)
#define pw_is_signed(value)    pw_is_subtype((value), PwTypeId_Signed)
#define pw_is_unsigned(value)  pw_is_subtype((value), PwTypeId_Unsigned)
#define pw_is_float(value)     pw_is_subtype((value), PwTypeId_Float)
#define pw_is_datetime(value)  pw_is_subtype((value), PwTypeId_DateTime)
#define pw_is_timestamp(value) pw_is_subtype((value), PwTypeId_Timestamp)
#define pw_is_ptr(value)       pw_is_subtype((value), PwTypeId_Ptr)
#define pw_is_charptr(value)   pw_is_subtype((value), PwTypeId_CharPtr)
#define pw_is_string(value)    pw_is_subtype((value), PwTypeId_String)
#define pw_is_struct(value)    pw_is_subtype((value), PwTypeId_Struct)
#define pw_is_compound(value)  pw_is_subtype((value), PwTypeId_Compound)
#define pw_is_status(value)    pw_is_subtype((value), PwTypeId_Status)
#define pw_is_iterator(value)  pw_is_subtype((value), PwTypeId_Iterator)
#define pw_is_array(value)     pw_is_subtype((value), PwTypeId_Array)
#define pw_is_map(value)       pw_is_subtype((value), PwTypeId_Map)

#define pw_assert_null(value)      pw_assert(pw_is_null    (value))
#define pw_assert_bool(value)      pw_assert(pw_is_bool    (value))
#define pw_assert_int(value)       pw_assert(pw_is_int     (value))
#define pw_assert_signed(value)    pw_assert(pw_is_signed  (value))
#define pw_assert_unsigned(value)  pw_assert(pw_is_unsigned(value))
#define pw_assert_float(value)     pw_assert(pw_is_float   (value))
#define pw_assert_datetime(value)  pw_assert(pw_is_datetime(value))
#define pw_assert_timestamp(value) pw_assert(pw_is_timestamp(value))
#define pw_assert_ptr(value)       pw_assert(pw_is_ptr     (value))
#define pw_assert_charptr(value)   pw_assert(pw_is_charptr (value))
#define pw_assert_string(value)    pw_assert(pw_is_string  (value))
#define pw_assert_struct(value)    pw_assert(pw_is_struct  (value))
#define pw_assert_compound(value)  pw_assert(pw_is_compound(value))
#define pw_assert_status(value)    pw_assert(pw_is_status  (value))
#define pw_assert_iterator(value)  pw_assert(pw_is_iterator(value))
#define pw_assert_array(value)     pw_assert(pw_is_array   (value))
#define pw_assert_map(value)       pw_assert(pw_is_map     (value))

// forward declarations

// defined in pw_status.h
struct __PwStatusData;

// defined in pw_interfaces.h
struct __PwInterface;
typedef struct __PwInterface _PwInterface;

// defined privately in src/pw_hash.c:
struct _PwHashContext;
typedef struct _PwHashContext PwHashContext;


// Type for type id.
typedef uint16_t PwTypeId;

// Integral types
typedef nullptr_t  PwType_Null;
typedef bool       PwType_Bool;
typedef int64_t    PwType_Signed;
typedef uint64_t   PwType_Unsigned;
typedef double     PwType_Float;

typedef struct {
    unsigned refcount;
    uint32_t capacity;
    uint8_t data[];
} _PwStringData;

typedef struct { uint8_t v[3]; } uint24_t;  // three bytes wide characters, always little-endian

typedef struct {
    union {
        // the basic structure contains reference count only,
        unsigned refcount;

        // but we need to make sure it has correct size for proper alignmenf of subsequent structures
        void* padding;
    };
} _PwStructData;

// make sure largest C type fits into 64 bits
static_assert( sizeof(long long) <= sizeof(uint64_t) );

union __PwValue {
    /*
     * 128-bit value
     */

    PwTypeId /* uint16_t */ type_id;

    uint64_t u64[2];

    struct {
        // integral types
        PwTypeId /* uint16_t */ _integral_type_id;
        uint8_t carry;  // for integer arithmetic
        uint8_t  _integral_pagging_1;
        uint32_t _integral_pagging_2;
        union {
            // Integral types
            PwType_Bool     bool_value;
            PwType_Signed   signed_value;
            PwType_Unsigned unsigned_value;
            PwType_Float    float_value;
        };
    };

    struct {
        // charptr and ptr
        PwTypeId /* uint16_t */ _charptr_type_id;
        uint8_t charptr_subtype; // see PW_CHAR*PTR constants
        uint8_t  _charptr_padding_1;
        uint32_t _charptr_pagging_2;
        union {
            // C string pointers for PwType_CharPtr
            char8_t*  charptr;
            char32_t* char32ptr;

            // void*
            void* ptr;
        };
    };

    struct {
        // struct
        PwTypeId /* uint16_t */ _struct_type_id;
        int16_t _struct_padding1;
        uint32_t _struct_padding2;
        _PwStructData* struct_data;  // the first member of struct_data is reference count
    };

    struct {
        // status
        PwTypeId /* uint16_t */ _status_type_id;
        int16_t pw_errno;
        uint16_t line_number;   // not set for PW_SUCCESS
        union {
            uint16_t is_error;  // all bit fields are zero for PW_SUCCESS
                                // so we can avoid bit operations when checking for success
            struct {
                uint16_t status_code: 15,
                         has_status_data: 1;
            };
        };
        union {
            char* file_name;  // when has_status_data == 0, not set for PW_SUCCESS
            struct __PwStatusData* status_data;  // when has_status_data == 1
        };
    };

    struct {
        // embedded string
        PwTypeId /* uint16_t */ _emb_string_type_id;
        uint8_t str_embedded:1,       // the string data is embedded into PwValue
                str_char_size:2;
        uint8_t str_embedded_length;  // length of embedded string
        union {
            uint8_t  str_1[12];
            uint16_t str_2[6];
            uint24_t str_3[4];
            uint32_t str_4[3];
        };
    };

    struct {
        // allocated string
        PwTypeId /* uint16_t */ _string_type_id;
        uint8_t _x_str_embedded:1,   // zero for allocated string
                _x_str_char_size:2;
        uint8_t _str_padding;
        uint32_t str_length;
        _PwStringData* string_data;
    };

    struct {
        // date/time
        PwTypeId /* uint16_t */ _datetime_type_id;
        uint16_t year;
        // -- 32 bits
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        // -- 64 bits
        uint32_t nanosecond;
        int16_t gmt_offset;  // in minutes
        uint8_t second;

        uint8_t tzindex;
        /* Index in the zone info cache.
         * If zero, zone info is undefined.
         */
    };

    struct {
        // timestamp
        PwTypeId /* uint16_t */ _timestamp_type_id;
        uint16_t _timestamp_padding;
        uint32_t ts_nanoseconds;
        uint64_t ts_seconds;
    };

};
typedef union __PwValue _PwValue;

typedef _PwValue* PwValuePtr;
typedef _PwValue  PwResult;  // alias for return values

// make sure _PwValue structure is correct
static_assert( offsetof(_PwValue, charptr_subtype) == 2 );

static_assert( offsetof(_PwValue, bool_value) == 8 );
static_assert( offsetof(_PwValue, charptr)    == 8 );
static_assert( offsetof(_PwValue, struct_data) == 8 );
static_assert( offsetof(_PwValue, status_data) == 8 );
static_assert( offsetof(_PwValue, string_data) == 8 );

static_assert( offsetof(_PwValue, str_embedded_length) == 3 );
static_assert( offsetof(_PwValue, str_1) == 4 );
static_assert( offsetof(_PwValue, str_2) == 4 );
static_assert( offsetof(_PwValue, str_3) == 4 );
static_assert( offsetof(_PwValue, str_4) == 4 );

static_assert( sizeof(_PwValue) == 16 );


struct __PwStatusData {
    unsigned refcount;
    unsigned line_number;
    char* file_name;
    _PwValue description;  // string
};
typedef struct __PwStatusData _PwStatusData;


struct __PwCompoundChain {
    /*
     * Compound values may contain cyclic references.
     * This structure along with function `_pw_on_chain`
     * helps to detect them.
     * See dump implementation for array and map values.
     */
    struct __PwCompoundChain* prev;
    PwValuePtr value;
};
typedef struct __PwCompoundChain _PwCompoundChain;

// Function types for the basic interface.
// The basic interface is embedded into PwType structure.
typedef PwResult (*PwMethodCreate)  (PwTypeId type_id, void* ctor_args);
typedef void     (*PwMethodDestroy) (PwValuePtr self);
typedef PwResult (*PwMethodClone)   (PwValuePtr self);
typedef void     (*PwMethodHash)    (PwValuePtr self, PwHashContext* ctx);
typedef PwResult (*PwMethodDeepCopy)(PwValuePtr self);
typedef void     (*PwMethodDump)    (PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail);
typedef PwResult (*PwMethodToString)(PwValuePtr self);
typedef bool     (*PwMethodIsTrue)  (PwValuePtr self);
typedef bool     (*PwMethodEqual)   (PwValuePtr self, PwValuePtr other);

// Function types for struct interface.
// They modify the data in place and return status.
typedef PwResult (*PwMethodInit)(PwValuePtr self, void* ctor_args);
typedef void     (*PwMethodFini)(PwValuePtr self);


typedef struct {
    /*
     * PW type
     */
    PwTypeId id;
    PwTypeId ancestor_id;
    char* name;
    Allocator* allocator;

    // basic interface
    // optional methods should be called only if not nullptr
    PwMethodCreate   create;    // mandatory
    PwMethodDestroy  destroy;   // optional if value does not have allocated data
    PwMethodClone    clone;     // optional; if set, it is called by pw_clone()
    PwMethodHash     hash;      // mandatory
    PwMethodDeepCopy deepcopy;  // optional; XXX how it should work with subtypes is not clear yet
    PwMethodDump     dump;      // mandatory
    PwMethodToString to_string; // mandatory
    PwMethodIsTrue   is_true;   // mandatory
    PwMethodEqual    equal_sametype;  // mandatory
    PwMethodEqual    equal;     // mandatory

    // struct data offset and size
    unsigned data_offset;
    unsigned data_size;

    /*
     * Struct interface
     *
     * Methods are optional and can be null.
     * They are called by _pw_struct_alloc and _pw_struct_release.
     *
     * init must be atomic, i.e. allocate either all data or nothing.
     * If init fails for some subtype, _pw_struct_alloc calls fini method
     * for already called init in the pedigree.
     */
    PwMethodInit init;
    PwMethodFini fini;

    // other interfaces
    unsigned num_interfaces;
    _PwInterface* interfaces;
} PwType;


void _pw_init_types();
/*
 * Initialize types.
 * Declared with [[ gnu::constructor ]] attribute and automatically called
 * before main().
 *
 * However, the order of initialization is undefined and other modules
 * that must call it explicitly from their constructors.
 *
 * This function is idempotent.
 */

extern PwType** _pw_types;
/*
 * Global list of types initialized with built-in types.
 */

#define pw_typeof(value)  (_pw_types[(value)->type_id])

#define pw_ancestor_of(type_id) (_pw_types[_pw_types[type_id]->ancestor_id])

static inline bool pw_is_subtype(PwValuePtr value, PwTypeId type_id)
{
    PwTypeId t = value->type_id;
    for (;;) {
        if (_pw_likely(t == type_id)) {
            return true;
        }
        t = _pw_types[t]->ancestor_id;
        if (_pw_likely(t == PwTypeId_Null)) {
            return false;
        }
    }
}

PwTypeId _pw_add_type(PwType* type, ...);
#define pw_add_type(type, ...)  _pw_add_type((type) __VA_OPT__(,) __VA_ARGS__, -1)
/*
 * Add type to the first available position in the global list.
 *
 * All fields of `type` must be initialized except interfaces.
 *
 * Variadic arguments are pairs of interface id and interface pointer
 * terminated by -1 (`pw_add_type` wrapper does that by default).
 *
 * Return new type id.
 *
 * All errors in this function are considered as critical and cause program abort.
 */

PwTypeId _pw_subtype(PwType* type, char* name, PwTypeId ancestor_id,
                     unsigned data_size, unsigned alignment, ...);

#define pw_subtype(type, name, ancestor_id, ...)  \
    _pw_subtype((type), (name), (ancestor_id), 0, 0 __VA_OPT__(,) __VA_ARGS__, -1)

#define pw_struct_subtype(type, name, ancestor_id, data_type, ...)  \
    _pw_subtype((type), (name), (ancestor_id), \
                sizeof(data_type), alignof(data_type) __VA_OPT__(,) __VA_ARGS__, -1)
/*
 * `type` and `name` should point to a static storage.
 *
 * `pw_subtype` creates subtype with no associated data.
 * This can also be used to create a subtype of Struct that does not extend
 * ancestor's structure.
 *
 * `pw_struct_subtype` creates subtype with additional data.
 *
 * Variadic arguments are interfaces to override.
 * They are pairs of interface id and interface pointer
 * terminated by -1 (`pw_subtype` wrapper does that by default).
 * Interfaces contain only methods to override, methods that do not need to be overriden
 * should be null pointers.
 *
 * The function initializes `type` with ancestor's type, calculates data_offset,
 * sets data_size and other essential fields, and then adds `type`
 * to the global list.
 *
 * The caller may alter basic methods after calling this function.
 *
 * Return new type id.
 *
 * All errors in this function are considered as critical and cause program abort.
 */

#define pw_get_type_name(v) _Generic((v),        \
                  char: _pw_get_type_name_by_id, \
         unsigned char: _pw_get_type_name_by_id, \
                 short: _pw_get_type_name_by_id, \
        unsigned short: _pw_get_type_name_by_id, \
                   int: _pw_get_type_name_by_id, \
          unsigned int: _pw_get_type_name_by_id, \
                  long: _pw_get_type_name_by_id, \
         unsigned long: _pw_get_type_name_by_id, \
             long long: _pw_get_type_name_by_id, \
    unsigned long long: _pw_get_type_name_by_id, \
            PwValuePtr: _pw_get_type_name_from_value  \
    )(v)

static inline char* _pw_get_type_name_by_id     (uint8_t type_id)  { return _pw_types[type_id]->name; }
static inline char* _pw_get_type_name_from_value(PwValuePtr value) { return _pw_types[value->type_id]->name; }

void pw_dump_types(FILE* fp);

/****************************************************************
 * Constructors
 */

#define pw_create(type_id)              _pw_types[type_id]->create((type_id), nullptr)
#define pw_create2(type_id, ctor_args)  _pw_types[type_id]->create((type_id), (ctor_args))
/*
 * Basic constructors.
 */

/*
 * In-place declarations and rvalues
 *
 * __PWDECL_* macros define values that are not automatically cleaned. Use carefully.
 *
 * PWDECL_* macros define automatically cleaned values. Okay to use for local variables.
 *
 * Pw<Typename> macros define rvalues that should be destroyed either explicitly
 * or automatically, by assigning them to an automatically cleaned variable
 * or passing to PwArray(), PwMap() or other pw_*_va function that takes care of its arguments.
 */

#define __PWDECL_Null(name)  \
    /* declare Null variable */  \
    _PwValue name = {  \
        .type_id = PwTypeId_Null  \
    }

#define PWDECL_Null(name)  _PW_VALUE_CLEANUP __PWDECL_Null(name)

#define PwNull()  \
    /* make Null rvalue */  \
    ({  \
        __PWDECL_Null(v);  \
        v;  \
    })

#define __PWDECL_Bool(name, initializer)  \
    /* declare Bool variable */  \
    _PwValue name = {  \
        ._integral_type_id = PwTypeId_Bool,  \
        .bool_value = (initializer)  \
    }

#define PWDECL_Bool(name, initializer)  _PW_VALUE_CLEANUP __PWDECL_Bool((name), (initializer))

#define PwBool(initializer)  \
    /* make Bool rvalue */  \
    ({  \
        __PWDECL_Bool(v, (initializer));  \
        v;  \
    })

#define __PWDECL_Signed(name, initializer)  \
    /* declare Signed variable */  \
    _PwValue name = {  \
        ._integral_type_id = PwTypeId_Signed,  \
        .signed_value = (initializer),  \
    }

#define PWDECL_Signed(name, initializer)  _PW_VALUE_CLEANUP __PWDECL_Signed((name), (initializer))

#define PwSigned(initializer)  \
    /* make Signed rvalue */  \
    ({  \
        __PWDECL_Signed(v, (initializer));  \
        v;  \
    })

#define __PWDECL_Unsigned(name, initializer)  \
    /* declare Unsigned variable */  \
    _PwValue name = {  \
        ._integral_type_id = PwTypeId_Unsigned,  \
        .unsigned_value = (initializer)  \
    }

#define PWDECL_Unsigned(name, initializer)  _PW_VALUE_CLEANUP __PWDECL_Unsigned((name), (initializer))

#define PwUnsigned(initializer)  \
    /* make Unsigned rvalue */  \
    ({  \
        __PWDECL_Unsigned(v, (initializer));  \
        v;  \
    })

#define __PWDECL_Float(name, initializer)  \
    /* declare Float variable */  \
    _PwValue name = {  \
        ._integral_type_id = PwTypeId_Float,  \
        .float_value = (initializer)  \
    }

#define PWDECL_Float(name, initializer)  _PW_VALUE_CLEANUP __PWDECL_Float((name), (initializer))

#define PwFloat(initializer)  \
    /* make Float rvalue */  \
    ({  \
        __PWDECL_Float(v, (initializer));  \
        v;  \
    })

// The very basic string declaration and rvalue, see pw_string.h for more macros:

#define __PWDECL_String(name)  \
    /* declare empty String variable */  \
    _PwValue name = {  \
        ._emb_string_type_id = PwTypeId_String,  \
        .str_embedded = 1  \
    }

#define PWDECL_String(name)  _PW_VALUE_CLEANUP __PWDECL_String(name)

#define PwString()  \
    /* make empty String rvalue */  \
    ({  \
        __PWDECL_String(v);  \
        v;  \
    })

#define __PWDECL_DateTime(name)  \
    /* declare DateTime variable */  \
    _PwValue name = {  \
        ._datetime_type_id = PwTypeId_DateTime  \
    }

#define PWDECL_DateTime(name)  _PW_VALUE_CLEANUP __PWDECL_DateTime((name))

#define PwDateTime()  \
    /* make DateTime rvalue */  \
    ({  \
        __PWDECL_DateTime(v);  \
        v;  \
    })

#define __PWDECL_Timestamp(name)  \
    /* declare Timestamp variable */  \
    _PwValue name = {  \
        ._timestamp_type_id = PwTypeId_Timestamp  \
    }

#define PWDECL_Timestamp(name)  _PW_VALUE_CLEANUP __PWDECL_Timestamp((name))

#define PwTimestamp()  \
    /* make Timestamp rvalue */  \
    ({  \
        __PWDECL_Timestamp(v);  \
        v;  \
    })

#define __PWDECL_CharPtr(name, initializer)  \
    /* declare CharPtr variable */  \
    _PwValue name = {  \
        ._charptr_type_id = PwTypeId_CharPtr,  \
        .charptr_subtype = PW_CHARPTR,  \
        .charptr = (char8_t*) (initializer)  \
    }

#define PWDECL_CharPtr(name, initializer)  _PW_VALUE_CLEANUP __PWDECL_CharPtr((name), (initializer))

#define PwCharPtr(initializer)  \
    /* make CharPtr rvalue */  \
    ({  \
        __PWDECL_CharPtr(v, (initializer));  \
        v;  \
    })

#define __PWDECL_Char32Ptr(name, initializer)  \
    /* declare Char32Ptr variable */  \
    _PwValue name = {  \
        ._charptr_type_id = PwTypeId_CharPtr,  \
        .charptr_subtype = PW_CHAR32PTR,  \
        .char32ptr = (initializer)  \
    }

#define PWDECL_Char32Ptr(name, initializer)  _PW_VALUE_CLEANUP __PWDECL_Char32Ptr((name), (initializer))

#define PwChar32Ptr(initializer)  \
    /* make Char32Ptr rvalue */  \
    ({  \
        __PWDECL_Char32Ptr(v, (initializer));  \
        v;  \
    })

#define __PWDECL_Ptr(name, initializer)  \
    /* declare Ptr variable */  \
    _PwValue name = {  \
        ._charptr_type_id = PwTypeId_Ptr,  \
        .ptr = (initializer)  \
    }

#define PWDECL_Ptr(name, initializer)  _PW_VALUE_CLEANUP __PWDECL_Ptr((name), (initializer))

#define PwPtr(initializer)  \
    /* make Ptr rvalue */  \
    ({  \
        __PWDECL_Ptr(v, (initializer));  \
        v;  \
    })


// Status declarations and rvalues

#define __PWDECL_Status(name, _status_code)  \
    /* declare Status variable */  \
    _PwValue name = {  \
        ._status_type_id = PwTypeId_Status,  \
        .status_code = _status_code,  \
        .line_number = __LINE__, \
        .file_name = __FILE__ \
    }

#define PWDECL_Status(name, _status_code)  \
    _PW_VALUE_CLEANUP __PWDECL_Status((name), (_status_code))

#define PwStatus(_status_code)  \
    /* make Status rvalue */  \
    ({  \
        __PWDECL_Status(status, (_status_code));  \
        status;  \
    })

#define PwOK()  \
    /* make success rvalue */  \
    ({  \
        _PwValue status = {  \
            ._status_type_id = PwTypeId_Status,  \
            .is_error = 0  \
        };  \
        status;  \
    })

#define PwError(code)  \
    /* make Status rvalue */  \
    ({  \
        __PWDECL_Status(status, (code));  \
        status;  \
    })

#define PwOOM()  \
    /* make PW_ERROR_OOM rvalue */  \
    ({  \
        __PWDECL_Status(status, PW_ERROR_OOM);  \
        status;  \
    })

#define PwVaEnd()  \
    /* make VA_END rvalue */  \
    ({  \
        _PwValue status = {  \
            ._status_type_id = PwTypeId_Status,  \
            .status_code = PW_STATUS_VA_END  \
        };  \
        status;  \
    })

#define __PWDECL_Errno(name, _errno)  \
    /* declare errno Status variable */  \
    _PwValue name = {  \
        ._status_type_id = PwTypeId_Status,  \
        .status_code = PW_ERROR_ERRNO,  \
        .pw_errno = _errno,  \
        .line_number = __LINE__, \
        .file_name = __FILE__ \
    }

#define PWDECL_Errno(name, _errno)  _PW_VALUE_CLEANUP __PWDECL_Errno((name), (_errno))

#define PwErrno(_errno)  \
    /* make errno Status rvalue */  \
    ({  \
        __PWDECL_Errno(status, _errno);  \
        status;  \
    })

/****************************************************************
 * Basic methods
 *
 * Most of them are inline wrappers around method invocation
 * macros.
 *
 * Using inline functions to avoid multiple evaluation of args
 * when, say, pw_destroy(vptr++) is called.
 */

static inline void pw_destroy(PwValuePtr value)
/*
 * Destroy value: call destructor and make `value` Null.
 */
{
    if (value->type_id != PwTypeId_Null) {
        PwMethodDestroy fn = pw_typeof(value)->destroy;
        if (fn) {
            fn(value);
        }
        value->type_id = PwTypeId_Null;
    }
}

static inline PwResult pw_clone(PwValuePtr value)
/*
 * Clone value.
 */
{
    PwMethodClone fn = pw_typeof(value)->clone;
    if (fn) {
        return fn(value);
    } else {
        return *value;
    }
}

static inline PwResult pw_move(PwValuePtr v)
/*
 * "Move" value to another variable or out of the function
 * (i.e. return a value and reset autocleaned variable)
 */
{
    _PwValue tmp = *v;
    v->type_id = PwTypeId_Null;
    return tmp;
}

static inline PwResult pw_deepcopy(PwValuePtr value)
{
    PwMethodDeepCopy fn = pw_typeof(value)->deepcopy;
    if (fn) {
        return fn(value);
    } else {
        return *value;
    }
}

static inline bool pw_is_true(PwValuePtr value)
{
    return pw_typeof(value)->is_true(value);
}

static inline PwResult pw_to_string(PwValuePtr value)
{
    return pw_typeof(value)->to_string(value);
}

/****************************************************************
 * API for struct types
 */

static inline void* _pw_get_data_ptr(PwValuePtr v, PwTypeId type_id)
/*
 * Helper function to get pointer to struct data.
 * Typically used in macros like this:
 *
 * #define get_data_ptr(value)  ((MyType*) _pw_get_data_ptr((value), PwTypeId_MyType))
 */
{
    if (v->struct_data) {
        PwType* t = _pw_types[type_id];
        return (void*) (
            ((uint8_t*) v->struct_data) + t->data_offset
        );
    } else {
        return nullptr;
    }
}

/****************************************************************
 * API for compound types
 */

bool _pw_adopt(PwValuePtr parent, PwValuePtr child);
/*
 * Add parent to child's parents or increment
 * parents_refcount if added already.
 *
 * Decrement child refcount.
 *
 * Return false if OOM.
 */

bool _pw_abandon(PwValuePtr parent, PwValuePtr child);
/*
 * Decrement parents_refcount in child's list of parents and when it reaches zero
 * remove parent from child's parents and return true.
 *
 * If child still refers to parent, return false.
 */

bool _pw_is_embraced(PwValuePtr value);
/*
 * Return true if value is embraced by some parent.
 */

bool _pw_need_break_cyclic_refs(PwValuePtr value);
/*
 * Check if all parents have zero refcount and there are cyclic references.
 */

static inline bool _pw_embrace(PwValuePtr parent, PwValuePtr child)
{
    if (pw_is_compound(child)) {
        return _pw_adopt(parent, child);
    } else {
        return true;
    }
}

PwValuePtr _pw_on_chain(PwValuePtr value, _PwCompoundChain* tail);
/*
 * Check if value struct_data is on chain.
 */

/****************************************************************
 * Compare for equality.
 */

static inline bool _pw_equal(PwValuePtr a, PwValuePtr b)
{
    if (a == b) {
        // compare with self
        return true;
    }
    if (a->u64[0] == b->u64[0] && a->u64[1] == b->u64[1]) {
        // quick comparison
        return true;
    }
    PwType* t = pw_typeof(a);
    PwMethodEqual cmp;
    if (a->type_id == b->type_id) {
        cmp = t->equal_sametype;
    } else {
        cmp = t->equal;
    }
    return cmp(a, b);
}

#define pw_equal(a, b) _Generic((b),           \
             nullptr_t: _pwc_equal_null,       \
                  bool: _pwc_equal_bool,       \
                  char: _pwc_equal_char,       \
         unsigned char: _pwc_equal_uchar,      \
                 short: _pwc_equal_short,      \
        unsigned short: _pwc_equal_ushort,     \
                   int: _pwc_equal_int,        \
          unsigned int: _pwc_equal_uint,       \
                  long: _pwc_equal_long,       \
         unsigned long: _pwc_equal_ulong,      \
             long long: _pwc_equal_longlong,   \
    unsigned long long: _pwc_equal_ulonglong,  \
                 float: _pwc_equal_float,      \
                double: _pwc_equal_double,     \
                 char*: _pwc_equal_u8_wrapper, \
              char8_t*: _pwc_equal_u8,         \
             char32_t*: _pwc_equal_u32,        \
            PwValuePtr: _pw_equal              \
    )((a), (b))
/*
 * Type-generic compare for equality.
 */

static inline bool _pwc_equal_null      (PwValuePtr a, nullptr_t          b) { return pw_is_null(a); }
static inline bool _pwc_equal_bool      (PwValuePtr a, bool               b) { __PWDECL_Bool     (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_char      (PwValuePtr a, char               b) { __PWDECL_Signed   (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_uchar     (PwValuePtr a, unsigned char      b) { __PWDECL_Unsigned (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_short     (PwValuePtr a, short              b) { __PWDECL_Signed   (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_ushort    (PwValuePtr a, unsigned short     b) { __PWDECL_Unsigned (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_int       (PwValuePtr a, int                b) { __PWDECL_Signed   (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_uint      (PwValuePtr a, unsigned int       b) { __PWDECL_Unsigned (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_long      (PwValuePtr a, long               b) { __PWDECL_Signed   (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_ulong     (PwValuePtr a, unsigned long      b) { __PWDECL_Unsigned (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_longlong  (PwValuePtr a, long long          b) { __PWDECL_Signed   (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_ulonglong (PwValuePtr a, unsigned long long b) { __PWDECL_Unsigned (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_float     (PwValuePtr a, float              b) { __PWDECL_Float    (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_double    (PwValuePtr a, double             b) { __PWDECL_Float    (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_u8        (PwValuePtr a, char8_t*           b) { __PWDECL_CharPtr  (v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_u32       (PwValuePtr a, char32_t*          b) { __PWDECL_Char32Ptr(v, b); return _pw_equal(a, &v); }
static inline bool _pwc_equal_u8_wrapper(PwValuePtr a, char*              b) { return _pwc_equal_u8(a, (char8_t*) b); }

#ifdef __cplusplus
}
#endif
