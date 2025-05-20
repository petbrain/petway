#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "include/pw.h"
#include "src/pw_interfaces_internal.h"
#include "src/pw_string_internal.h"
#include "src/pw_struct_internal.h"

/****************************************************************
 * File type
 */

PwTypeId PwTypeId_File = 0;

static PwType file_type;

typedef struct {
    _PwValue name;
    int  fd;              // file descriptor
    bool is_external_fd;  // fd is set by `set_fd`
    bool own_fd;          // fd is owned, If not owned, it is not closed by `close`
    int  error;           // errno, set by `open`

} _PwFile;

#define get_file_data_ptr(value)  ((_PwFile*) _pw_get_data_ptr((value), PwTypeId_File))


// forward declaration
static void file_close(PwValuePtr self);


static PwResult file_init(PwValuePtr self, void* ctor_args)
{
    // ctor_args is unused

    _PwFile* f = get_file_data_ptr(self);
    f->fd = -1;
    f->name = PwNull();
    return PwOK();
}

static void file_fini(PwValuePtr self)
{
    file_close(self);
}

static void file_hash(PwValuePtr self, PwHashContext* ctx)
{
    // it's not a hash of entire file content!

    _PwFile* f = get_file_data_ptr(self);

    _pw_hash_uint64(ctx, self->type_id);

    // XXX
    _pw_call_hash(&f->name, ctx);
    _pw_hash_uint64(ctx, f->fd);
    _pw_hash_uint64(ctx, f->is_external_fd);
}

static PwResult file_deepcopy(PwValuePtr self)
{
    // XXX duplicate fd?
    return PwError(PW_ERROR_NOT_IMPLEMENTED);
}

static void basic_file_dump(PwValuePtr self, FILE* fp)
{
    _PwFile* f = get_file_data_ptr(self);

    if (pw_is_string(&f->name)) {
        PW_CSTRING_LOCAL(file_name, &f->name);
        fprintf(fp, " name: %s", file_name);
    } else {
        fprintf(fp, " name: Null");
    }
    fprintf(fp, " fd: %d", f->fd);
    if (f->is_external_fd) {
        char* owned = "";
        if (f->own_fd) {
            owned = ",owned";
        }
        fprintf(fp, " (external%s)", owned);
    }
}

static void file_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    _pw_dump_struct_data(fp, self);
    basic_file_dump(self, fp);
    fputc('\n', fp);
}

/****************************************************************
 * File interface
 */

unsigned PwInterfaceId_File = 0;

static PwResult file_open(PwValuePtr self, PwValuePtr file_name, int flags, mode_t mode)
{
    _PwFile* f = get_file_data_ptr(self);

    if (f->fd != -1) {
        return PwError(PW_ERROR_FILE_ALREADY_OPENED);
    }

    if (pw_is_charptr(file_name)) {
        switch (file_name->charptr_subtype) {
            case PW_CHARPTR:
                do {
                    f->fd = open((char*) file_name->charptr, flags, mode);
                } while (f->fd == -1 && errno == EINTR);
                break;

            case PW_CHAR32PTR: {
                PW_CSTRING_LOCAL(fname, file_name);
                do {
                    f->fd = open(fname, flags, mode);
                } while (f->fd == -1 && errno == EINTR);
                break;
            }
            default:
                _pw_panic_bad_charptr_subtype(file_name);
        }
    } else {
        pw_assert_string(file_name);
        PW_CSTRING_LOCAL(fname, file_name);
        do {
            f->fd = open(fname, flags, mode);
        } while (f->fd == -1 && errno == EINTR);
    }

    if (f->fd == -1) {
        f->error = errno;
        return PwErrno(errno);
    }

    // set file name
    pw_destroy(&f->name);
    f->name = pw_clone(file_name);

    f->is_external_fd = false;
    f->own_fd = true;

    return PwOK();
}

