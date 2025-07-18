#pragma once

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 * Constructors
 */

[[nodiscard]] static inline bool pw_create_map(PwValuePtr result)
{
    return pw_create(PwTypeId_Map, result);
}

#define pw_map_va(result, ...)  \
    _pw_map_va((result), __VA_ARGS__  __VA_OPT__(,) PwVaEnd())

#define pwva_map(...) \
    ({  \
        _PwValue result = PW_NULL;  \
        if (!_pw_map_va(&result, __VA_ARGS__  __VA_OPT__(,) PwVaEnd())) {  \
            pw_clone2(&current_task->status, &result);  \
        }  \
        result;  \
    })

[[nodiscard]] bool _pw_map_va(PwValuePtr result, ...);
/*
 * Variadic constructor arguments are key-value pairs.
 */

/****************************************************************
 * Update map: insert key-value pair or replace existing value.
 */

[[nodiscard]] bool pw_map_update(PwValuePtr map, PwValuePtr key, PwValuePtr value);
/*
 * `key` is deeply copied and `value` is cloned before adding.
 */

[[nodiscard]] bool _pw_map_update_va(PwValuePtr map, ...);
/*
 * Variadic functions accept values, not pointers.
 * This encourages use cases when values are created during the call.
 * If an error is occured, a Status value is pushed on stack.
 * As long as statuses are prohibited, the function returns the first
 * status encountered and destroys all passed arguments.
 *
 * CAVEAT: DO NOT PASS LOCAL VARIABLES BY VALUES!
 */

#define pw_map_update_va(map, ...)  \
    _pw_map_update_va((map) __VA_OPT__(,) __VA_ARGS__, PwVaEnd())

[[nodiscard]] bool pw_map_update_ap(PwValuePtr map, va_list ap);

/****************************************************************
 * Check `key` is in `map`.
 */

#define pw_map_has_key(map, key) _Generic((key),    \
             nullptr_t: _pw_map_has_key_null,       \
                  bool: _pw_map_has_key_bool,       \
                  char: _pw_map_has_key_signed,    \
         unsigned char: _pw_map_has_key_unsigned,  \
                 short: _pw_map_has_key_signed,    \
        unsigned short: _pw_map_has_key_unsigned,  \
                   int: _pw_map_has_key_signed,    \
          unsigned int: _pw_map_has_key_unsigned,  \
                  long: _pw_map_has_key_signed,    \
         unsigned long: _pw_map_has_key_unsigned,  \
             long long: _pw_map_has_key_signed,    \
    unsigned long long: _pw_map_has_key_unsigned,  \
                 float: _pw_map_has_key_float,     \
                double: _pw_map_has_key_float,     \
                 char*: _pw_map_has_key_ascii,     \
              char8_t*: _pw_map_has_key_utf8,      \
             char32_t*: _pw_map_has_key_utf32,     \
            PwValuePtr: _pw_map_has_key            \
    )((map), (key))

bool _pw_map_has_key(PwValuePtr map, PwValuePtr key);

[[nodiscard]] static inline bool _pw_map_has_key_null    (PwValuePtr map, PwType_Null     key) { _PwValue v = PW_NULL;            return _pw_map_has_key(map, &v); }
[[nodiscard]] static inline bool _pw_map_has_key_bool    (PwValuePtr map, PwType_Bool     key) { _PwValue v = PW_BOOL(key);       return _pw_map_has_key(map, &v); }
[[nodiscard]] static inline bool _pw_map_has_key_signed  (PwValuePtr map, PwType_Signed   key) { _PwValue v = PW_SIGNED(key);     return _pw_map_has_key(map, &v); }
[[nodiscard]] static inline bool _pw_map_has_key_unsigned(PwValuePtr map, PwType_Unsigned key) { _PwValue v = PW_UNSIGNED(key);   return _pw_map_has_key(map, &v); }
[[nodiscard]] static inline bool _pw_map_has_key_float   (PwValuePtr map, PwType_Float    key) { _PwValue v = PW_FLOAT(key);      return _pw_map_has_key(map, &v); }
[[nodiscard]] static inline bool _pw_map_has_key_ascii   (PwValuePtr map, char*           key) {  PwValue v = PwStringAscii(key); return _pw_map_has_key(map, &v); }
[[nodiscard]] static inline bool _pw_map_has_key_utf8    (PwValuePtr map, char8_t*        key) {  PwValue v = PwStringUtf8(key);  return _pw_map_has_key(map, &v); }
[[nodiscard]] static inline bool _pw_map_has_key_utf32   (PwValuePtr map, char32_t*       key) {  PwValue v = PwStringUtf32(key); return _pw_map_has_key(map, &v); }


/****************************************************************
 * Get value by `key`. Return PW_ERROR_NO_KEY if `key`
 * is not in the `map`.
 */

#define pw_map_get(map, key, result) _Generic((key),   \
             nullptr_t: _pw_map_get_null,      \
                  bool: _pw_map_get_bool,      \
                  char: _pw_map_get_signed,    \
         unsigned char: _pw_map_get_unsigned,  \
                 short: _pw_map_get_signed,    \
        unsigned short: _pw_map_get_unsigned,  \
                   int: _pw_map_get_signed,    \
          unsigned int: _pw_map_get_unsigned,  \
                  long: _pw_map_get_signed,    \
         unsigned long: _pw_map_get_unsigned,  \
             long long: _pw_map_get_signed,    \
    unsigned long long: _pw_map_get_unsigned,  \
                 float: _pw_map_get_float,     \
                double: _pw_map_get_float,     \
                 char*: _pw_map_get_ascii,     \
              char8_t*: _pw_map_get_utf8,      \
             char32_t*: _pw_map_get_utf32,     \
            PwValuePtr: _pw_map_get            \
    )((map), (key), (result))

