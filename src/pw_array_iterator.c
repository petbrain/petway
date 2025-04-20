#include "include/pw.h"
#include "src/pw_array_internal.h"
#include "src/pw_interfaces_internal.h"
#include "src/pw_iterator_internal.h"
#include "src/pw_struct_internal.h"

typedef struct {
    _PwArray* array_data;  // cached array data, also indicates that iteratioo is in progress
    unsigned index;
    int increment;
    unsigned line_number;  // does not match index because line reader may skip non-strings
} _PwArrayIterator;

#define get_data_ptr(value)  ((_PwArrayIterator*) _pw_get_data_ptr((value), PwTypeId_ArrayIterator))


static bool start_iteration(PwValuePtr self)
{
    _PwArrayIterator* data = get_data_ptr(self);
    if (data->array_data) {
        // iteration is already in progress
        return false;
    }
    // cache array data
    _PwIterator* iter_data = get_iterator_data_ptr(self);
    data->array_data = get_array_data_ptr(&iter_data->iterable);

    // init iterator state
    data->index = 0;
    data->increment = 1;

    // increment itercount of array
    data->array_data->itercount++;
    return true;
}

static void stop_iteration(PwValuePtr self)
{
    _PwArrayIterator* data = get_data_ptr(self);
    if (data->array_data) {
        // decrement itercount of array
        data->array_data->itercount--;
        // reset cached array data
        data->array_data = nullptr;
    }
}

/****************************************************************
 * LineReader interface
 */

static PwResult start_read_lines(PwValuePtr self)
{
    if (!start_iteration(self)) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }
    _PwArrayIterator* data = get_data_ptr(self);
    data->line_number = 1;
    return PwOK();
}

static PwResult read_line(PwValuePtr self)
{
    _PwArrayIterator* data = get_data_ptr(self);

    while (data->index < data->array_data->length) {{
        PwValue result = pw_clone(&data->array_data->items[data->index++]);
        if (pw_is_string(&result)) {
            data->line_number++;
            return pw_move(&result);
        }
    }}
    return PwError(PW_ERROR_EOF);
}

static PwResult read_line_inplace(PwValuePtr self, PwValuePtr line)
{
    pw_destroy(line);

    _PwArrayIterator* data = get_data_ptr(self);
    _PwIterator* iter_data = get_iterator_data_ptr(self);
    _PwArray* array_data = get_array_data_ptr(&iter_data->iterable);

    while (data->index < array_data->length) {{
        PwValue result = pw_clone(&array_data->items[data->index++]);
        if (pw_is_string(&result)) {
            *line = pw_move(&result);
            data->line_number++;
            return PwOK();
        }
    }}
    return PwError(PW_ERROR_EOF);
}

static bool unread_line(PwValuePtr self, PwValuePtr line)
{
    _PwArrayIterator* data = get_data_ptr(self);
    _PwIterator* iter_data = get_iterator_data_ptr(self);
    _PwArray* array_data = get_array_data_ptr(&iter_data->iterable);

    // simply decrement iteration index, that's equivalent to pushback
    while (data->index) {
        if (pw_is_string(&array_data->items[data->index--])) {
            data->line_number--;
            return true;
        }
    }
    return false;
}

static unsigned get_line_number(PwValuePtr self)
{
    _PwArrayIterator* data = get_data_ptr(self);
    return data->line_number;
}

static void stop_read_lines(PwValuePtr self)
{
    stop_iteration(self);
}

static PwInterface_LineReader line_reader_interface = {
    .start             = start_read_lines,
    .read_line         = read_line,
    .read_line_inplace = read_line_inplace,
    .get_line_number   = get_line_number,
    .unread_line       = unread_line,
    .stop              = stop_read_lines
};

/****************************************************************
 * ArrayIterator type
 */

static void ari_fini(PwValuePtr self)
{
    // force stop iteration
    stop_iteration(self);
}

static PwType array_iterator_type;

PwTypeId PwTypeId_ArrayIterator = 0;

[[ gnu::constructor ]]
static void init_array_iterator_type()
{
    if (PwTypeId_ArrayIterator == 0) {
        PwTypeId_ArrayIterator = pw_subtype(
            &array_iterator_type, "ArrayIterator", PwTypeId_Iterator, _PwArrayIterator,
            PwInterfaceId_LineReader, &line_reader_interface
        );
        array_iterator_type.fini = ari_fini;
    }
}
