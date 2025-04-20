#pragma once

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PwMap(...) _pw_map_create(__VA_ARGS__  __VA_OPT__(,) PwVaEnd())
extern PwResult _pw_map_create(...);
/*
 * Map constructor arguments are key-value pairs.
 */

/****************************************************************
 * Update map: insert key-value pair or replace existing value.
 */

PwResult pw_map_update(PwValuePtr map, PwValuePtr key, PwValuePtr value);
/*
 * `key` is deeply copied and `value` is cloned before adding.
 * CharPtr values are converted to PW strings.
 */

PwResult _pw_map_update_va(PwValuePtr map, ...);
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
    _pw_map_update_va(map __VA_OPT__(,) __VA_ARGS__, PwVaEnd())

PwResult pw_map_update_ap(PwValuePtr map, va_list ap);

/****************************************************************
 * Check `key` is in `map`.
 */

#define pw_map_has_key(map, key) _Generic((key),    \
             nullptr_t: _pw_map_has_key_null,       \
                  bool: _pw_map_has_key_bool,       \
                  char: _pw_map_has_key_signed,     \
         unsigned char: _pw_map_has_key_unsigned,   \
                 short: _pw_map_has_key_signed,     \
        unsigned short: _pw_map_has_key_unsigned,   \
                   int: _pw_map_has_key_signed,     \
          unsigned int: _pw_map_has_key_unsigned,   \
                  long: _pw_map_has_key_signed,     \
         unsigned long: _pw_map_has_key_unsigned,   \
             long long: _pw_map_has_key_signed,     \
    unsigned long long: _pw_map_has_key_unsigned,   \
                 float: _pw_map_has_key_float,      \
                double: _pw_map_has_key_float,      \
                 char*: _pw_map_has_key_u8_wrapper, \
              char8_t*: _pw_map_has_key_u8,         \
             char32_t*: _pw_map_has_key_u32,        \
            PwValuePtr: _pw_map_has_key             \
    )((map), (key))

bool _pw_map_has_key(PwValuePtr map, PwValuePtr key);

static inline bool _pw_map_has_key_null    (PwValuePtr map, PwType_Null     key) { __PWDECL_Null     (v);      return _pw_map_has_key(map, &v); }
static inline bool _pw_map_has_key_bool    (PwValuePtr map, PwType_Bool     key) { __PWDECL_Bool     (v, key); return _pw_map_has_key(map, &v); }
static inline bool _pw_map_has_key_signed  (PwValuePtr map, PwType_Signed   key) { __PWDECL_Signed   (v, key); return _pw_map_has_key(map, &v); }
static inline bool _pw_map_has_key_unsigned(PwValuePtr map, PwType_Unsigned key) { __PWDECL_Unsigned (v, key); return _pw_map_has_key(map, &v); }
static inline bool _pw_map_has_key_float   (PwValuePtr map, PwType_Float    key) { __PWDECL_Float    (v, key); return _pw_map_has_key(map, &v); }
static inline bool _pw_map_has_key_u8      (PwValuePtr map, char8_t*        key) { __PWDECL_CharPtr  (v, key); return _pw_map_has_key(map, &v); }
static inline bool _pw_map_has_key_u32     (PwValuePtr map, char32_t*       key) { __PWDECL_Char32Ptr(v, key); return _pw_map_has_key(map, &v); }

static inline bool _pw_map_has_key_u8_wrapper(PwValuePtr map, char* key)
{
    return _pw_map_has_key_u8(map, (char8_t*) key);
}

/****************************************************************
 * Get value by `key`. Return PW_ERROR_NO_KEY if `key`
 * is not in the `map`.
 */

#define pw_map_get(map, key) _Generic((key),    \
             nullptr_t: _pw_map_get_null,       \
                  bool: _pw_map_get_bool,       \
                  char: _pw_map_get_signed,     \
         unsigned char: _pw_map_get_unsigned,   \
                 short: _pw_map_get_signed,     \
        unsigned short: _pw_map_get_unsigned,   \
                   int: _pw_map_get_signed,     \
          unsigned int: _pw_map_get_unsigned,   \
                  long: _pw_map_get_signed,     \
         unsigned long: _pw_map_get_unsigned,   \
             long long: _pw_map_get_signed,     \
    unsigned long long: _pw_map_get_unsigned,   \
                 float: _pw_map_get_float,      \
                double: _pw_map_get_float,      \
                 char*: _pw_map_get_u8_wrapper, \
              char8_t*: _pw_map_get_u8,         \
             char32_t*: _pw_map_get_u32,        \
            PwValuePtr: _pw_map_get             \
    )((map), (key))

