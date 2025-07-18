#include <limits.h>
#include <string.h>

#include "include/pw.h"
#include "src/pw_alloc.h"
#include "src/pw_compound_internal.h"
#include "src/pw_map_internal.h"
#include "src/pw_struct_internal.h"

#define get_data_ptr(value)  _pw_get_data_ptr((value), PwTypeId_Map)

[[nodiscard]] bool _pw_map_va(PwValuePtr result, ...)
{
    va_list ap;
    va_start(ap);
    if (!pw_create(PwTypeId_Map, result)) {
        _pw_destroy_args(ap);
        va_end(ap);
        return false;
    }
    bool ret = pw_map_update_ap(result, ap);
    va_end(ap);
    return ret;
}

static inline unsigned get_map_length(_PwMap* map)
{
    return _pw_array_length(&map->kv_pairs) >> 1;
}

static uint8_t get_item_size(unsigned capacity)
/*
 * Return hash table item size for desired capacity.
 *
 * Index 0 in hash table means no item, so one byte is capable
 * to index 255 items, not 256.
 */
{
    uint8_t item_size = 1;

    for (unsigned n = capacity; n > 255; n >>= 8) {
        item_size++;
    }
    return item_size;
}

/****************************************************************
 * methods for acessing hash table
 */

#define HT_ITEM_METHODS(typename) \
    static unsigned get_ht_item_##typename(struct _PwHashTable* ht, unsigned index) \
    { \
        return ((typename*) (ht->items))[index]; \
    } \
    static void set_ht_item_##typename(struct _PwHashTable* ht, unsigned index, unsigned value) \
    { \
        ((typename*) (ht->items))[index] = (typename) value; \
    }

HT_ITEM_METHODS(uint8_t)
HT_ITEM_METHODS(uint16_t)
HT_ITEM_METHODS(uint32_t)

#if UINT_WIDTH > 32
    HT_ITEM_METHODS(uint64_t)
#endif

/*
 * methods for acessing hash table with any item size
 */
static unsigned get_ht_item(struct _PwHashTable* ht, unsigned index)
{
    uint8_t *item_ptr = &ht->items[index * ht->item_size];
    unsigned result = 0;
    for (uint8_t i = ht->item_size; i > 0; i--) {
        result <<= 8;
        result += *item_ptr++;
    }
    return result;
}

static void set_ht_item(struct _PwHashTable* ht, unsigned index, unsigned value)
{
    uint8_t *item_ptr = &ht->items[(index + 1) * ht->item_size];
    for (uint8_t i = ht->item_size; i > 0; i--) {
        *(--item_ptr) = (uint8_t) value;
        value >>= 8;
    }
}

/****************************************************************
 * implementation
 *
 * Notes on indexes and naming conventions.
 *
 * In hash map we store indexes of key-value pair (named `kv_index`),
 * not the index in kv_pairs array.
 *
 * key_index is the index of key in kv_pais array, suitable for passing to pw_array_get.
 *
 * So,
 *
 * value_index = key_index + 1
 * kv_index = key_index / 2
 */

[[nodiscard]] static bool init_hash_table(PwTypeId type_id, struct _PwHashTable* ht,
                                          unsigned old_capacity, unsigned new_capacity)
{
    unsigned old_item_size = get_item_size(old_capacity);
    unsigned old_memsize = old_item_size * old_capacity;

    unsigned new_item_size = get_item_size(new_capacity);
    unsigned new_memsize = new_item_size * new_capacity;

    // reallocate items
    // if map is new, ht is initialized to all zero
    // if map is doubled, this reallocates the block
    if (!_pw_realloc(type_id, (void**) &ht->items, old_memsize, new_memsize, false)) {
        return false;
    }
    memset(ht->items, 0, new_memsize);

    ht->item_size    = new_item_size;
    ht->capacity     = new_capacity;
    ht->hash_bitmask = new_capacity - 1;

    switch (new_item_size) {
        case 1:
            ht->get_item = get_ht_item_uint8_t;
            ht->set_item = set_ht_item_uint8_t;
            break;
        case 2:
            ht->get_item = get_ht_item_uint16_t;
            ht->set_item = set_ht_item_uint16_t;
            break;
        case 4:
            ht->get_item = get_ht_item_uint32_t;
            ht->set_item = set_ht_item_uint32_t;
            break;
#if UINT_WIDTH > 32
        case 8:
            ht->get_item = get_ht_item_uint64_t;
            ht->set_item = set_ht_item_uint64_t;
            break;
#endif
        default:
            ht->get_item = get_ht_item;
            ht->set_item = set_ht_item;
            break;
    }
    return true;
}