static void file_close(PwValuePtr self)
{
    _PwFile* f = get_file_data_ptr(self);

    if (f->fd != -1 && f->own_fd) {
        close(f->fd);
    }
    f->fd = -1;
    f->error = 0;
    pw_destroy(&f->name);
}

static int file_get_fd(PwValuePtr self)
{
    return get_file_data_ptr(self)->fd;
}

static PwResult file_set_fd(PwValuePtr self, int fd, bool move)
{
    _PwFile* f = get_file_data_ptr(self);

    if (f->fd != -1) {
        // fd already set
        return PwError(PW_ERROR_FD_ALREADY_SET);
    }
    f->fd = fd;
    f->is_external_fd = true;
    f->own_fd = move;
    return PwOK();
}

static PwResult file_get_name(PwValuePtr self)
{
    _PwFile* f = get_file_data_ptr(self);
    return pw_clone(&f->name);
}

static PwResult file_set_name(PwValuePtr self, PwValuePtr file_name)
{
    _PwFile* f = get_file_data_ptr(self);

    if (f->fd != -1 && !f->is_external_fd) {
        // not an externally set fd
        return PwError(PW_ERROR_CANT_SET_FILENAME);
    }

    // set file name
    pw_destroy(&f->name);
    f->name = pw_clone(file_name);

    return PwOK();
}

static PwResult file_set_nonblocking(PwValuePtr self, bool mode)
{
    _PwFile* f = get_file_data_ptr(self);

    if (f->fd == -1) {
        return PwError(PW_ERROR_FILE_CLOSED);
    }
    int flags = fcntl(f->fd, F_GETFL, 0);
    if (mode) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    if (fcntl(f->fd, F_SETFL, flags) == -1) {
        return PwErrno(errno);
    } else {
        return PwOK();
    }
}

static PwResult file_seek(PwValuePtr self, off_t offset, int whence)
{
    _PwFile* f = get_file_data_ptr(self);

    off_t pos = lseek(f->fd, offset, whence);
    if (pos == -1) {
        return PwErrno(errno);
    }
    return PwUnsigned(pos);
}

static PwResult file_tell(PwValuePtr self)
{
    _PwFile* f = get_file_data_ptr(self);

    off_t pos = lseek(f->fd, 0, SEEK_CUR);
    if (pos == -1) {
        return PwErrno(errno);
    }
    return PwUnsigned(pos);
}

static PwInterface_File file_interface = {
    .open     = file_open,
    .close    = file_close,
    .get_fd   = file_get_fd,
    .set_fd   = file_set_fd,
    .get_name = file_get_name,
    .set_name = file_set_name,
    .set_nonblocking = file_set_nonblocking,
    .seek     = file_seek,
    .tell     = file_tell
};


/****************************************************************
 * Reader interface for File
 */

static PwResult file_read(PwValuePtr self, void* buffer, unsigned buffer_size, unsigned* bytes_read)
{
    _PwFile* f = get_file_data_ptr(self);

    ssize_t result;
    do {
        result = read(f->fd, buffer, buffer_size);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        *bytes_read = 0;
        return PwErrno(errno);
    } else {
        *bytes_read = (unsigned) result;
        return PwOK();
    }
}

static PwInterface_Reader file_reader_interface = {
    .read = file_read
};


/****************************************************************
 * Writer interface for File
 */

static PwResult file_write(PwValuePtr self, void* data, unsigned size, unsigned* bytes_written)
{
    _PwFile* f = get_file_data_ptr(self);

    ssize_t result;
    do {
        result = write(f->fd, data, size);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        *bytes_written = 0;
        return PwErrno(errno);
    } else {
        *bytes_written = (unsigned) result;
        return PwOK();
    }
}

static PwInterface_Writer file_writer_interface = {
    .write = file_write
};


/****************************************************************
 * BufferedFile type
 */

PwTypeId PwTypeId_BufferedFile = 0;

static PwType buffered_file_type;