PwResult _pw_map_get(PwValuePtr map, PwValuePtr key);

static inline PwResult _pw_map_get_null    (PwValuePtr map, PwType_Null     key) { __PWDECL_Null     (v);      return _pw_map_get(map, &v); }
static inline PwResult _pw_map_get_bool    (PwValuePtr map, PwType_Bool     key) { __PWDECL_Bool     (v, key); return _pw_map_get(map, &v); }
static inline PwResult _pw_map_get_signed  (PwValuePtr map, PwType_Signed   key) { __PWDECL_Signed   (v, key); return _pw_map_get(map, &v); }
static inline PwResult _pw_map_get_unsigned(PwValuePtr map, PwType_Unsigned key) { __PWDECL_Unsigned (v, key); return _pw_map_get(map, &v); }
static inline PwResult _pw_map_get_float   (PwValuePtr map, PwType_Float    key) { __PWDECL_Float    (v, key); return _pw_map_get(map, &v); }
static inline PwResult _pw_map_get_u8      (PwValuePtr map, char8_t*        key) { __PWDECL_CharPtr  (v, key); return _pw_map_get(map, &v); }
static inline PwResult _pw_map_get_u32     (PwValuePtr map, char32_t*       key) { __PWDECL_Char32Ptr(v, key); return _pw_map_get(map, &v); }

static inline PwResult _pw_map_get_u8_wrapper(PwValuePtr map, char* key)
{
    return _pw_map_get_u8(map, (char8_t*) key);
}

/****************************************************************
 * Delete item from map by `key`.
 *
 * Return true if value was deleted, false if no value was identified by key.
 */

#define pw_map_del(map, key) _Generic((key),    \
             nullptr_t: _pw_map_del_null,       \
                  bool: _pw_map_del_bool,       \
                  char: _pw_map_del_signed,     \
         unsigned char: _pw_map_del_unsigned,   \
                 short: _pw_map_del_signed,     \
        unsigned short: _pw_map_del_unsigned,   \
                   int: _pw_map_del_signed,     \
          unsigned int: _pw_map_del_unsigned,   \
                  long: _pw_map_del_signed,     \
         unsigned long: _pw_map_del_unsigned,   \
             long long: _pw_map_del_signed,     \
    unsigned long long: _pw_map_del_unsigned,   \
                 float: _pw_map_del_float,      \
                double: _pw_map_del_float,      \
                 char*: _pw_map_del_u8_wrapper, \
              char8_t*: _pw_map_del_u8,         \
             char32_t*: _pw_map_del_u32,        \
            PwValuePtr: _pw_map_del             \
    )((map), (key))

bool _pw_map_del(PwValuePtr map, PwValuePtr key);

static inline bool _pw_map_del_null    (PwValuePtr map, PwType_Null     key) { __PWDECL_Null     (v);      return _pw_map_del(map, &v); }
static inline bool _pw_map_del_bool    (PwValuePtr map, PwType_Bool     key) { __PWDECL_Bool     (v, key); return _pw_map_del(map, &v); }
static inline bool _pw_map_del_signed  (PwValuePtr map, PwType_Signed   key) { __PWDECL_Signed   (v, key); return _pw_map_del(map, &v); }
static inline bool _pw_map_del_unsigned(PwValuePtr map, PwType_Unsigned key) { __PWDECL_Unsigned (v, key); return _pw_map_del(map, &v); }
static inline bool _pw_map_del_float   (PwValuePtr map, PwType_Float    key) { __PWDECL_Float    (v, key); return _pw_map_del(map, &v); }
static inline bool _pw_map_del_u8      (PwValuePtr map, char8_t*        key) { __PWDECL_CharPtr  (v, key); return _pw_map_del(map, &v); }
static inline bool _pw_map_del_u32     (PwValuePtr map, char32_t*       key) { __PWDECL_Char32Ptr(v, key); return _pw_map_del(map, &v); }

static inline bool _pw_map_del_u8_wrapper(PwValuePtr map, char* key)
{
    return _pw_map_del_u8(map, (char8_t*) key);
}

/****************************************************************
 * Misc. functions.
 */

unsigned pw_map_length(PwValuePtr map);
/*
 * Return the number of items in `map`.
 */

bool pw_map_item(PwValuePtr map, unsigned index, PwValuePtr key, PwValuePtr value);
/*
 * Get key-value pair from the map.
 * If `index` is valid return true and write result to key and value.
 *
 * Example:
 *
 * PwValue key = PwNull();
 * PwValue value = PwNull();
 * if (pw_map_item(map, i, &key, &value)) {
 *     // success!
 * }
 */

#ifdef __cplusplus
}
#endif
