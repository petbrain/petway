#pragma once

/*
 * Map internals.
 */

#include "src/pw_array_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

// capacity must be power of two, it doubles when map needs to grow
#define PWMAP_INITIAL_CAPACITY  8

struct _PwHashTable;

typedef unsigned (*_PwHtGet)(struct _PwHashTable* ht, unsigned index);
typedef void     (*_PwHtSet)(struct _PwHashTable* ht, unsigned index, unsigned value);

struct _PwHashTable {
    uint8_t item_size;   // in bytes
    PwType_Hash hash_bitmask;  // calculated from item_size
    unsigned items_used;
    unsigned capacity;
    _PwHtGet get_item;  // getter function for specific item size
    _PwHtSet set_item;  // setter function for specific item size
    uint8_t* items;     // items have variable size
};

typedef struct {
    _PwArray kv_pairs;        // key-value pairs in the insertion order
    struct _PwHashTable hash_table;
} _PwMap;

#ifdef __cplusplus
}
#endif