typedef struct {
    uint8_t* read_buffer;
    unsigned read_buffer_size;  // size of read_buffer
    unsigned read_data_size;    // size of data in read_buffer
    unsigned read_position;     // current position in read_buffer

    uint8_t* write_buffer;
    unsigned write_buffer_size; // size of write_buffer
    unsigned write_position;    // current position in write_buffer, also it's the size of data
    off_t    write_offset;      // file position to write the data;
                                // original position is preserved during write because it's used for reading

    bool is_pipe;

    // line reader data
    char8_t  partial_utf8[4];   // UTF-8 sequence may span adjacent reads
    unsigned partial_utf8_len;
    _PwValue pushback;          // for unread_line

    // line reader iterator data
    bool     iterating;         // indicates that iteration is in progress
    unsigned line_number;

} _PwBufferedFile;

// forward declaration
static void stop_read_lines(PwValuePtr self);

#define get_bfile_data_ptr(value)  ((_PwBufferedFile*) _pw_get_data_ptr((value), PwTypeId_BufferedFile))

static PwResult bfile_init(PwValuePtr self, void* ctor_args)
{
    PwBufferedFileCtorArgs* args= ctor_args;

    _PwBufferedFile* f = get_bfile_data_ptr(self);
    f->read_buffer_size  = align_unsigned_to_page(args->read_bufsize);
    f->write_buffer_size = align_unsigned_to_page(args->write_bufsize);

    if (f->read_buffer_size) {
        f->read_buffer = allocate(f->read_buffer_size, false);
        if (!f->read_buffer) {
            return PwOOM();
        }
    }
    if (f->write_buffer_size) {
        f->write_buffer = allocate(f->write_buffer_size, false);
        if (!f->write_buffer) {
            release((void**) &f->read_buffer, f->read_buffer_size);
            return PwOOM();
        }
    }
    f->pushback = PwNull();
    return PwOK();
}

static void bfile_fini(PwValuePtr self)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);
    if (f->read_buffer) {
        release((void**) &f->read_buffer, f->read_buffer_size);
    }
    if (f->write_buffer) {
        release((void**) &f->write_buffer, f->write_buffer_size);
    }
    pw_destroy(&f->pushback);
}

static void bfile_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    _pw_dump_struct_data(fp, self);
    basic_file_dump(self, fp);

    _PwBufferedFile* f = get_bfile_data_ptr(self);

    fprintf(fp, " read_buffer: %p", f->read_buffer);
    if (f->read_buffer) {
        fprintf(fp, " %u bytes", f->read_buffer_size);
    }
    fprintf(fp, " write_buffer: %p", f->write_buffer);
    if (f->write_buffer) {
        fprintf(fp, " %u bytes", f->write_buffer_size);
    }
    fputc('\n', fp);
}


/****************************************************************
 * BufferedFile interface
 */

unsigned PwInterfaceId_BufferedFile = 0;

static PwResult bfile_strict_write(PwValuePtr self, uint8_t* data, unsigned size, unsigned* bytes_written)
/*
 * Try writing exactly `size` bytes at write_offset, preserving current file position.
 */
{
    *bytes_written = 0;
    if (size == 0) {
        return PwOK();
    }
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    PwValue saved_pos = PwNull();
    PwValue write_pos = PwNull();
    if (!f->is_pipe) {
        saved_pos = file_tell(self);
        if (pw_errno(&saved_pos, ESPIPE)) {
            f->is_pipe = true;
        } else {
            pw_return_if_error(&saved_pos);
        }
        if (!f->is_pipe) {
            write_pos = file_seek(self, f->write_offset, SEEK_SET);
            pw_return_if_error(&write_pos);
        }
    }
    unsigned offset = 0;
    unsigned remaining = size;
    PwValue result = PwOK();
    do {
        unsigned n_written;
        pw_destroy(&result);
        result = file_write(self, data + offset, remaining, &n_written);
        *bytes_written += n_written;
        if (pw_error(&result)) {
            break;
        }

        offset += n_written;
        remaining -= n_written;

    } while (remaining);

    if (!f->is_pipe) {
        PwValue new_pos = file_tell(self);
        pw_return_if_error(&new_pos);

        f->write_offset = new_pos.unsigned_value;

        pw_expect_ok( file_seek(self, saved_pos.unsigned_value, SEEK_SET) );
    }
    return pw_move(&result);
}

