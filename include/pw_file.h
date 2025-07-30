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
 */

extern PwTypeId PwTypeId_File;

#define pw_is_file(value)      pw_is_subtype((value), PwTypeId_File)
#define pw_assert_file(value)  pw_assert(pw_is_file(value))


/*
 * In addition to interfaces supported by File, BufferedFile type also supports:
 *  - BufferedFile
 *  - LineReader
 *
 * Position returned by `seek` method is meaningless for buffered file.
 * This method resets buffers. Write buffer is flushed.
 *
 * LineReader has dual purpose. Its `read_line`, `unread_line` can be used as normal
 * methods, but when wrapped with `start` and `stop`, it can be considered as a singleton
 * iterator, comprised of both iterable and iterator.
 *
 * This means nested iterations aren't possible but they do not make
 * sense for files either.
 */

extern PwTypeId PwTypeId_BufferedFile;

#define pw_is_buffered_file(value)      pw_is_subtype((value), PwTypeId_BufferedFile)
#define pw_assert_buffered_file(value)  pw_assert(pw_is_buffered_file(value))

typedef struct {
    unsigned read_bufsize;
    unsigned write_bufsize;

} PwBufferedFileCtorArgs;


/****************************************************************
 * File interface
 */

extern unsigned PwInterfaceId_File;

typedef struct {
    [[ gnu::warn_unused_result ]] bool (*open)(PwValuePtr self, PwValuePtr file_name, int flags, mode_t mode);
    /*
     * Open or create file with `open` function from standard C library.
     * `mode` is unused when file is not created
     */

    [[ gnu::warn_unused_result ]] bool (*close)(PwValuePtr self);
    /*
     * Close file if was opened with `open` method or set with ownership transfer.
     */

    [[ gnu::warn_unused_result ]] int (*get_fd)(PwValuePtr self);
    /*
     * Returns file descriptor or -1.
     */

    [[ gnu::warn_unused_result ]] bool (*set_fd)(PwValuePtr self, int fd, bool take_ownership);
    /*
     * Set file descriptor obtained elsewhere.
     * If `take_ownership` is true, File takes the ownership and fd will be closed by `close` method.
     */

    [[ gnu::warn_unused_result ]] bool (*get_name)(PwValuePtr self, PwValuePtr result);
    /*
     * Get file name.
     */

    [[ gnu::warn_unused_result ]] bool (*set_name)(PwValuePtr self, PwValuePtr file_name);
    /*
     * Set file name.
     * This works only if file descriptor was set with `set_fd` method.
     */

    [[ gnu::warn_unused_result ]] bool (*set_nonblocking)(PwValuePtr self, bool mode);
    /*
     * Set/reset nonblocking mode for file descriptor.
     */

    [[ gnu::warn_unused_result ]] bool (*seek)(PwValuePtr self, off_t offset, int whence, off_t* position);
    /*
     * XXX all clear but a good comment would be good
     * position can be nullptr
     */

    [[ gnu::warn_unused_result ]] bool (*tell)(PwValuePtr self, off_t* position);
    /*
     * XXX all clear but a good comment would be good
     */

    // TODO truncate, etc.

} PwInterface_File;


/****************************************************************
 * BufferedFile interface
 */

extern unsigned PwInterfaceId_BufferedFile;

typedef struct {

    [[ gnu::warn_unused_result ]] bool (*flush)(PwValuePtr self);
    /*
     * Flush write buffer.
     */

} PwInterface_BufferedFile;


/****************************************************************
 * Shorthand functions
 */

// `pw_file_open` and `pw_file_from_fd` return BufferedFile
// with read buffer equals to page size and with no write bufer

#define pw_file_open(file_name, flags, mode, result) _Generic((file_name), \
             char*: _pw_file_open_ascii,  \
          char8_t*: _pw_file_open_utf8,   \
         char32_t*: _pw_file_open_utf32,  \
        PwValuePtr: _pw_file_open         \
    )((file_name), (flags), (mode), (result))

[[nodiscard]] bool _pw_file_open(PwValuePtr file_name, int flags, mode_t mode, PwValuePtr result);

[[nodiscard]] static inline bool _pw_file_open_ascii(char*  file_name, int flags, mode_t mode, PwValuePtr result)
{
    _PwValue fname = PwStaticString(file_name);
    return _pw_file_open(&fname, flags, mode, result);
}

[[nodiscard]] static inline bool _pw_file_open_utf8(char8_t*  file_name, int flags, mode_t mode, PwValuePtr result)
{
    PwValue fname = PwStringUtf8(file_name);
    return _pw_file_open(&fname, flags, mode, result);
}

[[nodiscard]] static inline bool _pw_file_open_utf32(char32_t* file_name, int flags, mode_t mode, PwValuePtr result)
{
    _PwValue fname = PwStaticString(file_name);
    return _pw_file_open(&fname, flags, mode, result);
}


[[nodiscard]] bool pw_file_from_fd(int fd, bool take_ownership, PwValuePtr result);

[[nodiscard]] static inline bool pw_file_close(PwValuePtr file)
{
    return pw_interface(file->type_id, File)->close(file);
}

[[nodiscard]] static inline int  pw_file_get_fd(PwValuePtr file)
{
    return pw_interface(file->type_id, File)->get_fd(file);
}

[[nodiscard]] static inline bool pw_file_get_name(PwValuePtr file, PwValuePtr result)
{
    return pw_interface(file->type_id, File)->get_name(file, result);
}

[[nodiscard]] static inline bool pw_file_set_fd(PwValuePtr file, int fd, bool take_ownership)
{
    return pw_interface(file->type_id, File)->set_fd(file, fd, take_ownership);
}

[[nodiscard]] static inline bool pw_file_set_name(PwValuePtr file, PwValuePtr file_name)
{
    return pw_interface(file->type_id, File)->set_name(file, file_name);
}

[[nodiscard]] static inline bool pw_file_set_nonblocking(PwValuePtr file, bool mode)
{
    return pw_interface(file->type_id, File)->set_nonblocking(file, mode);
}

[[nodiscard]] static inline bool pw_file_seek(PwValuePtr file, off_t offset, int whence, off_t* position)
{
    return pw_interface(file->type_id, File)->seek(file, offset, whence, position);
}

[[nodiscard]] static inline bool pw_file_tell(PwValuePtr file, off_t* position)
{
    return pw_interface(file->type_id, File)->tell(file, position);
}

[[nodiscard]] static inline bool pw_file_flush(PwValuePtr file)
{
    return pw_interface(file->type_id, BufferedFile)->flush(file);
}


/****************************************************************
 * Miscellaneous functions
 */

[[nodiscard]] bool pw_file_size(PwValuePtr file_name, off_t* size);
/*
 * Return file size as Unsigned or Status if error.
 */


/****************************************************************
 * Path functions, probably should be separated
 */

[[nodiscard]] bool pw_basename(PwValuePtr filename, PwValuePtr result);
[[nodiscard]] bool pw_dirname(PwValuePtr filename, PwValuePtr result);

#define pw_path(result, ...)  \
    _pw_path_va((result) __VA_OPT__(,) __VA_ARGS__, PwVaEnd())

[[nodiscard]] bool _pw_path_va(PwValuePtr result, ...);

#ifdef __cplusplus
}
#endif
