#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /*
     * This structure extends _PwStructData.
     */
    _PwStructData struct_data;

    _PwValue iterable;  // cloned iterable value
} _PwIterator;

#define get_iterator_ptr(value)  ((_PwIterator*) ((value)->struct_data))

#ifdef __cplusplus
}
#endif