static void free_hash_table(PwTypeId type_id, struct _PwHashTable* ht)
{
    unsigned ht_memsize = get_item_size(ht->capacity) * ht->capacity;
    _pw_free(type_id, (void**) &ht->items, ht_memsize);
}

static unsigned lookup(_PwMap* map, PwValuePtr key, unsigned* ht_index, unsigned* ht_offset)
/*
 * Lookup key starting from index = hash(key).
 *
 * Return index of key in kv_pairs or UINT_MAX if hash table has no item matching `key`.
 *
 * If `ht_index` is not `nullptr`: write index of hash table item at which lookup has stopped.
 * If `ht_offset` is not `nullptr`: write the difference from final `ht_index` and initial `ht_index` to `ht_offset`;
 */
{
    struct _PwHashTable* ht = &map->hash_table;
    PwType_Hash index = pw_hash(key) & ht->hash_bitmask;
    unsigned offset = 0;
    do {
        unsigned kv_index = ht->get_item(ht, index);

        if (kv_index == 0) {
            // no entry matching key
            if (ht_index) {
                *ht_index = index;
            }
            if (ht_offset) {
                *ht_offset = offset;
            }
            return UINT_MAX;
        }

        // make index 0-based
        kv_index--;

        PwValuePtr k = &map->kv_pairs.items[kv_index * 2];

        // compare keys
        if (_pw_equal(k, key)) {
            // found key
            if (ht_index) {
                *ht_index = index;
            }
            if (ht_offset) {
                *ht_offset = offset;
            }
            return kv_index * 2;
        }

        // probe next item
        index = (index + 1) & ht->hash_bitmask;
        offset++;

    } while (true);
}

static unsigned set_hash_table_item(struct _PwHashTable* hash_table, unsigned ht_index, unsigned kv_index)
/*
 * Assign `kv_index` to `hash_table` at position `ht_index` & hash_bitmask.
 * If the position is already occupied, try next one.
 *
 * Return ht_index, possibly updated.
 */
{
    do {
        ht_index &= hash_table->hash_bitmask;
        if (hash_table->get_item(hash_table, ht_index)) {
            ht_index++;
        } else {
            hash_table->set_item(hash_table, ht_index, kv_index);
            return ht_index;
        }
    } while (true);
}

[[ nodiscard]] static bool _pw_map_expand(PwTypeId type_id, _PwMap* map, unsigned desired_capacity, unsigned ht_offset)
/*
 * Expand map if necessary.
 *
 * ht_offset is a hint, can be 0. If greater or equal 1/4 of capacity, hash table size will be doubled.
 */
{
    // expand array if necessary
    unsigned array_cap = desired_capacity << 1;
    if (array_cap > _pw_array_capacity(&map->kv_pairs)) {
        if (!_pw_array_resize(type_id, &map->kv_pairs, array_cap)) {
            return false;
        }
    }

    struct _PwHashTable* ht = &map->hash_table;

    // check if hash table needs expansion
    unsigned quarter_cap = ht->capacity >> 2;
    if ((ht->capacity >= desired_capacity + quarter_cap) && (ht_offset < quarter_cap)) {
        return true;
    }

    unsigned new_capacity = ht->capacity << 1;
    quarter_cap = desired_capacity >> 2;
    while (new_capacity < desired_capacity + quarter_cap) {
        new_capacity <<= 1;
    }

    if (!init_hash_table(type_id, ht, ht->capacity, new_capacity)) {
        return false;
    }

    // rebuild hash table
    PwValuePtr key_ptr = &map->kv_pairs.items[0];
    unsigned kv_index = 1;  // index is 1-based, zero means unused item in hash table
    unsigned n = _pw_array_length(&map->kv_pairs);
    pw_assert((n & 1) == 0);
    while (n) {
        set_hash_table_item(ht, pw_hash(key_ptr), kv_index);
        key_ptr += 2;
        n -= 2;
        kv_index++;
    }
    return true;
}

