#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "include/pw.h"
#include "src/pw_interfaces_internal.h"
#include "src/pw_string_internal.h"
#include "src/pw_struct_internal.h"

#define LINE_READER_BUFFER_SIZE  4096  // typical filesystem block size

typedef struct {
    int fd;               // file descriptor
    bool is_external_fd;  // fd is set by `set_fd` and should not be closed
    int error;            // errno, set by `open`
    _PwValue name;

    // line reader data
    // XXX okay for now, revise later
    char8_t* buffer;    // this has fixed LINE_READER_BUFFER_SIZE
    unsigned position;  // current position in the buffer when scanning for line break
    unsigned data_size; // size of data in the buffer
    char8_t  partial_utf8[4];  // UTF-8 sequence may span adjacent reads
    unsigned partial_utf8_len;
    _PwValue pushback;  // for unread_line
    unsigned line_number;
} _PwFile;

#define get_data_ptr(value)  ((_PwFile*) _pw_get_data_ptr((value), PwTypeId_File))

// forward declarations
static void file_close(PwValuePtr self);
static void stop_read_lines(PwValuePtr self);

/****************************************************************
 * File type
 */

PwTypeId PwTypeId_File = 0;

static PwType file_type;

static PwResult file_init(PwValuePtr self, void* ctor_args)
{
    // XXX not using ctor_args for now

    _PwFile* f = get_data_ptr(self);
    f->fd = -1;
    f->name = PwNull();
    f->pushback = PwNull();
    return PwOK();
}

static void file_fini(PwValuePtr self)
{
    file_close(self);
}

static void file_hash(PwValuePtr self, PwHashContext* ctx)
{
    // it's not a hash of entire file content!

    _PwFile* f = get_data_ptr(self);

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

static void file_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _PwFile* f = get_data_ptr(self);

    _pw_dump_start(fp, self, first_indent);
    _pw_dump_struct_data(fp, self);

    if (pw_is_string(&f->name)) {
        PW_CSTRING_LOCAL(file_name, &f->name);
        fprintf(fp, " name: %s", file_name);
    } else {
        fprintf(fp, " name: Null");
    }
    fprintf(fp, " fd: %d", f->fd);
    if (f->is_external_fd) {
        fprintf(fp, " (external)");
    }
    fputc('\n', fp);
}

/****************************************************************
 * File interface
 */

unsigned PwInterfaceId_File = 0;

static PwResult file_open(PwValuePtr self, PwValuePtr file_name, int flags, mode_t mode)
{
    _PwFile* f = get_data_ptr(self);

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
    f->line_number = 0;

    pw_destroy(&f->pushback);

    return PwOK();
}

static void file_close(PwValuePtr self)
{
    // XXX check if iteration is in progress

    _PwFile* f = get_data_ptr(self);

    stop_read_lines(self);

    if (f->fd != -1 && !f->is_external_fd) {
        close(f->fd);
    }
    f->fd = -1;
    f->error = 0;
    pw_destroy(&f->name);
}

static int file_get_fd(PwValuePtr self)
{
    return get_data_ptr(self)->fd;
}

static bool file_set_fd(PwValuePtr self, int fd)
{
    _PwFile* f = get_data_ptr(self);

    // XXX check if iteration is in progress

    if (f->fd != -1) {
        // fd already set
        return false;
    }
    f->fd = fd;
    f->is_external_fd = true;
    f->line_number = 0;
    pw_destroy(&f->pushback);
    return true;
}

static PwResult file_get_name(PwValuePtr self)
{
    _PwFile* f = get_data_ptr(self);
    return pw_clone(&f->name);
}

static bool file_set_name(PwValuePtr self, PwValuePtr file_name)
{
    _PwFile* f = get_data_ptr(self);

    // XXX check if iteration is in progress

    if (f->fd != -1 && !f->is_external_fd) {
        // not an externally set fd
        return false;
    }

    // set file name
    pw_destroy(&f->name);
    f->name = pw_clone(file_name);

    return true;
}

