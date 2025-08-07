#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "include/pw.h"
#include "include/pw_utf.h"
#include "src/pw_interfaces_internal.h"

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
[[nodiscard]] static bool file_close(PwValuePtr self);


[[nodiscard]] static bool file_init(PwValuePtr self, void* ctor_args)
{
    // ctor_args is unused

    _PwFile* f = get_file_data_ptr(self);
    f->fd = -1;
    f->name = PwNull();
    return true;
}

static void file_fini(PwValuePtr self)
{
    if (!file_close(self)) {
        fprintf(stderr, "Failed %s\n", __func__);
        pw_print_status(stderr, &current_task->status);
    }
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

[[nodiscard]] static bool file_deepcopy(PwValuePtr self, PwValuePtr result)
{
    // XXX duplicate fd?
    pw_set_status(PwStatus(PW_ERROR_NOT_IMPLEMENTED));
    return false;
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

[[nodiscard]] static bool file_open(PwValuePtr self, PwValuePtr file_name, int flags, mode_t mode)
{
    _PwFile* f = get_file_data_ptr(self);

    if (f->fd != -1) {
        pw_set_status(PwStatus(PW_ERROR_FILE_ALREADY_OPENED));
        return false;
    }

    pw_assert_string(file_name);
    PW_CSTRING_LOCAL(fname, file_name);
    do {
        f->fd = open(fname, flags, mode);
    } while (f->fd == -1 && errno == EINTR);

    if (f->fd == -1) {
        f->error = errno;
        pw_set_status(PwErrno(errno));
        return false;
    }

    pw_clone2(file_name, &f->name);
    f->is_external_fd = false;
    f->own_fd = true;

    return true;
}

[[ nodiscard]] static bool file_close(PwValuePtr self)
{
    _PwFile* f = get_file_data_ptr(self);
    bool ret = true;
    if (f->fd != -1 && f->own_fd) {
        int result;
        do {
            result = close(f->fd);
        } while (result == -1 && errno == EINTR);
        ret = result == 0;
    }
    f->fd = -1;
    f->error = 0;
    pw_destroy(&f->name);
    return ret;
}

[[nodiscard]] static int file_get_fd(PwValuePtr self)
{
    return get_file_data_ptr(self)->fd;
}

[[nodiscard]] static bool file_set_fd(PwValuePtr self, int fd, bool move)
{
    _PwFile* f = get_file_data_ptr(self);

    if (f->fd != -1) {
        // fd already set
        pw_set_status(PwStatus(PW_ERROR_FD_ALREADY_SET));
        return false;
    }
    f->fd = fd;
    f->is_external_fd = true;
    f->own_fd = move;
    return true;
}

[[nodiscard]] static bool file_get_name(PwValuePtr self, PwValuePtr result)
{
    _PwFile* f = get_file_data_ptr(self);
    pw_clone2(&f->name, result);
    return true;
}

[[nodiscard]] static bool file_set_name(PwValuePtr self, PwValuePtr file_name)
{
    _PwFile* f = get_file_data_ptr(self);

    if (f->fd != -1 && !f->is_external_fd) {
        // not an externally set fd
        pw_set_status(PwStatus(PW_ERROR_CANT_SET_FILENAME));
        return false;
    }

    pw_clone2(file_name, &f->name);
    return true;
}

[[nodiscard]] static bool file_set_nonblocking(PwValuePtr self, bool mode)
{
    _PwFile* f = get_file_data_ptr(self);

    if (f->fd == -1) {
        pw_set_status(PwStatus(PW_ERROR_FILE_CLOSED));
        return false;
    }
    int flags = fcntl(f->fd, F_GETFL, 0);
    if (mode) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    if (fcntl(f->fd, F_SETFL, flags) == -1) {
        pw_set_status(PwErrno(errno));
        return false;
    }
    return true;
}

[[nodiscard]] static bool file_seek(PwValuePtr self, off_t offset, int whence, off_t* position)
{
    _PwFile* f = get_file_data_ptr(self);
    off_t pos = lseek(f->fd, offset, whence);
    if (pos == -1) {
        pw_set_status(PwErrno(errno));
        return false;
    }
    if (position) {
        *position = pos;
    }
    return true;
}

[[nodiscard]] static bool file_tell(PwValuePtr self, off_t* position)
{
    _PwFile* f = get_file_data_ptr(self);

    *position = lseek(f->fd, 0, SEEK_CUR);
    if (*position == -1) {
        pw_set_status(PwErrno(errno));
        return false;
    }
    return true;
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

[[nodiscard]] static bool file_read(PwValuePtr self, void* buffer, unsigned buffer_size, unsigned* bytes_read)
{
    _PwFile* f = get_file_data_ptr(self);

    ssize_t result;
    do {
        result = read(f->fd, buffer, buffer_size);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        *bytes_read = 0;
        pw_set_status(PwErrno(errno));
        return false;
    }
    *bytes_read = (unsigned) result;
    return true;
}

static PwInterface_Reader file_reader_interface = {
    .read = file_read
};


/****************************************************************
 * Writer interface for File
 */

[[nodiscard]] static bool file_write(PwValuePtr self, void* data, unsigned size, unsigned* bytes_written)
{
    _PwFile* f = get_file_data_ptr(self);

    unsigned written = 0;
    while (written < size) {
        ssize_t n;
        do {
            n = write(f->fd, ((uint8_t*) data) + written, size - written);
        } while (n < 0 && errno == EINTR);

        if (n < 0) {
            if (bytes_written) {
                *bytes_written = written;
            }
            pw_set_status(PwErrno(errno));
            return false;
        }
        written += (unsigned) n;
    }
    if (bytes_written) {
        *bytes_written = written;
    }
    return true;
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

    // line reader data
    char8_t  partial_utf8[8];   // UTF-8 sequence may span adjacent reads, the buffer size is for surrogate pair
    unsigned partial_utf8_len;
    _PwValue pushback;          // for unread_line

    // line reader iterator data
    bool     iterating;         // indicates that iteration is in progress
    unsigned line_number;

} _PwBufferedFile;

// forward declaration
static void bfile_stop_read_lines(PwValuePtr self);

#define get_bfile_data_ptr(value)  ((_PwBufferedFile*) _pw_get_data_ptr((value), PwTypeId_BufferedFile))

[[nodiscard]] static bool bfile_init(PwValuePtr self, void* ctor_args)
{
    PwBufferedFileCtorArgs* args = ctor_args;

    _PwBufferedFile* f = get_bfile_data_ptr(self);
    f->read_buffer_size  = align_unsigned_to_page(args->read_bufsize);
    f->write_buffer_size = align_unsigned_to_page(args->write_bufsize);

    if (f->read_buffer_size) {
        f->read_buffer = allocate(f->read_buffer_size, false);
        if (!f->read_buffer) {
            pw_set_status(PwStatus(PW_ERROR_OOM));
            return false;
        }
    }
    if (f->write_buffer_size) {
        f->write_buffer = allocate(f->write_buffer_size, false);
        if (!f->write_buffer) {
            release((void**) &f->read_buffer, f->read_buffer_size);
            pw_set_status(PwStatus(PW_ERROR_OOM));
            return false;
        }
    }
    f->pushback = PwNull();
    return true;
}

[[nodiscard]] static bool bfile_close(PwValuePtr self);  // forward declaration for bfile_fini

static void bfile_fini(PwValuePtr self)
{
    if (!bfile_close(self)) {
        fprintf(stderr, "Failed %s\n", __func__);
        pw_print_status(stderr, &current_task->status);
    }
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


[[nodiscard]] static bool bfile_flush(PwValuePtr self)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->write_position == 0) {
        // nothing to write
        return true;
    }

    if (f->iterating) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }

    unsigned bytes_written;
    if (file_write(self, f->write_buffer, f->write_position, &bytes_written)) {
        // success, all data is written
        f->write_position = 0;
        return true;
    }
    f->write_position -= bytes_written;
    if (f->write_position) {
        // move unwritten data to the beginning of `data`
        memmove(f->write_buffer, f->write_buffer + bytes_written, f->write_position);
    }
    return false;
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
    f->partial_utf8_len = 0;
    pw_destroy(&f->pushback);
}

[[nodiscard]] static bool bfile_close(PwValuePtr self)
{
    bfile_stop_read_lines(self);
    bool flush_result = bfile_flush(self);
    reset_bfile_data(self);
    return file_close(self) && flush_result;  // XXX if both flush and close are failed, flush status is lost
}

[[nodiscard]] static bool bfile_set_fd(PwValuePtr self, int fd, bool move)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->iterating) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }
    reset_bfile_data(self);
    return file_set_fd(self, fd, move);
}

