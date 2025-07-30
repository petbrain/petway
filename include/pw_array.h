#pragma once

#include <stdarg.h>
#include <uchar.h>

#include <pw_iterator.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 * Constructors
 */

[[nodiscard]] static inline bool pw_create_array(PwValuePtr result)
{
    return pw_create(PwTypeId_Array, result);
}

#define pw_array_va(result, ...)  \
    _pw_array_va((result), __VA_ARGS__ __VA_OPT__(,) PwVaEnd())

#define pwva_array(...) \
    __extension__ \
    ({  \
        _PwValue result = PW_NULL;  \
        if (!_pw_array_va(&result, __VA_ARGS__  __VA_OPT__(,) PwVaEnd())) {  \
            pw_clone2(&current_task->status, &result);  \
        }  \
        result;  \
    })

[[nodiscard]] extern bool _pw_array_va(PwValuePtr result, ...);
/*
 * Variadic array constructor arguments are array items.
 */

/****************************************************************
 * Append and insert functions
 */

#define pw_array_append(array, item) _Generic((item), \
             nullptr_t: _pw_array_append_null,      \
                  bool: _pw_array_append_bool,      \
                  char: _pw_array_append_signed,    \
         unsigned char: _pw_array_append_unsigned,  \
                 short: _pw_array_append_signed,    \
        unsigned short: _pw_array_append_unsigned,  \
                   int: _pw_array_append_signed,    \
          unsigned int: _pw_array_append_unsigned,  \
                  long: _pw_array_append_signed,    \
         unsigned long: _pw_array_append_unsigned,  \
             long long: _pw_array_append_signed,    \
    unsigned long long: _pw_array_append_unsigned,  \
                 float: _pw_array_append_float,     \
                double: _pw_array_append_float,     \
                 char*: _pw_array_append_ascii,     \
              char8_t*: _pw_array_append_utf8,      \
             char32_t*: _pw_array_append_utf32,     \
            PwValuePtr: _pw_array_append            \
    )((array), (item))

[[nodiscard]] bool _pw_array_append(PwValuePtr array, PwValuePtr item);
/*
 * The basic append function.
 *
 * `item` is cloned before appending.
 */

[[nodiscard]] static inline bool _pw_array_append_null    (PwValuePtr array, PwType_Null     item) { _PwValue v = PW_NULL;              return _pw_array_append(array, &v); }
[[nodiscard]] static inline bool _pw_array_append_bool    (PwValuePtr array, PwType_Bool     item) { _PwValue v = PW_BOOL(item);        return _pw_array_append(array, &v); }
[[nodiscard]] static inline bool _pw_array_append_signed  (PwValuePtr array, PwType_Signed   item) { _PwValue v = PW_SIGNED(item);      return _pw_array_append(array, &v); }
[[nodiscard]] static inline bool _pw_array_append_unsigned(PwValuePtr array, PwType_Unsigned item) { _PwValue v = PW_UNSIGNED(item);    return _pw_array_append(array, &v); }
[[nodiscard]] static inline bool _pw_array_append_float   (PwValuePtr array, PwType_Float    item) { _PwValue v = PW_FLOAT(item);       return _pw_array_append(array, &v); }
[[nodiscard]] static inline bool _pw_array_append_ascii   (PwValuePtr array, char*           item) { _PwValue v = PwStaticString(item); return _pw_array_append(array, &v); }
[[nodiscard]] static inline bool _pw_array_append_utf8    (PwValuePtr array, char8_t*        item) {  PwValue v = PwStringUtf8  (item); return _pw_array_append(array, &v); }
[[nodiscard]] static inline bool _pw_array_append_utf32   (PwValuePtr array, char32_t*       item) { _PwValue v = PwStaticString(item); return _pw_array_append(array, &v); }

[[nodiscard]] bool _pw_array_append_va(PwValuePtr array, ...);
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

[[nodiscard]] bool pw_array_append_ap(PwValuePtr array, va_list ap);
/*
 * Append items to the `array`.
 * Item are cloned before appending.
 */

#define pw_array_insert(array, index, item) _Generic((item), \
             nullptr_t: _pw_array_insert_null,      \
                  bool: _pw_array_insert_bool,      \
                  char: _pw_array_insert_signed,    \
         unsigned char: _pw_array_insert_unsigned,  \
                 short: _pw_array_insert_signed,    \
        unsigned short: _pw_array_insert_unsigned,  \
                   int: _pw_array_insert_signed,    \
          unsigned int: _pw_array_insert_unsigned,  \
                  long: _pw_array_insert_signed,    \
         unsigned long: _pw_array_insert_unsigned,  \
             long long: _pw_array_insert_signed,    \
    unsigned long long: _pw_array_insert_unsigned,  \
                 float: _pw_array_insert_float,     \
                double: _pw_array_insert_float,     \
                 char*: _pw_array_insert_ascii,     \
              char8_t*: _pw_array_insert_utf8,      \
             char32_t*: _pw_array_insert_utf32,     \
            PwValuePtr: _pw_array_insert            \
    )((array), (index), (item))

[[nodiscard]] bool _pw_array_insert(PwValuePtr array, unsigned index, PwValuePtr item);
/*
 * The basic insert function.
 *
 * `item` is cloned before inserting.
 */

