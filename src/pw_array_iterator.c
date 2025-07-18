#include "include/pw.h"
#include "src/pw_array_internal.h"
#include "src/pw_interfaces_internal.h"
#include "src/pw_iterator_internal.h"
#include "src/pw_struct_internal.h"

typedef struct {
    _PwArray* array;  // cached pointer to array structure, also indicates that iteration is in progress
    unsigned index;
    int increment;
    unsigned line_number;  // does not match index because line reader may skip non-strings
} _PwArrayIterator;

#define get_array_iterator_ptr(value)  ((_PwArrayIterator*) _pw_get_data_ptr((value), PwTypeId_ArrayIterator))

[[nodiscard]] static bool start_iteration(PwValuePtr self)
{
    _PwArrayIterator* iterator = get_array_iterator_ptr(self);
    if (iterator->array) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }
    // cache pointer to array structure
    iterator->array = get_array_struct_ptr(&get_iterator_ptr(self)->iterable);

    // init iterator state
    iterator->index = 0;
    iterator->increment = 1;

    // increment itercount of array
    iterator->array->itercount++;
    return true;
}

static void stop_iteration(PwValuePtr self)
{
    _PwArrayIterator* iterator = get_array_iterator_ptr(self);
    if (iterator->array) {
        // decrement itercount of array
        iterator->array->itercount--;
        // reset pointer to array structure
        iterator->array = nullptr;
    }
}

/****************************************************************
 * LineReader interface
 */

[[nodiscard]] static bool start_read_lines(PwValuePtr self)
{
    if (!start_iteration(self)) {
        return false;
    }
    _PwArrayIterator* iterator = get_array_iterator_ptr(self);
    iterator->line_number = 1;
    return true;
}

[[nodiscard]] static bool read_line(PwValuePtr self, PwValuePtr result)
{
    _PwArrayIterator* iterator = get_array_iterator_ptr(self);

    while (iterator->index < iterator->array->length) {
        PwValuePtr item_ptr = &iterator->array->items[iterator->index++];
        if (pw_is_string(item_ptr)) {
            pw_clone2(item_ptr, result);
            iterator->line_number++;
            return true;
        }
    }
    pw_set_status(PwStatus(PW_ERROR_EOF));
    return false;
}

[[nodiscard]] static bool unread_line(PwValuePtr self, PwValuePtr line)
{
    _PwArrayIterator* iterator = get_array_iterator_ptr(self);
    _PwArray* array = get_array_struct_ptr(&get_iterator_ptr(self)->iterable);

    // simply decrement iteration index, that's equivalent to pushback
    while (iterator->index) {
        if (pw_is_string(&array->items[iterator->index--])) {
            iterator->line_number--;
            return true;
        }
    }
    return false;
}

[[nodiscard]] static unsigned get_line_number(PwValuePtr self)
{
    _PwArrayIterator* iterator = get_array_iterator_ptr(self);
    return iterator->line_number;
}

static void stop_read_lines(PwValuePtr self)
{
    stop_iteration(self);
}

static PwInterface_LineReader line_reader_interface = {
    .start             = start_read_lines,
    .read_line         = read_line,
    .read_line_inplace = read_line, // `read_line` clones list item. Truly reading in-place would imply copying.

    .get_line_number   = get_line_number,
    .unread_line       = unread_line,
    .stop              = stop_read_lines
};

/****************************************************************
 * ArrayIterator type
 */

static void array_iter_fini(PwValuePtr self)
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
        PwTypeId_ArrayIterator = pw_struct_subtype(
            &array_iterator_type, "ArrayIterator", PwTypeId_Iterator, _PwArrayIterator,
            PwInterfaceId_LineReader, &line_reader_interface
        );
        array_iterator_type.fini = array_iter_fini;
    }
}