[[nodiscard]] static bool bfile_set_name(PwValuePtr self, PwValuePtr file_name)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->iterating) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }
    reset_bfile_data(self);
    return file_set_name(self, file_name);
}

[[nodiscard]] static bool bfile_seek(PwValuePtr self, off_t offset, int whence, off_t* position)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->iterating) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }
    // reset read buffer
    f->read_data_size = 0;
    f->read_position = 0;
    // flush write buffer
    if (!bfile_flush(self)) {
        return false;
    }
    // seek
    return file_seek(self, offset, whence, position);
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

[[nodiscard]] static bool bfile_read(PwValuePtr self, void* buffer, unsigned buffer_size, unsigned* bytes_read)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->iterating) {
        *bytes_read = 0;
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
    }

    if (f->read_buffer_size == 0) {
        // return directly from file
        return file_read(self, buffer, buffer_size, bytes_read);
    }

    if (f->read_position == f->read_data_size) {

        // no data in the read_buffer, read next portion
        f->read_position = 0;

        if (!file_read(self, f->read_buffer, f->read_buffer_size, &f->read_data_size)) {
            return false;
        }
        if (f->read_data_size == 0) {
            pw_set_status(PwStatus(PW_ERROR_EOF));
            return false;
        }
    }
    unsigned avail = f->read_data_size - f->read_position;
    unsigned size = (avail < buffer_size)? avail : buffer_size;
    memcpy(buffer, f->read_buffer + f->read_position, size);
    f->read_position += size;
    *bytes_read = size;
    return true;
}

