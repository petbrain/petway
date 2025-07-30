#include "include/pw.h"

void _pw_dump_start(FILE* fp, PwValuePtr value, int indent)
{
    char* type_name = pw_typeof(value)->name;

    _pw_print_indent(fp, indent);
    fprintf(fp, "%p", (void*) value);
    if (type_name == nullptr) {
        fprintf(fp, " BAD TYPE %d", value->type_id);
    } else {
        fprintf(fp, " %s (type id: %d)", type_name, value->type_id);
    }
}

void _pw_print_indent(FILE* fp, int indent)
{
    for (int i = 0; i < indent; i++ ) {
        fputc(' ', fp);
    }
}

void _pw_dump(FILE* fp, PwValuePtr value, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    if (value == nullptr) {
        _pw_print_indent(fp, first_indent);
        fprintf(fp, "nullptr\n");
        return;
    }
    PwMethodDump fn_dump = pw_typeof(value)->dump;
    pw_assert(fn_dump != nullptr);
    fn_dump(value, fp, first_indent, next_indent, tail);
}

void pw_dump(FILE* fp, PwValuePtr value)
{
    _pw_dump(fp, value, 0, 0, nullptr);
}