[[nodiscard]] bool _pw_map_get(PwValuePtr map, PwValuePtr key, PwValuePtr result);

[[nodiscard]] static inline bool _pw_map_get_null    (PwValuePtr map, PwType_Null     key, PwValuePtr result) { _PwValue v = PW_NULL;            return _pw_map_get(map, &v, result); }
[[nodiscard]] static inline bool _pw_map_get_bool    (PwValuePtr map, PwType_Bool     key, PwValuePtr result) { _PwValue v = PW_BOOL(key);       return _pw_map_get(map, &v, result); }
[[nodiscard]] static inline bool _pw_map_get_signed  (PwValuePtr map, PwType_Signed   key, PwValuePtr result) { _PwValue v = PW_SIGNED(key);     return _pw_map_get(map, &v, result); }
[[nodiscard]] static inline bool _pw_map_get_unsigned(PwValuePtr map, PwType_Unsigned key, PwValuePtr result) { _PwValue v = PW_UNSIGNED(key);   return _pw_map_get(map, &v, result); }
[[nodiscard]] static inline bool _pw_map_get_float   (PwValuePtr map, PwType_Float    key, PwValuePtr result) { _PwValue v = PW_FLOAT(key);      return _pw_map_get(map, &v, result); }
[[nodiscard]] static inline bool _pw_map_get_ascii   (PwValuePtr map, char*           key, PwValuePtr result) {  PwValue v = PwStringAscii(key); return _pw_map_get(map, &v, result); }
[[nodiscard]] static inline bool _pw_map_get_utf8    (PwValuePtr map, char8_t*        key, PwValuePtr result) {  PwValue v = PwStringUtf8(key);  return _pw_map_get(map, &v, result); }
[[nodiscard]] static inline bool _pw_map_get_utf32   (PwValuePtr map, char32_t*       key, PwValuePtr result) {  PwValue v = PwStringUtf32(key); return _pw_map_get(map, &v, result); }


/****************************************************************
 * Delete item from map by `key`.
 *
 * Return true if value was deleted, false if no value was identified by key.
 */

#define pw_map_del(map, key) _Generic((key),   \
             nullptr_t: _pw_map_del_null,      \
                  bool: _pw_map_del_bool,      \
                  char: _pw_map_del_signed,    \
         unsigned char: _pw_map_del_unsigned,  \
                 short: _pw_map_del_signed,    \
        unsigned short: _pw_map_del_unsigned,  \
                   int: _pw_map_del_signed,    \
          unsigned int: _pw_map_del_unsigned,  \
                  long: _pw_map_del_signed,    \
         unsigned long: _pw_map_del_unsigned,  \
             long long: _pw_map_del_signed,    \
    unsigned long long: _pw_map_del_unsigned,  \
                 float: _pw_map_del_float,     \
                double: _pw_map_del_float,     \
                 char*: _pw_map_del_ascii,     \
              char8_t*: _pw_map_del_utf8,      \
             char32_t*: _pw_map_del_utf32,     \
            PwValuePtr: _pw_map_del            \
    )((map), (key))

[[nodiscard]] bool _pw_map_del(PwValuePtr map, PwValuePtr key);

[[nodiscard]] static inline bool _pw_map_del_null    (PwValuePtr map, PwType_Null     key) { _PwValue v = PW_NULL;            return _pw_map_del(map, &v); }
[[nodiscard]] static inline bool _pw_map_del_bool    (PwValuePtr map, PwType_Bool     key) { _PwValue v = PW_BOOL(key);       return _pw_map_del(map, &v); }
[[nodiscard]] static inline bool _pw_map_del_signed  (PwValuePtr map, PwType_Signed   key) { _PwValue v = PW_SIGNED(key);     return _pw_map_del(map, &v); }
[[nodiscard]] static inline bool _pw_map_del_unsigned(PwValuePtr map, PwType_Unsigned key) { _PwValue v = PW_UNSIGNED(key);   return _pw_map_del(map, &v); }
[[nodiscard]] static inline bool _pw_map_del_float   (PwValuePtr map, PwType_Float    key) { _PwValue v = PW_FLOAT(key);      return _pw_map_del(map, &v); }
[[nodiscard]] static inline bool _pw_map_del_ascii   (PwValuePtr map, char*           key) {  PwValue v = PwStringAscii(key); return _pw_map_del(map, &v); }
[[nodiscard]] static inline bool _pw_map_del_utf8    (PwValuePtr map, char8_t*        key) {  PwValue v = PwStringUtf8(key);  return _pw_map_del(map, &v); }
[[nodiscard]] static inline bool _pw_map_del_utf32   (PwValuePtr map, char32_t*       key) {  PwValue v = PwStringUtf32(key); return _pw_map_del(map, &v); }


/****************************************************************
 * Misc. functions.
 */

unsigned pw_map_length(PwValuePtr map);
/*
 * Return the number of items in `map`.
 */

[[nodiscard]] bool pw_map_item(PwValuePtr map, unsigned index, PwValuePtr key, PwValuePtr value);
/*
 * Get key-value pair from the map.
 * If `index` is valid return true and write result to key and value.
 *
 * Example:
 *
 * PwValue key = PW_NULL;
 * PwValue value = PW_NULL;
 * if (pw_map_item(map, i, &key, &value)) {
 *     // success!
 * }
 */

#ifdef __cplusplus
}
#endif
