#pragma once

#include <stdarg.h>

#include <pw_assert.h>
#include <pw_helpers.h>
#include <pw_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct __PwInterface {
    unsigned interface_id;
    void** interface_methods;  // pointer to array of pointers to functions,
                               // pw_interface casts this to pointer to interface structure
};
typedef struct __PwInterface _PwInterface;

/*
 * Built-in interfaces
 */
#define PwInterfaceId_Reader      0
#define PwInterfaceId_Writer      1
#define PwInterfaceId_LineReader  2  // iterator interface

/****************************************************************
 * Reader interface
 */

typedef PwResult (*PwMethodRead)(PwValuePtr self, void* buffer, unsigned buffer_size, unsigned* bytes_read);

typedef struct {
    PwMethodRead read;

} PwInterface_Reader;


/****************************************************************
 * Writer interface
 */

typedef PwResult (*PwMethodWrite)(PwValuePtr self, void* data, unsigned size, unsigned* bytes_written);

typedef struct {
    PwMethodWrite write;

} PwInterface_Writer;


/****************************************************************
 * LineReader interface
 */

typedef PwResult (*PwMethodStartReadLines)(PwValuePtr self);
/*
 * Prepare to read lines.
 *
 * Basically, any value that implements LineReader interface
 * must be prepared to read lines without making this call.
 *
 * Calling this method again should reset line reader.
 */

typedef PwResult (*PwMethodReadLine)(PwValuePtr self);
/*
 * Read next line.
 */

typedef PwResult (*PwMethodReadLineInPlace)(PwValuePtr self, PwValuePtr line);
/*
 * Truncate line and read next line into it.
 * Return true if read some data, false if error or eof.
 *
 * Rationale: avoid frequent memory allocations that would take place
 * if we returned line by line.
 * On the contrary, existing line is a pre-allocated buffer for the next one.
 */

typedef bool (*PwMethodUnreadLine)(PwValuePtr self, PwValuePtr line);
/*
 * Push line back to the reader.
 * Only one pushback is guaranteed.
 * Return false if pushback buffer is full.
 */

typedef unsigned (*PwMethodGetLineNumber)(PwValuePtr self);
/*
 * Return current line number, 1-based.
 */

typedef void (*PwMethodStopReadLines)(PwValuePtr self);
/*
 * Free internal buffer.
 */

typedef struct {
    PwMethodStartReadLines  start;
    PwMethodReadLine        read_line;
    PwMethodReadLineInPlace read_line_inplace;
    PwMethodUnreadLine      unread_line;
    PwMethodGetLineNumber   get_line_number;
    PwMethodStopReadLines   stop;

} PwInterface_LineReader;


/*
// TBD, TODO
#define PwInterfaceId_Logic         0
    // TBD
#define PwInterfaceId_Arithmetic    1
    // TBD
#define PwInterfaceId_Bitwise       2
    // TBD
#define PwInterfaceId_Comparison    3
    // PwMethodCompare  -- compare_sametype, compare;
#define PwInterfaceId_RandomAccess  4
    // PwMethodLength
    // PwMethodGetItem     (by index for arrays/strings or by key for maps)
    // PwMethodSetItem     (by index for arrays/strings or by key for maps)
    // PwMethodDeleteItem  (by index for arrays/strings or by key for maps)
    // PwMethodPopItem -- necessary here? it's just delete_item(length - 1)
#define PwInterfaceId_String        5
    // TBD substring, truncate, trim, append_substring, etc
#define PwInterfaceId_Array         6
    // string supports this interface
    // PwMethodAppend
    // PwMethodSlice
    // PwMethodDeleteRange
*/


unsigned _pw_register_interface(char* name, unsigned num_methods);
/*
 * Generate global identifier for interface and associate
 * `name` and `num_methods` with it.
 *
 * The number of methods is used to initialize interface fields
 * in `_pw_add_type` and `_pw_subtype` functions.
 */

#define pw_register_interface(name, interface_type)  \
    _pw_register_interface((name), sizeof(interface_type) / sizeof(void*))

char* pw_get_interface_name(unsigned interface_id);
/*
 * Get registered interface name by id.
 */

static inline _PwInterface* _pw_lookup_interface(PwTypeId type_id, unsigned interface_id)
{
    PwType* type = _pw_types[type_id];
    _PwInterface* iface = type->interfaces;
    if (iface) {
        unsigned i = 0;
        do {
            if (iface->interface_id == interface_id) {
                return iface;
            }
            iface++;
            i++;
        } while (i < type->num_interfaces);
    }
    return nullptr;
}

static inline void* _pw_get_interface(PwTypeId type_id, unsigned interface_id)
{
    _PwInterface* result = _pw_lookup_interface(type_id, interface_id);
    if (result) {
        return result->interface_methods;
    }
    _pw_panic_no_interface(type_id, interface_id);
}

static inline bool _pw_has_interface(PwTypeId type_id, unsigned interface_id)
{
    return (bool) _pw_lookup_interface(type_id, interface_id);
}

/*
 * The following macro depends on naming scheme where
 * interface structure is named PwInterface_<interface_name>
 * and id is named PwInterfaceId_<interface_name>
 *
 * Example use:
 *
 * pw_interface(reader->type_id, LineReader)->read_line(reader);
 *
 * Calling super method:
 *
 * pw_interface(PwTypeId_AncestorOfReader, LineReader)->read_line(reader);
 */
#define pw_interface(type_id, interface_name)  \
    (  \
        (PwInterface_##interface_name *)  \
            _pw_get_interface((type_id), PwInterfaceId_##interface_name)  \
    )


/****************************************************************
 * Shorthand functions
 */

static inline PwResult pw_read(PwValuePtr reader, void* buffer, unsigned buffer_size, unsigned* bytes_read)
{
    return pw_interface(reader->type_id, Reader)->read(reader, buffer, buffer_size, bytes_read);
}

static inline PwResult pw_write(PwValuePtr writer, void* data, unsigned size, unsigned* bytes_written)
{
    return pw_interface(writer->type_id, Writer)->write(writer, data, size, bytes_written);
}

#ifdef __cplusplus
}
#endif
