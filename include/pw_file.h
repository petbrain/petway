#pragma once

#include <stdarg.h>
#include <uchar.h>

#include <fcntl.h>

#include <pw_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * File type supports the following interfaces:
 *  - File
 *  - Reader
 *  - Writer
 *  - LineReader
 *
 * LineReader can be considered as a singleton iterator,
 * comprised of both iterable and iterator.
 *
 * This means nested iterations aren't possible but they do not make
 * sense for files either.
 */

extern PwTypeId PwTypeId_File;

#define pw_is_file(value)      pw_is_subtype((value), PwTypeId_File)
#define pw_assert_file(value)  pw_assert(pw_is_file(value))


/****************************************************************
 * File interface
 */

extern unsigned PwInterfaceId_File;

typedef struct {
    PwResult (*open)(PwValuePtr self, PwValuePtr file_name, int flags, mode_t mode);
    /*
     * Open or create file with `open` function from standard C library.
     * `mode` is unused when file is not created
     *
     * Return status.
     */

    void (*close)(PwValuePtr self);
    /*
     * Close file if was opened with `open` method or set with ownership transfer.
     */

    int (*get_fd)(PwValuePtr self);
    /*
     * Returns file descriptor or -1.
     */

    PwResult (*set_fd)(PwValuePtr self, int fd, bool move);
    /*
     * Set file descriptor obtained elsewhere.
     * If `move` is true, File takes the ownership and fd will be closed by `close` method.
     * Return status.
     */

    PwResult (*get_name)(PwValuePtr self);
    /*
     * Get file name.
     */

    PwResult (*set_name)(PwValuePtr self, PwValuePtr file_name);
    /*
     * Set file name.
     * This works only if file descriptor was set with `set_fd` method.
     * Return status.
     */

    PwResult (*set_nonblocking)(PwValuePtr self, bool mode);
    /*
     * Set/reset nonblocking mode for file descriptor.
     */

    PwResult (*seek)(PwValuePtr self, off_t offset, int whence);
    /*
     * Return position or error.
     */

    PwResult (*tell)(PwValuePtr self);
    /*
     * Return position or error.
     */

    // TODO truncate, etc.

} PwInterface_File;

/****************************************************************
 * Shorthand functions
 */

#define pw_file_open(file_name, flags, mode) _Generic((file_name), \
             char*: _pw_file_open_u8_wrapper,  \
          char8_t*: _pw_file_open_u8,          \
         char32_t*: _pw_file_open_u32,         \
        PwValuePtr: _pw_file_open              \
    )((file_name), (flags), (mode))

PwResult _pw_file_open(PwValuePtr file_name, int flags, mode_t mode);

static inline PwResult _pw_file_open_u8  (char8_t*  file_name, int flags, mode_t mode) { __PWDECL_CharPtr  (fname, file_name); return _pw_file_open(&fname, flags, mode); }
static inline PwResult _pw_file_open_u32 (char32_t* file_name, int flags, mode_t mode) { __PWDECL_Char32Ptr(fname, file_name); return _pw_file_open(&fname, flags, mode); }

static inline PwResult _pw_file_open_u8_wrapper(char* file_name, int flags, mode_t mode)
{
    return _pw_file_open_u8((char8_t*) file_name, flags, mode);
}

static inline void     pw_file_close   (PwValuePtr file) { pw_interface(file->type_id, File)->close(file); }
static inline int      pw_file_get_fd  (PwValuePtr file) { return pw_interface(file->type_id, File)->get_fd(file); }
static inline PwResult pw_file_get_name(PwValuePtr file) { return pw_interface(file->type_id, File)->get_name(file); }
static inline PwResult pw_file_set_fd  (PwValuePtr file, int fd, bool move)        { return pw_interface(file->type_id, File)->set_fd(file, fd, move); }
static inline PwResult pw_file_set_name(PwValuePtr file, PwValuePtr file_name)     { return pw_interface(file->type_id, File)->set_name(file, file_name); }
static inline PwResult pw_file_set_nonblocking(PwValuePtr file, bool mode)         { return pw_interface(file->type_id, File)->set_nonblocking(file, mode); }
static inline PwResult pw_file_seek    (PwValuePtr file, off_t offset, int whence) { return pw_interface(file->type_id, File)->seek(file, offset, whence); }
static inline PwResult pw_file_tell    (PwValuePtr file) { return pw_interface(file->type_id, File)->tell(file); }


/****************************************************************
 * Miscellaneous functions
 */

PwResult pw_file_size(PwValuePtr file_name);
/*
 * Return file size as Unsigned or Status if error.
 */


/****************************************************************
 * Path functions, probably should be separated
 */

PwResult pw_basename(PwValuePtr filename);
PwResult pw_dirname(PwValuePtr filename);

/*
 * pw_path generic macro allows passing arguments
 * either by value or by reference.
 * When passed by value, the function destroys them
 * .
 * CAVEAT: DO NOT PASS LOCAL VARIABLES BY VALUES!
 */

#define pw_path(part, ...) _Generic((part),  \
        _PwValue:   _pw_path_v,  \
        PwValuePtr: _pw_path_p   \
    )((part) __VA_OPT__(,) __VA_ARGS__,  \
        _Generic((part),  \
            _PwValue:   PwVaEnd(),  \
            PwValuePtr: nullptr     \
        ))

PwResult _pw_path_v(...);
PwResult _pw_path_p(...);

#ifdef __cplusplus
}
#endif