static PwResult bfile_flush(PwValuePtr self)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->write_position == 0) {
        // nothing to write
        return PwOK();
    }

    if (f->iterating) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }

    unsigned bytes_written;
    PwValue result = bfile_strict_write(self, f->write_buffer, f->write_position, &bytes_written);
    if (pw_error(&result)) {
        unsigned remaining = f->write_position - bytes_written;
        if (remaining) {
            // move unwritten data to the beginning of `data`
            memmove(f->write_buffer, f->write_buffer + bytes_written, remaining);
        }
    }
    f->write_position = 0;

    return pw_move(&result);
}

static PwInterface_BufferedFile bfile_buffered_file_interface = {
    .flush = bfile_flush
};


/****************************************************************
 * File interface for BufferedFile
 */

static void reset_bfile_data(PwValuePtr self)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    f->read_data_size = 0;
    f->read_position = 0;
    f->write_position = 0;
    f->write_offset = 0;
    f->partial_utf8_len = 0;
    pw_destroy(&f->pushback);
}

// forward declaration
static void stop_read_lines(PwValuePtr self);


static void bfile_close(PwValuePtr self)
{
    stop_read_lines(self);
    bfile_flush(self);
    reset_bfile_data(self);
    file_close(self);
}

static PwResult bfile_set_fd(PwValuePtr self, int fd, bool move)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->iterating) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }
    reset_bfile_data(self);
    return file_set_fd(self, fd, move);
}

static PwResult bfile_set_name(PwValuePtr self, PwValuePtr file_name)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->iterating) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }
    reset_bfile_data(self);
    return file_set_name(self, file_name);
}

static PwResult bfile_seek(PwValuePtr self, off_t offset, int whence)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->iterating) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }

    // flush write buffer
    bfile_flush(self);

    // seek
    PwValue pos = file_seek(self, offset, whence);
    pw_return_if_error(&pos);

    // reset read buffer
    f->read_data_size = 0;
    f->read_position = 0;

    // set write position
    f->write_offset = pos.unsigned_value;

    return pw_move(&pos);
}

static PwInterface_File bfile_file_interface = {
    .close    = bfile_close,
    .set_fd   = bfile_set_fd,
    .set_name = bfile_set_name,
    .seek     = bfile_seek,
};


/****************************************************************
 * Reader interface for BufferedFile
 */

static PwResult bfile_read(PwValuePtr self, void* buffer, unsigned buffer_size, unsigned* bytes_read)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->iterating) {
        *bytes_read = 0;
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }

    if (f->read_buffer_size == 0) {
        // return directly from file
        return file_read(self, buffer, buffer_size, bytes_read);
    }

    if (f->read_position == f->read_data_size) {

        // no data in the read_buffer, read next portion
        f->read_position = 0;

        pw_expect_ok( file_read(self, f->read_buffer, f->read_buffer_size, &f->read_data_size) );
        if (f->read_data_size == 0) {
            return PwError(PW_ERROR_EOF);
        }
    }
    unsigned avail = f->read_data_size - f->read_position;
    unsigned size = (avail < buffer_size)? avail : buffer_size;
    memcpy(buffer, f->read_buffer + f->read_position, size);
    f->read_position += size;
    *bytes_read = size;
    return PwOK();
}

static PwInterface_Reader bfile_reader_interface = {
    .read = bfile_read
};


/****************************************************************
 * Writer interface for File
 */

