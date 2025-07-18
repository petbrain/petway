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
#define PwInterfaceId_RandomAccess  0
#define PwInterfaceId_Reader        1
#define PwInterfaceId_Writer        2
#define PwInterfaceId_LineReader    3  // iterator interface

/****************************************************************
 * RandomAccess interface
 *
 * Methods that accept key argument may convert it from String
 * to number if items are accessible by numeric index.
 */

typedef struct {
    unsigned (*length)(PwValuePtr self);
    [[nodiscard]] bool (*get_item)(PwValuePtr self, PwValuePtr key, PwValuePtr result);
    [[nodiscard]] bool (*set_item)(PwValuePtr self, PwValuePtr key, PwValuePtr value);
    [[nodiscard]] bool (*delete_item)(PwValuePtr self, PwValuePtr key);

} PwInterface_RandomAccess;

/****************************************************************
 * Reader interface
 */

typedef struct {
    [[nodiscard]] bool (*read)(PwValuePtr self, void* buffer, unsigned buffer_size, unsigned* bytes_read);
} PwInterface_Reader;


/****************************************************************
 * Writer interface
 */

typedef struct {
    [[nodiscard]] bool (*write)(PwValuePtr self, void* data, unsigned size, unsigned* bytes_written);
} PwInterface_Writer;


/****************************************************************
 * LineReader interface
 */


typedef struct {

    [[nodiscard]] bool (*start)(PwValuePtr self);
    /*
     * Prepare to read lines.
     *
     * Basically, any value that implements LineReader interface
     * must be prepared to read lines without making this call.
     *
     * Calling this method again should reset line reader.
     */

    [[nodiscard]] bool (*read_line)(PwValuePtr self, PwValuePtr result);
    /*
     * Read next line.
     */

    [[nodiscard]] bool (*read_line_inplace)(PwValuePtr self, PwValuePtr line);
    /*
     * Truncate line and read next line into it.
     * Return true if read some data, false if error or eof.
     *
     * This makes sense for files, but for string list it destroys line and clones nexr one into it.
     */

    [[nodiscard]] bool (*unread_line)(PwValuePtr self, PwValuePtr line);
    /*
     * Push line back to the reader.
     * Only one pushback is guaranteed.
     * Return false if pushback buffer is full.
     */

    [[nodiscard]] unsigned (*get_line_number)(PwValuePtr self);
    /*
     * Return current line number, 1-based.
     */

    void (*stop)(PwValuePtr self);
    /*
     * Free internal buffer.
     */

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
#define PwInterfaceId_String        5
    // TBD substring, truncate, trim, append_substring, etc
#define PwInterfaceId_Array         6
    // string supports this interface too
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

[[noreturn]]
void _pw_panic_no_interface(PwTypeId type_id, unsigned interface_id);

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

[[nodiscard]] static inline bool pw_read(PwValuePtr reader, void* buffer, unsigned buffer_size, unsigned* bytes_read)
{
    return pw_interface(reader->type_id, Reader)->read(reader, buffer, buffer_size, bytes_read);
}

[[nodiscard]] static inline bool pw_write(PwValuePtr writer, void* data, unsigned size, unsigned* bytes_written)
{
    return pw_interface(writer->type_id, Writer)->write(writer, data, size, bytes_written);
}

[[nodiscard]] bool pw_write_exact(PwValuePtr writeable, void* data, unsigned size);
/*
 * Write exactly `size` bytes. Return status.
 * XXX blend with bfile_strict_write?
 */

#ifdef __cplusplus
}
#endif