static PwInterface_Reader bfile_reader_interface = {
    .read = bfile_read
};


/****************************************************************
 * Writer interface for BufferedFile
 */

[[nodiscard]] static bool bfile_write(PwValuePtr self, void* data, unsigned size, unsigned* bytes_written)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    *bytes_written = 0;

    if (size == 0) {
        return true;
    }

    if (f->iterating) {
        pw_set_status(PwStatus(PW_ERROR_ITERATION_IN_PROGRESS));
        return false;
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
            return true;
        }
        size -= n;
        data = ((uint8_t*) data) + n;
    }
    // write_buffer is full, flush it
    if (!bfile_flush(self)) {
        return false;
    }
    // write directly to file
    while (size >= f->write_buffer_size) {{
        unsigned n;
        bool ret = file_write(self, data, f->write_buffer_size, &n);
        *bytes_written += n;
        if (!ret) {
            return false;
        }
        size -= n;
        data = ((uint8_t*) data) + n;
    }}

    if (size) {
        // move remaining data to the write_buffer
        memcpy(f->write_buffer, data, size);
        *bytes_written += size;
        f->write_position = size;
    }
    return true;
}

static PwInterface_Writer bfile_writer_interface = {
    .write = bfile_write
};


/****************************************************************
 * LineReader interface methods for buffered file
 */

[[nodiscard]] static bool bfile_start_read_lines(PwValuePtr self)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->read_buffer_size == 0) {
        pw_set_status(PwStatus(PW_ERROR_UNBUFFERED_FILE));
        return false;
    }

    f->partial_utf8_len = 0;
    f->line_number = 0;
    pw_destroy(&f->pushback);
    f->iterating = true;
    return true;
}