static PwResult bfile_write(PwValuePtr self, void* data, unsigned size, unsigned* bytes_written)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    *bytes_written = 0;

    if (size == 0) {
        return PwOK();
    }

    if (f->iterating) {
        return PwError(PW_ERROR_ITERATION_IN_PROGRESS);
    }

    if (f->write_buffer_size == 0) {
        // write directly to file
        return file_write(self, data, size, bytes_written);
    }

    unsigned remaining_capacity = f->write_buffer_size - f->write_position;

    if (remaining_capacity) {
        // fill the write_buffer
        unsigned n = (size < remaining_capacity)? size : remaining_capacity;
        memcpy(f->write_buffer + f->write_position, data, n);
        f->write_position += n;
        *bytes_written += n;
        if (f->write_position < f->write_buffer_size) {
            // write_buffer is not full yet
            return PwOK();
        }
        size -= n;
        data = ((uint8_t*) data) + n;
    }
    // write_buffer is full, flush it
    pw_expect_ok( bfile_flush(self) );

    // write directly to file
    while (size >= f->write_buffer_size) {{
        unsigned n;
        PwValue status = bfile_strict_write(self, data, f->write_buffer_size, &n);
        *bytes_written += n;
        pw_return_if_error(&status);
        size -= n;
        data = ((uint8_t*) data) + n;
    }}

    if (size) {
        // move remaining data to the write_buffer
        memcpy(f->write_buffer, data, size);
        *bytes_written += size;
        f->write_position = size;
    }
    return PwOK();
}

static PwInterface_Writer bfile_writer_interface = {
    .write = bfile_write
};


/****************************************************************
 * LineReader interface methods
 */

static PwResult start_read_lines(PwValuePtr self)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->read_buffer_size == 0) {
        return PwError(PW_ERROR_UNBUFFERED_FILE);
    }

    f->partial_utf8_len = 0;
    f->line_number = 0;
    pw_destroy(&f->pushback);
    f->iterating = true;
    return PwOK();
}

static PwResult read_line_inplace(PwValuePtr self, PwValuePtr line)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->read_buffer_size == 0) {
        return PwError(PW_ERROR_UNBUFFERED_FILE);
    }

    pw_string_truncate(line, 0);

    if (pw_is_string(&f->pushback)) {
        pw_expect_true( pw_string_append(line, &f->pushback) );
        pw_destroy(&f->pushback);
        f->line_number++;
        return PwOK();
    }
    do {
        if (f->read_position == f->read_data_size) {

            // reached end of data scanning for line break

            f->read_position = 0;

            // read next chunk of file
            pw_expect_ok( file_read(self, f->read_buffer, f->read_buffer_size, &f->read_data_size) );
            if (f->read_data_size == 0) {
                return PwError(PW_ERROR_EOF);
            }

            if (f->partial_utf8_len) {
                // process partial UTF-8 sequence
                while (f->partial_utf8_len < 4) {

                    if (f->read_position == f->read_data_size) {
                        // premature end of file
                        // XXX warn?
                        return PwError(PW_ERROR_EOF);
                    }

                    char8_t c = f->read_buffer[f->read_position];
                    if (c < 0x80 || ((c & 0xC0) != 0x80)) {
                        // malformed UTF-8 sequence
                        break;
                    }
                    f->read_position++;
                    f->partial_utf8[f->partial_utf8_len++] = c;

                    char8_t* ptr = f->partial_utf8;
                    unsigned bytes_remaining = f->partial_utf8_len;
                    char32_t chr;
                    if (read_utf8_buffer(&ptr, &bytes_remaining, &chr)) {
                        if (chr != 0xFFFFFFFF) {
                            pw_expect_true( pw_string_append(line, chr) );
                        }
                        break;
                    }
                }
                f->partial_utf8_len = 0;
            }
        }

        char8_t* ptr = f->read_buffer + f->read_position;
        unsigned bytes_remaining = f->read_data_size - f->read_position;
        while (bytes_remaining) {
            char32_t chr;
            if (!read_utf8_buffer(&ptr, &bytes_remaining, &chr)) {
                break;
            }
            if (chr != 0xFFFFFFFF) {
                pw_expect_true( pw_string_append(line, chr) );
                if (chr == '\n') {
                    f->read_position = f->read_data_size - bytes_remaining;
                    f->line_number++;
                    return PwOK();
                }
            }
        }
        // move unprocessed data to partial_utf8
        while (bytes_remaining--) {
            f->partial_utf8[f->partial_utf8_len++] = *ptr++;
        }
        if (f->read_data_size < f->read_buffer_size) {
            // reached end of file
            f->read_position = 0;
            f->read_data_size = 0;
            f->line_number++;
            return PwOK();
        }

        // go read next chunk
        f->read_position = f->read_data_size;

    } while(true);
}

