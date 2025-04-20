#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    _PwValue iterable;  // cloned iterable value
} _PwIterator;

#define get_iterator_data_ptr(value)  ((_PwIterator*) _pw_get_data_ptr((value), PwTypeId_Iterator))

#ifdef __cplusplus
}
#endif
