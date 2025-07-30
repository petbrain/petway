#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <uchar.h>

#include <libpussy/allocator.h>

#include <pw_assert.h>
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
#define PwTypeId_String      9U
#define PwTypeId_Struct     10U  // the base for reference counted data
#define PwTypeId_Compound   11U  // the base for values that may contain circular references
#define PwTypeId_Status     12U  // value_data is optional
#define PwTypeId_Iterator   13U
#define PwTypeId_Array      14U
#define PwTypeId_Map        15U

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
    // allocated string data for fixed char size strings
    unsigned refcount;
    uint32_t capacity;  // in characters
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
        // ptr
        PwTypeId /* uint16_t */ _ptr_type_id;
        uint16_t _ptr_padding_1;
        uint32_t _ptr_pagging_2;
        // ISO C forbids conversion of object pointer to function pointer type, so define both
        union {
            void* ptr;
            void (*func_ptr)();
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
        PwTypeId /* uint16_t */ _e_string_type_id;
        uint8_t char_size: 3,
                embedded:1,
                _e_allocated:1;
        uint8_t embedded_length;
        union {
            uint8_t  str_1[12];
            uint16_t str_2[6];
            uint24_t str_3[4];
            uint32_t str_4[3];
        };
    };

    struct {
        // allocated string
        PwTypeId /* uint16_t */ _a_string_type_id;
        uint8_t _a_char_size:3,
                _a_embedded:1,
                allocated:1;
        uint8_t _a_padding;
        uint32_t length;
        _PwStringData* string_data;
    };

    struct {
        // static 0-terminated string
        PwTypeId /* uint16_t */ _s_string_type_id;
        uint8_t _s_char_size:3,
                _s_embedded:1,
                _s_allocated:1;
        uint8_t _s_padding;
        uint32_t _s_length;
        void* char_ptr;
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

// make sure _PwValue structure is correct
static_assert( offsetof(_PwValue, bool_value)  == 8 );
static_assert( offsetof(_PwValue, ptr)         == 8 );
static_assert( offsetof(_PwValue, func_ptr)    == 8 );
static_assert( offsetof(_PwValue, struct_data) == 8 );
static_assert( offsetof(_PwValue, status_data) == 8 );
static_assert( offsetof(_PwValue, string_data) == 8 );

static_assert( offsetof(_PwValue, embedded_length) == 3 );
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
typedef bool (*PwMethodCreate)  (PwTypeId type_id, void* ctor_args, PwValuePtr result);
typedef void (*PwMethodDestroy) (PwValuePtr self);
typedef void (*PwMethodClone)   (PwValuePtr self);  // this method never fails; called for assigned value to increment refcount
typedef void (*PwMethodHash)    (PwValuePtr self, PwHashContext* ctx);
typedef bool (*PwMethodDeepCopy)(PwValuePtr self, PwValuePtr result);
typedef void (*PwMethodDump)    (PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail);
typedef bool (*PwMethodToString)(PwValuePtr self, PwValuePtr result);
typedef bool (*PwMethodIsTrue)  (PwValuePtr self);
typedef bool (*PwMethodEqual)   (PwValuePtr self, PwValuePtr other);

// Function types for struct interface.
// They modify the data in place.
typedef bool (*PwMethodInit)(PwValuePtr self, void* ctor_args);
typedef void (*PwMethodFini)(PwValuePtr self);


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
    [[ gnu::warn_unused_result ]]
    PwMethodCreate   create;    // mandatory
    PwMethodDestroy  destroy;   // optional if value does not have allocated data
    PwMethodClone    clone;     // optional; if set, it is called by pw_clone()
    PwMethodHash     hash;      // mandatory
    [[ gnu::warn_unused_result ]]
    PwMethodDeepCopy deepcopy;  // optional; XXX how it should work with subtypes is not clear yet
    PwMethodDump     dump;      // mandatory
    [[ gnu::warn_unused_result ]]
    PwMethodToString to_string; // mandatory
    [[ gnu::warn_unused_result ]]
    PwMethodIsTrue   is_true;   // mandatory
    [[ gnu::warn_unused_result ]]
    PwMethodEqual    equal_sametype;  // mandatory
    [[ gnu::warn_unused_result ]]
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
    [[ gnu::warn_unused_result ]]
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

[[nodiscard]] static inline bool pw_is_subtype(PwValuePtr value, PwTypeId type_id)
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

[[nodiscard]] PwTypeId _pw_add_type(PwType* type, ...);
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

[[nodiscard]] PwTypeId _pw_subtype(PwType* type, char* name, PwTypeId ancestor_id,
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

[[nodiscard]] static inline char* _pw_get_type_name_by_id     (uint8_t type_id)  { return _pw_types[type_id]->name; }
[[nodiscard]] static inline char* _pw_get_type_name_from_value(PwValuePtr value) { return _pw_types[value->type_id]->name; }

void pw_dump_types(FILE* fp);

/****************************************************************
 * Basic constructors.
 */

#define pw_create(type_id, result)              _pw_types[type_id]->create((type_id), nullptr, (result))
#define pw_create2(type_id, ctor_args, result)  _pw_types[type_id]->create((type_id), (ctor_args), (result))


/****************************************************************
 * Initializers and rvalues
 */

#define PW_NULL {.type_id = PwTypeId_Null}

#define PwNull(initializer)  \
    /* make Bool rvalue */  \
    __extension__ \
    ({  \
        _PwValue v = PW_NULL;  \
        v;  \
    })

#define PW_BOOL(initializer)  \
    {  \
        ._integral_type_id = PwTypeId_Bool,  \
        .bool_value = (initializer)  \
    }

#define PwBool(initializer)  \
    /* make Bool rvalue */  \
    __extension__ \
    ({  \
        _PwValue v = PW_BOOL(initializer);  \
        v;  \
    })

#define PW_SIGNED(initializer)  \
    {  \
        ._integral_type_id = PwTypeId_Signed,  \
        .signed_value = (initializer),  \
    }

#define PwSigned(initializer)  \
    /* make Signed rvalue */  \
    __extension__ \
    ({  \
        _PwValue v = PW_SIGNED(initializer);  \
        v;  \
    })

#define PW_UNSIGNED(initializer)  \
    {  \
        ._integral_type_id = PwTypeId_Unsigned,  \
        .unsigned_value = (initializer)  \
    }

#define PwUnsigned(initializer)  \
    /* make Unsigned rvalue */  \
    __extension__ \
    ({  \
        _PwValue v = PW_UNSIGNED(initializer);  \
        v;  \
    })

#define PW_FLOAT(initializer)  \
    {  \
        ._integral_type_id = PwTypeId_Float,  \
        .float_value = (initializer)  \
    }

#define PwFloat(initializer)  \
    /* make Float rvalue */  \
    __extension__ \
    ({  \
        _PwValue v = PW_FLOAT(initializer);  \
        v;  \
    })

#define PW_STRING(initializer)  \
    /* Embedded string, character size 1 byte, up to 12 chars */  \
    {  \
        ._e_string_type_id = PwTypeId_String,  \
        .embedded = 1,  \
        .char_size = 1,  \
        .embedded_length = sizeof(initializer) - 1,  \
        .str_1 = initializer  \
    }

#define PwString(initializer)  \
    /* make String rvalue, character size 1 byte, up to 12 chars */  \
    __extension__ \
    ({  \
        _PwValue s = PW_STRING(initializer);  \
        s;  \
    })

#define PW_STRING_UTF32(initializer)  \
    /* Embedded string, character size 4 bytes, up to 3 chars */  \
    {  \
        ._e_string_type_id = PwTypeId_String,  \
        .embedded = 1,  \
        .char_size = 4,  \
        .embedded_length = (sizeof(initializer) - 4) / 4,  \
        .str_4 = initializer  \
    }

#define PwStringUtf32(initializer)  \
    /* make String rvalue, character size 4 bytes, up to 3 chars */  \
    __extension__ \
    ({  \
        _PwValue s = PW_STRING_UTF32(initializer);  \
        s;  \
    })


#define PW_STATIC_STRING(initializer)  \
    /* Static string, character size 1 byte */  \
    {  \
        ._s_string_type_id = PwTypeId_String,  \
        ._s_char_size = 1,  \
        ._s_length = sizeof(initializer) - 1,  \
        .char_ptr = initializer  \
    }

#define PW_STATIC_STRING_UTF32(initializer)  \
    /* Static UTF-32 string */  \
    {  \
        ._s_string_type_id = PwTypeId_String,  \
        ._s_char_size = 4,  \
        ._s_length = (((unsigned) sizeof(initializer)) - 4) / 4,  \
        .char_ptr = initializer  \
    }

#define PwStaticString(initializer) _Generic((initializer),  \
             char*: _pw_create_static_string_ascii,  \
         char32_t*: _pw_create_static_string_utf32   \
    )((initializer))
[[nodiscard]] _PwValue _pw_create_static_string_ascii(char* initializer);
[[nodiscard]] _PwValue _pw_create_static_string_utf32(char32_t* initializer);

[[nodiscard]] _PwValue PwStringUtf8 (char8_t*  initializer);

#define PW_DATETIME  \
    {  \
        ._datetime_type_id = PwTypeId_DateTime  \
    }

#define PwDateTime()  \
    /* make DateTime rvalue */  \
    __extension__ \
    ({  \
        _PwValue v = PW_DATETIME;  \
        v;  \
    })

#define PW_TIMESTAMP  \
    {  \
        ._timestamp_type_id = PwTypeId_Timestamp  \
    }

#define PwTimestamp()  \
    /* make Timestamp rvalue */  \
    __extension__ \
    ({  \
        _PwValue v = PW_TIMESTAMP;  \
        v;  \
    })

#define PW_PTR(initializer)  \
    {  \
        ._ptr_type_id = PwTypeId_Ptr,  \
        .ptr = (initializer)  \
    }

#define PwPtr(initializer)  \
    /* make Ptr rvalue */  \
    __extension__ \
    ({  \
        _PwValue v = PW_PTR(initializer);  \
        v;  \
    })

#define PwVaEnd()  \
    /* make VA_END rvalue */  \
    __extension__ \
    ({  \
        _PwValue status = {  \
            ._status_type_id = PwTypeId_Status,  \
            .status_code = PW_STATUS_VA_END  \
        };  \
        status;  \
    })

#define PW_STATUS(_status_code)  \
    {  \
        ._status_type_id = PwTypeId_Status,  \
        .status_code = _status_code,  \
        .line_number = __LINE__,  \
        .file_name = __FILE__  \
    }

#define PwStatus(_status_code)  \
    /* make Status rvalue */  \
    __extension__ \
    ({  \
        _PwValue status = PW_STATUS(_status_code);  \
        status;  \
    })

#define PwErrno(_errno)  \
    /* make errno Status rvalue */  \
    __extension__ \
    ({  \
        _PwValue status = PW_STATUS(PW_ERROR_ERRNO);  \
        status.pw_errno = _errno;  \
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

static inline void __pw_clone(PwValuePtr value, PwValuePtr result)
/*
 * Helper function for pw_clone and pw_clone2.
 */
{
    *result = *value;
    PwMethodClone fn = pw_typeof(value)->clone;
    if (fn) {
        fn(result);
    }
}

[[nodiscard]] static inline _PwValue pw_clone(PwValuePtr value)
/*
 * Single argument version, return PwValue.
 * Useful for use as initializer of as an argument for variadic functions
 * that accept PwValues (i.e. not PwValuePtr)
 * such as pw_array, pw_array_append, pw_map, pw_map_update.
 */
{
    _PwValue result = PW_NULL;
    __pw_clone(value, &result);
    return result;
}

static inline void pw_clone2(PwValuePtr value, PwValuePtr result)
/*
 * Two arguments version.
 * `result` is destroyed before cloning.
 */
{
    pw_destroy(result);
    __pw_clone(value, result);
}

static inline void pw_move(PwValuePtr v, PwValuePtr result)
/*
 * "Move" value to another variable or out of the function
 * (i.e. return a value and reset autocleaned variable)
 * `result` is destroyed before moving.
 */
{
    pw_destroy(result);
    *result = *v;
    v->type_id = PwTypeId_Null;
}

[[nodiscard]] static inline bool pw_deepcopy(PwValuePtr value, PwValuePtr result)
{
    PwMethodDeepCopy fn = pw_typeof(value)->deepcopy;
    if (fn) {
        return fn(value, result);
    } else {
        *result = *value;
        return true;
    }
}

static inline bool pw_is_true(PwValuePtr value)
{
    return pw_typeof(value)->is_true(value);
}

[[nodiscard]] static inline bool pw_to_string(PwValuePtr value, PwValuePtr result)
{
    return pw_typeof(value)->to_string(value, result);
}

/****************************************************************
 * API for struct types
 */

[[nodiscard]] static inline void* _pw_get_data_ptr(PwValuePtr v, PwTypeId type_id)
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

[[nodiscard]] bool _pw_adopt(PwValuePtr parent, PwValuePtr child);
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
 *
 * XXX return value is not used.
 */

[[nodiscard]] bool _pw_is_embraced(PwValuePtr value);
/*
 * Return true if value is embraced by some parent.
 */

[[nodiscard]] bool _pw_need_break_cyclic_refs(PwValuePtr value);
/*
 * Check if all parents have zero refcount and there are cyclic references.
 */

[[nodiscard]] static inline bool _pw_embrace(PwValuePtr parent, PwValuePtr child)
{
    if (pw_is_compound(child)) {
        return _pw_adopt(parent, child);
    } else {
        return true;
    }
}

[[nodiscard]] PwValuePtr _pw_on_chain(PwValuePtr value, _PwCompoundChain* tail);
/*
 * Check if value struct_data is on chain.
 */

/****************************************************************
 * Compare for equality.
 */

[[nodiscard]] static inline bool _pw_equal(PwValuePtr a, PwValuePtr b)
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
                 char*: _pwc_equal_ascii,      \
              char8_t*: _pwc_equal_utf8,       \
             char32_t*: _pwc_equal_utf32,      \
            PwValuePtr: _pw_equal              \
    )((a), (b))
/*
 * Type-generic compare for equality.
 */

[[nodiscard]] static inline bool _pwc_equal_null     (PwValuePtr a, nullptr_t          b) { return pw_is_null(a); }
[[nodiscard]] static inline bool _pwc_equal_bool     (PwValuePtr a, bool               b) { _PwValue v = PW_BOOL(b);        return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_char     (PwValuePtr a, char               b) { _PwValue v = PW_SIGNED(b);      return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_uchar    (PwValuePtr a, unsigned char      b) { _PwValue v = PW_UNSIGNED(b);    return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_short    (PwValuePtr a, short              b) { _PwValue v = PW_SIGNED(b);      return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_ushort   (PwValuePtr a, unsigned short     b) { _PwValue v = PW_UNSIGNED(b);    return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_int      (PwValuePtr a, int                b) { _PwValue v = PW_SIGNED(b);      return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_uint     (PwValuePtr a, unsigned int       b) { _PwValue v = PW_UNSIGNED(b);    return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_long     (PwValuePtr a, long               b) { _PwValue v = PW_SIGNED(b);      return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_ulong    (PwValuePtr a, unsigned long      b) { _PwValue v = PW_UNSIGNED(b);    return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_longlong (PwValuePtr a, long long          b) { _PwValue v = PW_SIGNED(b);      return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_ulonglong(PwValuePtr a, unsigned long long b) { _PwValue v = PW_UNSIGNED(b);    return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_float    (PwValuePtr a, float              b) { _PwValue v = PW_FLOAT(b);       return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_double   (PwValuePtr a, double             b) { _PwValue v = PW_FLOAT(b);       return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_ascii    (PwValuePtr a, char*              b) { _PwValue v = PwStaticString(b); return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_utf8     (PwValuePtr a, char8_t*           b) {  PwValue v = PwStringUtf8  (b); return _pw_equal(a, &v); }
[[nodiscard]] static inline bool _pwc_equal_utf32    (PwValuePtr a, char32_t*          b) { _PwValue v = PwStaticString(b); return _pw_equal(a, &v); }

#ifdef __cplusplus
}
#endif