[[nodiscard]] static bool update_map(PwValuePtr map, PwValuePtr key, PwValuePtr value)
/*
 * key and value are moved to the internal array
 */
{
    PwTypeId type_id = map->type_id;
    _PwMap* __map = get_data_ptr(map);

    // lookup key in the map

    unsigned ht_offset;
    unsigned key_index = lookup(__map, key, nullptr, &ht_offset);

    if (key_index != UINT_MAX) {
        // found key, update value
        unsigned value_index = key_index + 1;
        PwValuePtr v_ptr = &__map->kv_pairs.items[value_index];
        pw_move(value, v_ptr);
        return true;
    }

    // key not found, insert

    if (!_pw_map_expand(type_id, __map, get_map_length(__map) + 1, ht_offset)) {
        return false;
    }

    // append key and value
    unsigned kv_index = _pw_array_length(&__map->kv_pairs) >> 1;
    set_hash_table_item(&__map->hash_table, pw_hash(key), kv_index + 1);

    if (!_pw_array_append_item(type_id, &__map->kv_pairs, key, map)) {
        return false;
    }
    return _pw_array_append_item(type_id, &__map->kv_pairs, value, map);
}

/****************************************************************
 * Basic interface methods
 */

static void map_fini(PwValuePtr self)
{
    _PwMap* map = get_data_ptr(self);

    _pw_destroy_array(self->type_id, &map->kv_pairs, self);

    struct _PwHashTable* ht = &map->hash_table;
    free_hash_table(self->type_id, ht);
}

[[nodiscard]] static bool map_init(PwValuePtr self, void* ctor_args)
{
    // XXX not using ctor_args for now
    // XXX add capacity parameter

    _PwMap* map = get_data_ptr(self);
    struct _PwHashTable* ht = &map->hash_table;
    ht->items_used = 0;
    if (!init_hash_table(self->type_id, ht, 0, PWMAP_INITIAL_CAPACITY)) {
        return false;
    }
    if (!_pw_alloc_array(self->type_id, &map->kv_pairs, PWMAP_INITIAL_CAPACITY * 2)) {
        map_fini(self);
        return false;
    }
    return true;
}

static void map_hash(PwValuePtr self, PwHashContext* ctx)
{
    _pw_hash_uint64(ctx, self->type_id);
    _PwMap* map = get_data_ptr(self);
    for (unsigned i = 0, n = _pw_array_length(&map->kv_pairs); i < n; i++) {
        PwValuePtr item = &map->kv_pairs.items[i];
        _pw_call_hash(item, ctx);
    }
}

[[nodiscard]] static bool map_deepcopy(PwValuePtr self, PwValuePtr result)
{
    if (!pw_create(self->type_id, result)) {
        return false;
    }
    _PwMap* src_map = get_data_ptr(self);
    unsigned map_length = get_map_length(src_map);

    // XXX use capacity parameter in constructor
    if (!_pw_map_expand(self->type_id, get_data_ptr(result), map_length, 0)) {
        return false;
    }
    PwValuePtr kv = &src_map->kv_pairs.items[0];
    PwValue key = PW_NULL;
    PwValue value = PW_NULL;
    for (unsigned i = 0; i < map_length; i++) {{
        pw_clone2(kv++, &key);  // okay to clone because keys are already deeply copied
        pw_clone2(kv++, &value);
        if (!update_map(result, &key, &value)) {  // error should not happen because the map already resized
            return false;
        }
    }}
    return true;
}

