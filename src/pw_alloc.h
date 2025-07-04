#pragma once

#include "include/pw_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Memory allocation helpers.
 */

[[nodiscard]] static inline void* _pw_alloc(PwTypeId type_id, unsigned memsize, bool clean)
{
    void* result = _pw_types[type_id]->allocator->allocate(memsize, clean);
    if (!result) {
        pw_set_status(PwStatus(PW_ERROR_OOM));
    }
    return result;
}

[[nodiscard]] static inline bool _pw_realloc(PwTypeId type_id, void** memblock, unsigned old_memsize, unsigned new_memsize, bool clean)
{
    if (_pw_types[type_id]->allocator->reallocate(memblock, old_memsize, new_memsize, clean, nullptr)) {
        return true;
    } else {
        pw_set_status(PwStatus(PW_ERROR_OOM));
        return false;
    }
}

static inline void _pw_free(PwTypeId type_id, void** memblock, unsigned memsize)
{
    _pw_types[type_id]->allocator->release(memblock, memsize);
}

#ifdef __cplusplus
}
#endif