static PwResult file_set_nonblocking(PwValuePtr self, bool mode)
{
    _PwFile* f = get_data_ptr(self);

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

static PwInterface_File file_interface = {
    .open     = file_open,
    .close    = file_close,
    .get_fd   = file_get_fd,
    .set_fd   = file_set_fd,
    .get_name = file_get_name,
    .set_name = file_set_name,
    .set_nonblocking = file_set_nonblocking
};


/****************************************************************
 * Reader interface
 */

static PwResult file_read(PwValuePtr self, void* buffer, unsigned buffer_size, unsigned* bytes_read)
{
    _PwFile* f = get_data_ptr(self);

    // XXX check if iteration is in progress

    ssize_t result;
    do {
        result = read(f->fd, buffer, buffer_size);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
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
 * Writer interface
 */

static PwResult file_write(PwValuePtr self, void* data, unsigned size, unsigned* bytes_written)
{
    _PwFile* f = get_data_ptr(self);

    // XXX check if iteration is in progress

    ssize_t result;
    do {
        result = write(f->fd, data, size);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
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
 * LineReader interface methods
 */

static PwResult start_read_lines(PwValuePtr self)
{
    _PwFile* f = get_data_ptr(self);

    pw_destroy(&f->pushback);

    if (f->buffer == nullptr) {
        f->buffer = allocate(LINE_READER_BUFFER_SIZE, false);
        if (!f->buffer) {
            return PwOOM();
        }
    }
    f->partial_utf8_len = 0;

    // the following settings will force line_reader to read next chunk of data immediately:
    f->position = LINE_READER_BUFFER_SIZE;
    f->data_size = LINE_READER_BUFFER_SIZE;

    // reset file position
    if (lseek(f->fd, 0, SEEK_SET) == -1) {
        return PwErrno(errno);
    }
    f->line_number = 0;
    return PwOK();
}

static PwResult read_line_inplace(PwValuePtr self, PwValuePtr line)
{
    _PwFile* f = get_data_ptr(self);

    pw_string_truncate(line, 0);

    if (f->buffer == nullptr) {
        pw_expect_ok( start_read_lines(self) );
    }

    if (pw_is_string(&f->pushback)) {
        if (!pw_string_append(line, &f->pushback)) {
            return PwOOM();
        }
        pw_destroy(&f->pushback);
        f->line_number++;
        return PwOK();
    }

    if ( ! (f->position || f->data_size)) {
        // EOF state
        // XXX warn if f->partial_utf8_len != 0
        return PwError(PW_ERROR_EOF);
    }

    do {
        if (f->position == f->data_size) {

            // reached end of data scanning for line break

            f->position = 0;

            // read next chunk of file
            {
                pw_expect_ok( file_read(self, f->buffer, LINE_READER_BUFFER_SIZE, &f->data_size) );
                if (f->data_size == 0) {
                    return PwError(PW_ERROR_EOF);
                }
            }

            if (f->partial_utf8_len) {
                // process partial UTF-8 sequence
                while (f->partial_utf8_len < 4) {

                    if (f->position == f->data_size) {
                        // premature end of file
                        // XXX warn?
                        return PwError(PW_ERROR_EOF);
                    }

                    char8_t c = f->buffer[f->position];
                    if (c < 0x80 || ((c & 0xC0) != 0x80)) {
                        // malformed UTF-8 sequence
                        break;
                    }
                    f->position++;
                    f->partial_utf8[f->partial_utf8_len++] = c;

                    char8_t* ptr = f->partial_utf8;
                    unsigned bytes_remaining = f->partial_utf8_len;
                    char32_t chr;
                    if (read_utf8_buffer(&ptr, &bytes_remaining, &chr)) {
                        if (chr != 0xFFFFFFFF) {
                            if (!pw_string_append(line, chr)) {
                                return PwOOM();
                            }
                        }
                        break;
                    }
                }
                f->partial_utf8_len = 0;
            }
        }

        char8_t* ptr = f->buffer + f->position;
        unsigned bytes_remaining = f->data_size - f->position;
        while (bytes_remaining) {
            char32_t chr;
            if (!read_utf8_buffer(&ptr, &bytes_remaining, &chr)) {
                break;
            }
            if (chr != 0xFFFFFFFF) {
                if (!pw_string_append(line, chr)) {
                    return PwOOM();
                }
                if (chr == '\n') {
                    f->position = f->data_size - bytes_remaining;
                    f->line_number++;
                    return PwOK();
                }
            }
        }
        // move unprocessed data to partial_utf8
        while (bytes_remaining--) {
            f->partial_utf8[f->partial_utf8_len++] = *ptr++;
        }
        if (f->data_size < LINE_READER_BUFFER_SIZE) {
            // reached end of file, set EOF state
            f->position = 0;
            f->data_size = 0;
            f->line_number++;
            return PwOK();
        }

        // go read next chunk
        f->position = f->data_size;

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
    _PwFile* f = get_data_ptr(self);

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
    return get_data_ptr(self)->line_number;
}

static void stop_read_lines(PwValuePtr self)
{
    _PwFile* f = get_data_ptr(self);

    release((void**) &f->buffer, LINE_READER_BUFFER_SIZE);
    f->buffer = nullptr;
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
static void init_globals()
{
    // interface can be already registered
    // basically. interfaces can be registered by any type in any order
    if (PwInterfaceId_File == 0) {
        PwInterfaceId_File = pw_register_interface("File", PwInterface_File);
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
    }
}

/****************************************************************
 * Shorthand functions
 */

PwResult _pw_file_open(PwValuePtr file_name, int flags, mode_t mode)
{
    PwValue file = pw_create(PwTypeId_File);
    pw_return_if_error(&file);

    pw_expect_ok( pw_interface(file.type_id, File)->open(&file, file_name, flags, mode) );

    return pw_move(&file);
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
