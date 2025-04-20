#pragma once

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void _pw_init_interfaces();
/*
 * Initialize interface ids.
 * Declared with [[ gnu::constructor ]] attribute and automatically called
 * before main().
 *
 * However, the order of initialization is undefined and other modules
 * that use interfaces must call it explicitly from their constructors.
 *
 * This function is idempotent.
 */

void _pw_create_interfaces(PwType* type, va_list ap);
void _pw_update_interfaces(PwType* type, PwType* ancestor, va_list ap);
unsigned _pw_get_num_interface_methods(unsigned interface_id);
/*
 * for internal use
 */

#ifdef __cplusplus
}
#endif
