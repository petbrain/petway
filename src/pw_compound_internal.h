#pragma once

#include "include/pw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern PwType _pw_compound_type;

/*
 * Traversing cyclic references requires the list of parents with reference counts.
 * Parent reference count is necessary for cases when, say, a list contains items
 * pointing to the same other list.
 */

#define PW_PARENTS_CHUNK_SIZE  8

typedef struct {
    /*
     * Pointers and refcounts on the list of parents are allocated in chunks
     * because alignment for pointer and integer can be different.
     */
    struct __PwCompoundData* parents[PW_PARENTS_CHUNK_SIZE];
    unsigned parents_refcount[PW_PARENTS_CHUNK_SIZE];
} _PwParentsChunk;

struct __PwCompoundData;
typedef struct __PwCompoundData _PwCompoundData;
struct __PwCompoundData {
    /*
     * We need reference count, so we embed struct data.
     */
    _PwStructData struct_data;

    bool destroying;  // set when destruction is in progress to bypass values during traversal

    /*
     * The minimal structure for tracking circular references
     * is capable to hold two pointers to parent values.
     *
     * For more back references a list is allocated.
     */
    struct {
        union {
            ptrdiff_t using_parents_list: 1;    // given that pointers are aligned, we use least significant bit for the flag
            _PwCompoundData* parents[2];        // using_parents_list == 0, using pointers as is
            struct {
                _PwParentsChunk* parents_list;  // using_parents_list == 1, this points to the list of other parents
                unsigned num_parents_chunks;
            };
        };
        unsigned parents_refcount[2];
    };
};

#define _pw_compound_data_ptr(value)  ((_PwCompoundData*) ((value)->struct_data))


/*
 * methods for use in statically defined descendant types
 */

void _pw_compound_destroy(PwValuePtr self);

#ifdef __cplusplus
}
#endif