[[nodiscard]] static bool bfile_read_line_inplace(PwValuePtr self, PwValuePtr line)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->read_buffer_size == 0) {
        pw_set_status(PwStatus(PW_ERROR_UNBUFFERED_FILE));
        return false;
    }
    if (!pw_string_truncate(line, 0)) {
        return false;
    }
    if (pw_is_string(&f->pushback)) {
        if (!pw_string_append(line, &f->pushback)) {
            return false;
        }
        pw_destroy(&f->pushback);
        f->line_number++;
        return true;
    }
    do {
        if (f->read_position == f->read_data_size) {

            // reached end of data scanning for line break

            f->read_position = 0;

            // read next chunk of file
            if (!file_read(self, f->read_buffer, f->read_buffer_size, &f->read_data_size)) {
                return false;
            }
            if (f->read_data_size == 0) {
                pw_set_status(PwStatus(PW_ERROR_EOF));
                return false;
            }

            if (f->partial_utf8_len) {
                // process partial UTF-8 sequence
                while (f->partial_utf8_len < sizeof(f->partial_utf8)) {

                    if (f->read_position == f->read_data_size) {
                        // premature end of file
                        // XXX warn?
                        pw_set_status(PwStatus(PW_ERROR_EOF));
                        return false;
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
                    if (_pw_decode_utf8_buffer(&ptr, &bytes_remaining, &chr)) {
                        if (chr != 0xFFFFFFFF) {
                            if (!pw_string_append(line, chr)) {
                                return false;
                            }
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
            if (!_pw_decode_utf8_buffer(&ptr, &bytes_remaining, &chr)) {
                break;
            }
            if (chr != 0xFFFFFFFF) {
                if (!pw_string_append(line, chr)) {
                    return false;
                }
                if (chr == '\n') {
                    f->read_position = f->read_data_size - bytes_remaining;
                    f->line_number++;
                    return true;
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
            return true;
        }

        // go read next chunk
        f->read_position = f->read_data_size;

    } while(true);
}

[[nodiscard]] static bool bfile_read_line(PwValuePtr self, PwValuePtr result)
{
    PwValue line = PW_STRING("");
    if (!bfile_read_line_inplace(self, &line)) {
        return false;
    }
    pw_move(&line, result);
    return true;
}

[[nodiscard]] static bool bfile_unread_line(PwValuePtr self, PwValuePtr line)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (pw_is_null(&f->pushback)) {
        __pw_clone(line, &f->pushback);  // puchback is already Null, so use __pw_clone here
        f->line_number--;
        return true;
    } else {
        return false;
    }
}

[[nodiscard]] static unsigned bfile_get_line_number(PwValuePtr self)
{
    return get_bfile_data_ptr(self)->line_number;
}

static void bfile_stop_read_lines(PwValuePtr self)
{
    _PwBufferedFile* f = get_bfile_data_ptr(self);

    f->iterating = false;
    pw_destroy(&f->pushback);
}

static PwInterface_LineReader bfile_line_reader_interface = {
    .start             = bfile_start_read_lines,
    .read_line         = bfile_read_line,
    .read_line_inplace = bfile_read_line_inplace,
    .get_line_number   = bfile_get_line_number,
    .unread_line       = bfile_unread_line,
    .stop              = bfile_stop_read_lines
};


/****************************************************************
 * Append interface methods for unbuffered file
 */

static bool file_append_string_data(PwValuePtr self, uint8_t* start_ptr, uint8_t* end_ptr, uint8_t char_size)
{
    if (start_ptr >= end_ptr) {
        return true;
    }
    if (char_size < 2) {
        // write ASCII and UTF-8 directly to file
        return file_write(self, start_ptr, end_ptr - start_ptr, nullptr);
    }

    // convert wide chars to UTF-8

    uint8_t buffer[1024];
    unsigned n = 0;

    while (start_ptr < end_ptr) {
        char32_t codepoint = _pw_get_char(start_ptr, char_size);
        n += pw_char32_to_utf8(codepoint, (char*) &buffer[n]);
        if (n > sizeof(buffer) - 4) {
            if (!file_write(self, buffer, n, nullptr)) {
                return false;
            }
            n = 0;
        }
    }
    if (n) {
        if (!file_write(self, buffer, n, nullptr)) {
            return false;
        }
    }
    return true;
}

static bool file_append(PwValuePtr self, PwValuePtr value)
{
    pw_expect(String, value);

    uint8_t* end_ptr;
    uint8_t* start_ptr = _pw_string_start_end(value, &end_ptr);
    return file_append_string_data(self, start_ptr, end_ptr, value->char_size);
}

static PwInterface_Append file_append_interface = {
    .append = file_append,
    .append_string_data = file_append_string_data

};

/****************************************************************
 * Append interface methods for buffered file
 */

static bool bfile_append_string_data(PwValuePtr self, uint8_t* start_ptr, uint8_t* end_ptr, uint8_t char_size)
{
    if (start_ptr >= end_ptr) {
        return true;
    }

    if (char_size < 2) {
        // write ASCII and UTF-8 directly to file
        return bfile_write(self, start_ptr, end_ptr - start_ptr, nullptr);
    }

    // convert wide chars to UTF-8

    _PwBufferedFile* f = get_bfile_data_ptr(self);

    if (f->write_buffer_size == 0) {
        // invoke unbuffered version
        return file_append_string_data(self, start_ptr, end_ptr, char_size);
    }

    while (start_ptr < end_ptr) {
        unsigned remaining = f->write_buffer_size - f->write_position;
        if (remaining < 4) {
            if (!bfile_flush(self)) {
                return false;
            }
        }
        char32_t codepoint = _pw_get_char(start_ptr, char_size);
        start_ptr += char_size;
        f->write_position += pw_char32_to_utf8(codepoint, (char*) &f->write_buffer[f->write_position]);
    }
    return true;
}

static bool bfile_append(PwValuePtr self, PwValuePtr value)
{
    pw_expect(String, value);

    uint8_t* end_ptr;
    uint8_t* start_ptr = _pw_string_start_end(value, &end_ptr);
    return bfile_append_string_data(self, start_ptr, end_ptr, value->char_size);
}

static PwInterface_Append bfile_append_interface = {
    .append = bfile_append,
    .append_string_data = bfile_append_string_data
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
            PwInterfaceId_Append, &file_append_interface
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
            PwInterfaceId_LineReader,   &bfile_line_reader_interface,
            PwInterfaceId_Append,       &bfile_append_interface
        );
        buffered_file_type.dump = bfile_dump;
        buffered_file_type.init = bfile_init;
        buffered_file_type.fini = bfile_fini;
    }
}

/****************************************************************
 * Shorthand functions
 */

[[nodiscard]] bool _pw_file_open(PwValuePtr file_name, int flags, mode_t mode, PwValuePtr result)
{
    PwBufferedFileCtorArgs args = {
        .read_bufsize = sys_page_size,
        .write_bufsize = sys_page_size
    };
    if (!pw_create2(PwTypeId_BufferedFile, &args, result)) {
        return false;
    }
    return pw_interface(result->type_id, File)->open(result, file_name, flags, mode);
}

[[nodiscard]] bool pw_file_from_fd(int fd, bool take_ownership, PwValuePtr result)
{
    PwBufferedFileCtorArgs args = {
        .read_bufsize = sys_page_size,
        .write_bufsize = sys_page_size
    };
    if (!pw_create2(PwTypeId_BufferedFile, &args, result)) {
        return false;
    }
    return pw_interface(result->type_id, File)->set_fd(result, fd, take_ownership);
}

/****************************************************************
 * Miscellaneous functions
 */

[[nodiscard]] bool pw_file_size(PwValuePtr file_name, off_t* result)
{
    pw_assert_string(file_name);
    PW_CSTRING_LOCAL(fname, file_name);

    struct stat statbuf;

    if (stat(fname, &statbuf) == -1) {
        pw_set_status(PwErrno(errno));
        return false;
    }
    if ( ! (statbuf.st_mode & S_IFREG)) {
        pw_set_status(PwStatus(PW_ERROR_NOT_REGULAR_FILE));
        return false;
    }
    *result = statbuf.st_size;
    return true;
}

/****************************************************************
 * Path functions, probably should be separated
 */

[[nodiscard]] bool pw_basename(PwValuePtr filename, PwValuePtr result)
{
    pw_expect(String, filename);
    PwValue parts = PW_NULL;
    if (!pw_string_rsplit_chr(filename, '/', 1, &parts)) {
        return false;
    }
    return pw_array_item(&parts, -1, result);
}

[[nodiscard]] bool pw_dirname(PwValuePtr filename, PwValuePtr result)
{
    pw_expect(String, filename);
    PwValue parts = PW_NULL;
    if (!pw_string_rsplit_chr(filename, '/', 1, &parts)) {
        return false;
    }
    return pw_array_item(&parts, 0, result);
}

[[nodiscard]] bool _pw_path_va(PwValuePtr result, ...)
{
    PwValue parts = PW_NULL;
    if (!pw_create_array(&parts)) {
        return false;
    }
    va_list ap;
    va_start(ap);
    for (;;) {{
        PwValue arg = va_arg(ap, _PwValue);
        if (pw_is_status(&arg)) {
            if (pw_is_va_end(&arg)) {
                break;
            }
            _pw_destroy_args(ap);
            va_end(ap);
            pw_set_status(pw_clone(&arg));
            return false;
        }
        if (pw_is_string(&arg)) {
            if (!pw_array_append(&parts, &arg)) {
                _pw_destroy_args(ap);
                va_end(ap);
                return false;
            }
        }
    }}
    va_end(ap);
    return pw_array_join('/', &parts, result);
}
