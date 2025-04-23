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

typedef PwResult (*PwMethodOpenFile)         (PwValuePtr self, PwValuePtr file_name, int flags, mode_t mode);
typedef void     (*PwMethodCloseFile)        (PwValuePtr self);
typedef bool     (*PwMethodSetFileDescriptor)(PwValuePtr self, int fd);
typedef PwResult (*PwMethodGetFileName)      (PwValuePtr self);
typedef bool     (*PwMethodSetFileName)      (PwValuePtr self, PwValuePtr file_name);

// XXX other fd operation: seek, tell, etc.

typedef struct {
    PwMethodOpenFile          open;
    PwMethodCloseFile         close;  // only if opened with `open`, don't close one assigned by `set_fd`, right?
    PwMethodSetFileDescriptor set_fd;
    PwMethodGetFileName       get_name;
    PwMethodSetFileName       set_name;

    PwResult (*set_nonblocking)(PwValuePtr self, bool mode);
    /*
     * Set/reset nonblocking mode for file descriptor.
     */

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

static inline void     pw_file_close   (PwValuePtr file)         { pw_interface(file->type_id, File)->close(file); }
static inline bool     pw_file_set_fd  (PwValuePtr file, int fd) { return pw_interface(file->type_id, File)->set_fd(file, fd); }
static inline PwResult pw_file_get_name(PwValuePtr file)         { return pw_interface(file->type_id, File)->get_name(file); }
static inline bool     pw_file_set_name(PwValuePtr file, PwValuePtr file_name)  { return pw_interface(file->type_id, File)->set_name(file, file_name); }


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
