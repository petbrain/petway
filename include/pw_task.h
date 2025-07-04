#pragma once

#include <pw_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    _PwValue status;
} PwTask;

extern PwTask* current_task;

#ifdef __cplusplus
}
#endif
