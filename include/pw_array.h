#pragma once

#include <stdarg.h>
#include <uchar.h>

#include <pw_iterator.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PwArray(...)  _pw_array_create(__VA_ARGS__ __VA_OPT__(,) PwVaEnd())
extern PwResult _pw_array_create(...);
/*
 * Array constructor arguments are array items.
 */

/****************************************************************
 * Append and insert functions
 */

#define pw_array_append(array, item) _Generic((item), \
             nullptr_t: _pw_array_append_null,       \
                  bool: _pw_array_append_bool,       \
                  char: _pw_array_append_signed,     \
         unsigned char: _pw_array_append_unsigned,   \
                 short: _pw_array_append_signed,     \
        unsigned short: _pw_array_append_unsigned,   \
                   int: _pw_array_append_signed,     \
          unsigned int: _pw_array_append_unsigned,   \
                  long: _pw_array_append_signed,     \
         unsigned long: _pw_array_append_unsigned,   \
             long long: _pw_array_append_signed,     \
    unsigned long long: _pw_array_append_unsigned,   \
                 float: _pw_array_append_float,      \
                double: _pw_array_append_float,      \
                 char*: _pw_array_append_u8_wrapper, \
              char8_t*: _pw_array_append_u8,         \
             char32_t*: _pw_array_append_u32,        \
            PwValuePtr: _pw_array_append             \
    )((array), (item))

PwResult _pw_array_append(PwValuePtr array, PwValuePtr item);
/*
 * The basic append function.
 *
 * `item` is cloned before appending. CharPtr values are converted to PW strings.
 */

static inline PwResult _pw_array_append_null    (PwValuePtr array, PwType_Null     item) { __PWDECL_Null     (v);       return _pw_array_append(array, &v); }
static inline PwResult _pw_array_append_bool    (PwValuePtr array, PwType_Bool     item) { __PWDECL_Bool     (v, item); return _pw_array_append(array, &v); }
static inline PwResult _pw_array_append_signed  (PwValuePtr array, PwType_Signed   item) { __PWDECL_Signed   (v, item); return _pw_array_append(array, &v); }
static inline PwResult _pw_array_append_unsigned(PwValuePtr array, PwType_Unsigned item) { __PWDECL_Unsigned (v, item); return _pw_array_append(array, &v); }
static inline PwResult _pw_array_append_float   (PwValuePtr array, PwType_Float    item) { __PWDECL_Float    (v, item); return _pw_array_append(array, &v); }
static inline PwResult _pw_array_append_u8      (PwValuePtr array, char8_t*        item) { __PWDECL_CharPtr  (v, item); return _pw_array_append(array, &v); }
static inline PwResult _pw_array_append_u32     (PwValuePtr array, char32_t*       item) { __PWDECL_Char32Ptr(v, item); return _pw_array_append(array, &v); }

static inline PwResult _pw_array_append_u8_wrapper(PwValuePtr array, char* item)
{
    return _pw_array_append_u8(array, (char8_t*) item);
}

PwResult _pw_array_append_va(PwValuePtr array, ...);
/*
 * Variadic functions accept values, not pointers.
 * This encourages use cases when values are created during the call.
 * If an error is occured, a Status value is pushed on stack.
 * As long as statuses are prohibited, the function returns the first
 * status encountered and destroys all passed arguments.
 *
 * CAVEAT: DO NOT PASS LOCAL VARIABLES BY VALUES!
 */

#define pw_array_append_va(array, ...)  \
    _pw_array_append_va(array __VA_OPT__(,) __VA_ARGS__, PwVaEnd())

PwResult pw_array_append_ap(PwValuePtr array, va_list ap);
/*
 * Append items to the `array`.
 * Item are cloned before appending. CharPtr values are converted to PW strings.
 */

#define pw_array_insert(array, index, item) _Generic((item), \
             nullptr_t: _pw_array_insert_null,       \
                  bool: _pw_array_insert_bool,       \
                  char: _pw_array_insert_signed,     \
         unsigned char: _pw_array_insert_unsigned,   \
                 short: _pw_array_insert_signed,     \
        unsigned short: _pw_array_insert_unsigned,   \
                   int: _pw_array_insert_signed,     \
          unsigned int: _pw_array_insert_unsigned,   \
                  long: _pw_array_insert_signed,     \
         unsigned long: _pw_array_insert_unsigned,   \
             long long: _pw_array_insert_signed,     \
    unsigned long long: _pw_array_insert_unsigned,   \
                 float: _pw_array_insert_float,      \
                double: _pw_array_insert_float,      \
                 char*: _pw_array_insert_u8_wrapper, \
              char8_t*: _pw_array_insert_u8,         \
             char32_t*: _pw_array_insert_u32,        \
            PwValuePtr: _pw_array_insert             \
    )((array), (index), (item))

PwResult _pw_array_insert(PwValuePtr array, unsigned index, PwValuePtr item);
/*
 * The basic insert function.
 *
 * `item` is cloned before inserting. CharPtr values are converted to PW strings.
 */