static void map_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    _pw_dump_struct_data(fp, self);
    _pw_dump_compound_data(fp, self, next_indent);
    _pw_print_indent(fp, next_indent);

    PwValuePtr value_seen = _pw_on_chain(self, tail);
    if (value_seen) {
        fprintf(fp, "already dumped: %p, data=%p\n", value_seen, value_seen->struct_data);
        return;
    }

    _PwCompoundChain this_link = {
        .prev = tail,
        .value = self
    };

    _PwMap* map = get_data_ptr(self);
    fprintf(fp, "%u items, array items/capacity=%u/%u\n",
            get_map_length(map), _pw_array_length(&map->kv_pairs), _pw_array_capacity(&map->kv_pairs));

    next_indent += 4;
    PwValuePtr item_ptr = &map->kv_pairs.items[0];
    for (unsigned n = _pw_array_length(&map->kv_pairs); n; n -= 2) {

        PwValuePtr key   = item_ptr++;
        PwValuePtr value = item_ptr++;

        _pw_print_indent(fp, next_indent);
        fputs("Key:   ", fp);
        _pw_call_dump(fp, key, 0, next_indent + 7, &this_link);

        _pw_print_indent(fp, next_indent);
        fputs("Value: ", fp);
        _pw_call_dump(fp, value, 0, next_indent + 7, &this_link);
    }

    _pw_print_indent(fp, next_indent);

    struct _PwHashTable* ht = &map->hash_table;
    fprintf(fp, "hash table item size %u, capacity=%u (bitmask %llx)\n",
            ht->item_size, ht->capacity, (unsigned long long) ht->hash_bitmask);

    unsigned hex_width = ht->item_size;
    unsigned dec_width;
    switch (ht->item_size) {
        case 1: dec_width = 3; break;
        case 2: dec_width = 5; break;
        case 3: dec_width = 8; break;
        case 4: dec_width = 10; break;
        case 5: dec_width = 13; break;
        case 6: dec_width = 15; break;
        case 7: dec_width = 17; break;
        default: dec_width = 20; break;
    }
    char fmt[32];
    sprintf(fmt, "%%%ux: %%-%uu", hex_width, dec_width);
    unsigned line_len = 0;
    _pw_print_indent(fp, next_indent);
    for (unsigned i = 0; i < ht->capacity; i++ ) {
        unsigned kv_index = ht->get_item(ht, i);
        fprintf(fp, fmt, i, kv_index);
        line_len += dec_width + hex_width + 4;
        if (line_len < 80) {
            fputs("  ", fp);
        } else {
            fputc('\n', fp);
            line_len = 0;
            _pw_print_indent(fp, next_indent);
        }
    }
    if (line_len) {
        fputc('\n', fp);
    }
}

[[nodiscard]] static bool map_to_string(PwValuePtr self, PwValuePtr result)
{
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
}

[[nodiscard]] static bool map_is_true(PwValuePtr self)
{
    return get_map_length(get_data_ptr(self));
}

[[nodiscard]] static inline bool map_eq(_PwMap* a, _PwMap* b)
{
    return _pw_array_eq(&a->kv_pairs, &b->kv_pairs);
}

[[nodiscard]] static bool map_equal_sametype(PwValuePtr self, PwValuePtr other)
{
    return map_eq(get_data_ptr(self), get_data_ptr(other));
}

[[nodiscard]] static bool map_equal(PwValuePtr self, PwValuePtr other)
{
    PwTypeId t = other->type_id;
    for (;;) {
        if (t == PwTypeId_Map) {
            return map_eq(get_data_ptr(self), get_data_ptr(other));
        }
        // check base type
        t = _pw_types[t]->ancestor_id;
        if (t == PwTypeId_Null) {
            return false;
        }
    }
}

static PwInterface_RandomAccess random_access_interface;  // forward declaration

static _PwInterface map_interfaces[] = {
    {
        .interface_id      = PwInterfaceId_RandomAccess,
        .interface_methods = (void**) &random_access_interface
    }
};

PwType _pw_map_type = {
    .id             = PwTypeId_Map,
    .ancestor_id    = PwTypeId_Compound,
    .name           = "Map",
    .allocator      = &default_allocator,

    .create         = _pw_struct_create,
    .destroy        = _pw_compound_destroy,
    .clone          = _pw_struct_clone,
    .hash           = map_hash,
    .deepcopy       = map_deepcopy,
    .dump           = map_dump,
    .to_string      = map_to_string,
    .is_true        = map_is_true,
    .equal_sametype = map_equal_sametype,
    .equal          = map_equal,

    .data_offset    = sizeof(_PwCompoundData),
    .data_size      = sizeof(_PwMap),

    .init           = map_init,
    .fini           = map_fini,

    .num_interfaces = PW_LENGTH(map_interfaces),
    .interfaces     = map_interfaces
};

// make sure _PwCompoundData has correct padding
static_assert((sizeof(_PwCompoundData) & (alignof(_PwMap) - 1)) == 0);


/****************************************************************
 * map functions
 */

[[nodiscard]] bool pw_map_update(PwValuePtr map, PwValuePtr key, PwValuePtr value)
{
    pw_assert_map(map);

    PwValue map_key = PW_NULL;
    if (!pw_deepcopy(key, &map_key)) {  // deep copy key for immutability
        return false;
    }
    PwValue map_value = pw_clone(value);
    return update_map(map, &map_key, &map_value);
}