[[nodiscard]] static inline bool _pw_array_insert_null    (PwValuePtr array, unsigned index, PwType_Null     item) { _PwValue v = PW_NULL;              return _pw_array_insert(array, index, &v); }
[[nodiscard]] static inline bool _pw_array_insert_bool    (PwValuePtr array, unsigned index, PwType_Bool     item) { _PwValue v = PW_BOOL(item);        return _pw_array_insert(array, index, &v); }
[[nodiscard]] static inline bool _pw_array_insert_signed  (PwValuePtr array, unsigned index, PwType_Signed   item) { _PwValue v = PW_SIGNED(item);      return _pw_array_insert(array, index, &v); }
[[nodiscard]] static inline bool _pw_array_insert_unsigned(PwValuePtr array, unsigned index, PwType_Unsigned item) { _PwValue v = PW_UNSIGNED(item);    return _pw_array_insert(array, index, &v); }
[[nodiscard]] static inline bool _pw_array_insert_float   (PwValuePtr array, unsigned index, PwType_Float    item) { _PwValue v = PW_FLOAT(item);       return _pw_array_insert(array, index, &v); }
[[nodiscard]] static inline bool _pw_array_insert_ascii   (PwValuePtr array, unsigned index, char*           item) { _PwValue v = PwStaticString(item); return _pw_array_insert(array, index, &v); }
[[nodiscard]] static inline bool _pw_array_insert_utf8    (PwValuePtr array, unsigned index, char8_t*        item) {  PwValue v = PwStringUtf8  (item); return _pw_array_insert(array, index, &v); }
[[nodiscard]] static inline bool _pw_array_insert_utf32   (PwValuePtr array, unsigned index, char32_t*       item) { _PwValue v = PwStaticString(item); return _pw_array_insert(array, index, &v); }


/****************************************************************
 * Join array items. Return string value.
 */

#define pw_array_join(separator, array, result) _Generic((separator), \
              char32_t: _pw_array_join_c32,    \
                   int: _pw_array_join_c32,    \
                 char*: _pw_array_join_ascii,  \
              char8_t*: _pw_array_join_utf8,   \
             char32_t*: _pw_array_join_utf32,  \
            PwValuePtr: _pw_array_join         \
    )((separator), (array), (result))

[[nodiscard]] bool _pw_array_join      (PwValuePtr separator, PwValuePtr array, PwValuePtr result);
[[nodiscard]] bool _pw_array_join_c32  (char32_t   separator, PwValuePtr array, PwValuePtr result);
[[nodiscard]] bool _pw_array_join_ascii(char*      separator, PwValuePtr array, PwValuePtr result);
[[nodiscard]] bool _pw_array_join_utf8 (char8_t*   separator, PwValuePtr array, PwValuePtr result);
[[nodiscard]] bool _pw_array_join_utf32(char32_t*  separator, PwValuePtr array, PwValuePtr result);


/****************************************************************
 * Get/set array items
 */

[[nodiscard]] bool pw_array_pull(PwValuePtr array, PwValuePtr result);
/*
 * Extract first item from the array.
 */

[[nodiscard]] bool pw_array_pop(PwValuePtr array, PwValuePtr result);
/*
 * Extract last item from the array.
 */

#define pw_array_item(array, index, result) _Generic((index), \
             int: _pw_array_item_signed,  \
         ssize_t: _pw_array_item_signed,  \
        unsigned: _pw_array_item  \
    )((array), (index), (result))

[[nodiscard]] bool _pw_array_item_signed(PwValuePtr array, ssize_t index, PwValuePtr result);
[[nodiscard]] bool _pw_array_item(PwValuePtr array, unsigned index, PwValuePtr result);
/*
 * Clone array item to result.
 * Negative indexes are allowed for signed version,
 * where -1 is the index of last item.
 */

#define pw_array_set_item(array, index, item) _Generic((index), \
             int: _pw_array_set_item_signed,  \
         ssize_t: _pw_array_set_item_signed,  \
        unsigned: _pw_array_set_item  \
    )((array), (index), (item))

[[nodiscard]] bool _pw_array_set_item_signed(PwValuePtr array, ssize_t index, PwValuePtr item);
[[nodiscard]] bool _pw_array_set_item(PwValuePtr array, unsigned index, PwValuePtr item);
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

[[nodiscard]] static inline bool pw_array_iterator(PwValuePtr array, PwValuePtr result)
/*
 * Return ArrayIterator that supports multiple (TBD) iteration interfaces:
 *   - LineReader
 */
{
    PwIteratorCtorArgs args = { .iterable = array };
    return pw_create2(PwTypeId_ArrayIterator, &args, result);
}

/****************************************************************
 * Miscellaneous array functions
 */

[[nodiscard]] bool pw_array_resize(PwValuePtr array, unsigned desired_capacity);

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

[[nodiscard]] bool pw_array_slice(PwValuePtr array, unsigned start_index, unsigned end_index, PwValuePtr result);
/*
 * Make shallow copy of the given range of array.
 */

[[nodiscard]] bool pw_array_dedent(PwValuePtr lines);
/*
 * Dedent array of strings inplace.
 * XXX count treat tabs as single spaces.
 *
 * Return status.
 */

#ifdef __cplusplus
}
#endif