static inline PwResult _pw_array_insert_null    (PwValuePtr array, unsigned index, PwType_Null     item) { __PWDECL_Null     (v);       return _pw_array_insert(array, index, &v); }
static inline PwResult _pw_array_insert_bool    (PwValuePtr array, unsigned index, PwType_Bool     item) { __PWDECL_Bool     (v, item); return _pw_array_insert(array, index, &v); }
static inline PwResult _pw_array_insert_signed  (PwValuePtr array, unsigned index, PwType_Signed   item) { __PWDECL_Signed   (v, item); return _pw_array_insert(array, index, &v); }
static inline PwResult _pw_array_insert_unsigned(PwValuePtr array, unsigned index, PwType_Unsigned item) { __PWDECL_Unsigned (v, item); return _pw_array_insert(array, index, &v); }
static inline PwResult _pw_array_insert_float   (PwValuePtr array, unsigned index, PwType_Float    item) { __PWDECL_Float    (v, item); return _pw_array_insert(array, index, &v); }
static inline PwResult _pw_array_insert_u8      (PwValuePtr array, unsigned index, char8_t*        item) { __PWDECL_CharPtr  (v, item); return _pw_array_insert(array, index, &v); }
static inline PwResult _pw_array_insert_u32     (PwValuePtr array, unsigned index, char32_t*       item) { __PWDECL_Char32Ptr(v, item); return _pw_array_insert(array, index, &v); }

static inline PwResult _pw_array_insert_u8_wrapper(PwValuePtr array, unsigned index, char* item)
{
    return _pw_array_insert_u8(array, index, (char8_t*) item);
}


/****************************************************************
 * Join array items. Return string value.
 */

#define pw_array_join(separator, array) _Generic((separator), \
              char32_t: _pw_array_join_c32,        \
                   int: _pw_array_join_c32,        \
                 char*: _pw_array_join_u8_wrapper, \
              char8_t*: _pw_array_join_u8,         \
             char32_t*: _pw_array_join_u32,        \
            PwValuePtr: _pw_array_join             \
    )((separator), (array))

PwResult _pw_array_join_c32(char32_t   separator, PwValuePtr array);
PwResult _pw_array_join_u8 (char8_t*   separator, PwValuePtr array);
PwResult _pw_array_join_u32(char32_t*  separator, PwValuePtr array);
PwResult _pw_array_join    (PwValuePtr separator, PwValuePtr array);

static inline PwResult _pw_array_join_u8_wrapper(char* separator, PwValuePtr array)
{
    return _pw_array_join_u8((char8_t*) separator, array);
}

/****************************************************************
 * Get/set array items
 */

PwResult pw_array_pull(PwValuePtr array);
/*
 * Extract first item from the array.
 */

PwResult pw_array_pop(PwValuePtr array);
/*
 * Extract last item from the array.
 */

#define pw_array_item(array, index) _Generic((index), \
             int: _pw_array_item_signed,  \
         ssize_t: _pw_array_item_signed,  \
        unsigned: _pw_array_item  \
    )((array), (index))

PwResult _pw_array_item_signed(PwValuePtr array, ssize_t index);
PwResult _pw_array_item(PwValuePtr array, unsigned index);
/*
 * Return a clone of array item.
 * Negative indexes are allowed for signed version,
 * where -1 is the index of last item.
 */

#define pw_array_set_item(array, index, item) _Generic((index), \
             int: _pw_array_set_item_signed,  \
         ssize_t: _pw_array_set_item_signed,  \
        unsigned: _pw_array_set_item  \
    )((array), (index), (item))

PwResult _pw_array_set_item_signed(PwValuePtr array, ssize_t index, PwValuePtr item);
PwResult _pw_array_set_item(PwValuePtr array, unsigned index, PwValuePtr item);
/*
 * Set item at specific index.
 * Negative indexes are allowed for signed version,
 * where -1 is the index of last item.
 * Return PwStatus.
 */


/****************************************************************
 * Iterator
 */

extern PwTypeId PwTypeId_ArrayIterator;

static inline PwResult pw_array_iterator(PwValuePtr array)
/*
 * Return ArrayIterator that supports multiple (TBD) iteration interfaces:
 *   - LineReader
 */
{
    PwIteratorCtorArgs args = { .iterable = array };
    return pw_create2(PwTypeId_ArrayIterator, &args);
}

/****************************************************************
 * Miscellaneous array functions
 */

PwResult pw_array_resize(PwValuePtr array, unsigned desired_capacity);

unsigned pw_array_length(PwValuePtr array);

void pw_array_del(PwValuePtr array, unsigned start_index, unsigned end_index);
/*
 * Delete items from array.
 * `end_index` is exclusive, i.e. the number of items to delete equals to end_index - start_index..
 */

void pw_array_clean(PwValuePtr array);
/*
 * Delete all items from array.
 */

PwResult pw_array_slice(PwValuePtr array, unsigned start_index, unsigned end_index);
/*
 * Return shallow copy of the given range of array.
 */

PwResult pw_array_dedent(PwValuePtr lines);
/*
 * Dedent array of strings inplace.
 * XXX count treat tabs as single spaces.
 *
 * Return status.
 */

#ifdef __cplusplus
}
#endif
