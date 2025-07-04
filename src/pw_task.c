#include "include/pw_task.h"

static PwTask initial_task = {};

PwTask* current_task = &initial_task;
