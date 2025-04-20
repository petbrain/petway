#pragma once

#include <stdio.h>

#include <pw_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 * Dump functions
 */

void _pw_print_indent(FILE* fp, int indent);
void _pw_dump_start(FILE* fp, PwValuePtr value, int indent);
void _pw_dump_struct_data(FILE* fp, PwValuePtr value);
void _pw_dump_compound_data(FILE* fp, PwValuePtr value, int indent);
void _pw_dump(FILE* fp, PwValuePtr value, int first_indent, int next_indent, _PwCompoundChain* tail);

void pw_dump(FILE* fp, PwValuePtr value);

static inline void _pw_call_dump(FILE* fp, PwValuePtr value, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    pw_typeof(value)->dump(value, fp, first_indent, next_indent, tail);
}

#ifdef __cplusplus
}
#endif