[[nodiscard]] bool _pw_map_update_va(PwValuePtr map, ...)
{
    va_list ap;
    va_start(ap);
    bool ret = pw_map_update_ap(map, ap);
    va_end(ap);
    return ret;
}

[[nodiscard]] bool pw_map_update_ap(PwValuePtr map, va_list ap)
{
    pw_assert_map(map);
    bool done = false;  // for special case when value is missing
    while (!done) {{
        PwValue key = va_arg(ap, _PwValue);
        if (pw_is_status(&key)) {
            if (pw_is_va_end(&key)) {
                return true;
            }
            pw_set_status(pw_clone(&key));
            goto failure;
        }
        PwValue value = va_arg(ap, _PwValue);
        if (pw_is_status(&value)) {
            if (pw_is_va_end(&value)) {
                pw_destroy(&value);
                value = PwNull();
                done = true;
            } else {
                pw_set_status(pw_clone(&value));
                goto failure;
            }
        }
        if (!update_map(map, &key, &value)) {
            goto failure;
        }
    }}

failure:
    // consume args
    if (!done) {
        _pw_destroy_args(ap);
    }
    return false;
}

[[nodiscard]] bool _pw_map_has_key(PwValuePtr self, PwValuePtr key)
{
    pw_assert_map(self);
    _PwMap* map = get_data_ptr(self);
    return lookup(map, key, nullptr, nullptr) != UINT_MAX;
}

[[nodiscard]] bool _pw_map_get(PwValuePtr self, PwValuePtr key, PwValuePtr result)
{
    pw_assert_map(self);
    _PwMap* map = get_data_ptr(self);

    // lookup key in the map
    unsigned key_index = lookup(map, key, nullptr, nullptr);

    if (key_index == UINT_MAX) {
        // key not found
        pw_set_status(PwStatus(PW_ERROR_KEY_NOT_FOUND));
        return false;
    }
    // return value
    unsigned value_index = key_index + 1;
    pw_clone2(&map->kv_pairs.items[value_index], result);
    return true;
}

[[nodiscard]] bool _pw_map_del(PwValuePtr self, PwValuePtr key)
{
    pw_assert_map(self);

    _PwMap* map = get_data_ptr(self);

    // lookup key in the map

    unsigned ht_index;
    unsigned key_index = lookup(map, key, &ht_index, nullptr);
    if (key_index == UINT_MAX) {
        // key not found
        pw_set_status(PwStatus(PW_ERROR_KEY_NOT_FOUND));
        return false;
    }

    struct _PwHashTable* ht = &map->hash_table;

    // delete item from hash table
    ht->set_item(ht, ht_index, 0);
    ht->items_used--;

    // delete key-value pair
    _pw_array_del(&map->kv_pairs, key_index, key_index + 2, self);

    if (key_index + 2 < _pw_array_length(&map->kv_pairs)) {
        // key-value was not the last pair in the array,
        // decrement indexes in the hash table that are greater than index of the deleted pair
        unsigned threshold = (key_index + 2) >> 1;
        threshold++; // kv_indexes in hash table are 1-based
        for (unsigned i = 0; i < ht->capacity; i++) {
            unsigned kv_index = ht->get_item(ht, i);
            if (kv_index >= threshold) {
                ht->set_item(ht, i, kv_index - 1);
            }
        }
    }
    return true;
}

unsigned pw_map_length(PwValuePtr self)
{
    pw_assert_map(self);
    return get_map_length(get_data_ptr(self));
}

[[nodiscard]] bool pw_map_item(PwValuePtr self, unsigned index, PwValuePtr key, PwValuePtr value)
{
    pw_assert_map(self);

    _PwMap* map = get_data_ptr(self);

    index <<= 1;

    if (index < _pw_array_length(&map->kv_pairs)) {
        pw_clone2(&map->kv_pairs.items[index], key);
        pw_clone2(&map->kv_pairs.items[index + 1], value);
        return true;
    } else {
        return false;
    }
}

/****************************************************************
 * RandomAccess interface
 */

static PwInterface_RandomAccess random_access_interface = {
    .length      = pw_map_length,
    .get_item    = _pw_map_get,
    .set_item    = pw_map_update,
    .delete_item = _pw_map_del
};