static PwResult read_line(PwValuePtr self)
{
    PwValue result = PwString();
    if (pw_ok(&result)) {
        pw_expect_ok( read_line_inplace(self, &result) );
    }
    return pw_move(&result);
}

static bool unread_line(PwValuePtr self, PwValuePtr line)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (pw_is_null(&f->pushback)) {
        f->pushback = pw_clone(line);
        f->line_number--;
        return true;
    } else {
        return false;
    }
}

static unsigned get_line_number(PwValuePtr self)
{
    return get_bfile_data_ptr(self)->line_number;
}

static void stop_read_lines(PwValuePtr self)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    f->iterating = false;
    pw_destroy(&f->pushback);
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
 * Initialization
 */

[[ gnu::constructor ]]
void _pw_init_file()
{
    // interface can be already registered
    // basically. interfaces can be registered by any type in any order
    if (PwInterfaceId_File == 0) {
        PwInterfaceId_File = pw_register_interface("File", PwInterface_File);
    }

    if (PwInterfaceId_BufferedFile == 0) {
        PwInterfaceId_BufferedFile = pw_register_interface("BufferedFile", PwInterface_BufferedFile);
    }

    if (PwTypeId_File == 0) {

        PwTypeId_File = pw_struct_subtype(
            &file_type, "File", PwTypeId_Struct, _PwFile,
            PwInterfaceId_File,   &file_interface,
            PwInterfaceId_Reader, &file_reader_interface,
            PwInterfaceId_Writer, &file_writer_interface,
            PwInterfaceId_LineReader, &line_reader_interface
        );
        file_type.hash     = file_hash;
        file_type.deepcopy = file_deepcopy;
        file_type.dump     = file_dump;
        file_type.init     = file_init;
        file_type.fini     = file_fini;


        PwTypeId_BufferedFile = pw_struct_subtype(
            &buffered_file_type, "BufferedFile", PwTypeId_File, _PwBufferedFile,
            PwInterfaceId_File,         &bfile_file_interface,
            PwInterfaceId_BufferedFile, &bfile_buffered_file_interface,
            PwInterfaceId_Reader,       &bfile_reader_interface,
            PwInterfaceId_Writer,       &bfile_writer_interface,
            PwInterfaceId_LineReader,   &line_reader_interface
        );
        buffered_file_type.dump = bfile_dump;
        buffered_file_type.init = bfile_init;
        buffered_file_type.fini = bfile_fini;
    }
}

/****************************************************************
 * Shorthand functions
 */

PwResult _pw_file_open(PwValuePtr file_name, int flags, mode_t mode)
{
    PwBufferedFileCtorArgs args = {
        .read_bufsize = sys_page_size,
        .write_bufsize = sys_page_size
    };
    PwValue file = pw_create2(PwTypeId_BufferedFile, &args);
    pw_return_if_error(&file);

    pw_expect_ok( pw_interface(file.type_id, File)->open(&file, file_name, flags, mode) );

    return pw_move(&file);
}

PwResult pw_file_from_fd(int fd, bool take_ownership)
{
    PwBufferedFileCtorArgs args = {
        .read_bufsize = sys_page_size,
        .write_bufsize = sys_page_size
    };
    PwValue file = pw_create2(PwTypeId_BufferedFile, &args);
    pw_return_if_error(&file);

    pw_expect_ok( pw_interface(file.type_id, File)->set_fd(&file, fd, take_ownership) );

    return pw_move(&file);
}

PwResult pw_write_exact(PwValuePtr file, void* data, unsigned size)
{
    unsigned bytes_written;
    PwValue status = pw_write(file, data, size, &bytes_written);
    pw_return_if_error(&status);
    if (bytes_written != size) {
        return PwError(PW_ERROR_WRITE);
    }
    return PwOK();
}

/****************************************************************
 * Miscellaneous functions
 */

static PwResult get_file_size(char* file_name)
{
    struct stat statbuf;
    if (stat(file_name, &statbuf) == -1) {
        return PwErrno(errno);
    }
    if ( ! (statbuf.st_mode & S_IFREG)) {
        return PwError(PW_ERROR_NOT_REGULAR_FILE);
    }
    return PwUnsigned(statbuf.st_size);
}

PwResult pw_file_size(PwValuePtr file_name)
{
    if (pw_is_charptr(file_name)) {
        switch (file_name->charptr_subtype) {
            case PW_CHARPTR:
                return get_file_size((char*) file_name->charptr);

            case PW_CHAR32PTR: {
                PW_CSTRING_LOCAL(fname, file_name);
                return get_file_size(fname);
            }
            default:
                _pw_panic_bad_charptr_subtype(file_name);
        }
    } else {
        pw_assert_string(file_name);
        PW_CSTRING_LOCAL(fname, file_name);
        return get_file_size(fname);
    }
}

/****************************************************************
 * Path functions, probably should be separated
 */

PwResult pw_basename(PwValuePtr filename)
{
    PwValue parts = PwNull();
    if (pw_is_charptr(filename)) {
        PwValue f = pw_clone(filename);
        parts = pw_string_rsplit_chr(&f, '/', 1);
    } else {
        pw_expect(String, filename);
        parts = pw_string_rsplit_chr(filename, '/', 1);
    }
    return pw_array_item(&parts, -1);
}

PwResult pw_dirname(PwValuePtr filename)
{
    PwValue parts = PwNull();
    if (pw_is_charptr(filename)) {
        PwValue f = pw_clone(filename);
        parts = pw_string_rsplit_chr(&f, '/', 1);
    } else {
        pw_expect(String, filename);
        parts = pw_string_rsplit_chr(filename, '/', 1);
    }
    return pw_array_item(&parts, 0);
}

PwResult _pw_path_v(...)
{
    PwValue parts = pw_create(PwTypeId_Array);
    va_list ap;
    va_start(ap);
    for (;;) {{
        PwValue arg = va_arg(ap, _PwValue);
        if (pw_is_status(&arg)) {
            if (pw_va_end(&arg)) {
                break;
            }
            _pw_destroy_args(ap);
            va_end(ap);
            return pw_move(&arg);
        }
        if (pw_is_string(&arg) || pw_is_charptr(&arg)) {
            PwValue status = pw_array_append(&parts, &arg);
            if (pw_error(&status)) {
                _pw_destroy_args(ap);
                va_end(ap);
                return pw_move(&status);
            }
        }
    }}
    va_end(ap);
    return pw_array_join('/', &parts);
}

PwResult _pw_path_p(...)
{
    PwValue parts = pw_create(PwTypeId_Array);
    va_list ap;
    va_start(ap);
    for (;;) {
        PwValuePtr arg = va_arg(ap, PwValuePtr);
        if (!arg) {
            break;
        }
        if (pw_error(arg)) {
            va_end(ap);
            return pw_clone(arg);
        }
        if (pw_is_string(arg) || pw_is_charptr(arg)) {
            PwValue status = pw_array_append(&parts, arg);
            if (pw_error(&status)) {
                va_end(ap);
                return pw_move(&status);
            }
        }
    }
    va_end(ap);
    return pw_array_join('/', &parts);
}
