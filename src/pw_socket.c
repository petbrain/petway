#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>

#include "include/pw_args.h"
#include "include/pw_netutils.h"
#include "include/pw_socket.h"
#include "include/pw_utf.h"
#include "src/pw_struct_internal.h"

// status codes
uint16_t PW_ERROR_BAD_ADDRESS_FAMILY = 0;
uint16_t PW_ERROR_BAD_IP_ADDRESS = 0;
uint16_t PW_ERROR_BAD_PORT = 0;
uint16_t PW_ERROR_HOST_ADDRESS_EXPECTED = 0;
uint16_t PW_ERROR_ADDRESS_FAMILY_MISMATCH = 0;
uint16_t PW_ERROR_SOCKET_NAME_TOO_LONG = 0;
uint16_t PW_ERROR_MISSING_NETMASK = 0;
uint16_t PW_ERROR_BAD_NETMASK = 0;
uint16_t PW_ERROR_PORT_UNSPECIFIED = 0;

/****************************************************************
 * SockAddr type
 */

PwTypeId PwTypeId_SockAddr = 0;

static PwType sockaddr_type;

static void sockaddr_hash(PwValuePtr self, PwHashContext* ctx)
{
    _PwSockAddrData* sa = _pw_sockaddr_data_ptr(self);

    _pw_hash_uint64(ctx, self->type_id);
    _pw_hash_uint64(ctx, sa->addr.ss_family);
    switch (sa->addr.ss_family) {
        case AF_LOCAL: {
            char* path = ((struct sockaddr_un*) &sa->addr)->sun_path;
            for (;;) {
                unsigned char chr = *path++;
                if (chr == 0) {
                    break;
                }
                _pw_hash_uint64(ctx, chr);
            }
            break;
        }
        case AF_INET:
            _pw_hash_uint64(ctx, ((struct sockaddr_in*) &sa->addr)->sin_addr.s_addr);
            _pw_hash_uint64(ctx, ((struct sockaddr_in*) &sa->addr)->sin_port);
            break;
        case AF_INET6: {
            uint64_t* addr = (uint64_t*) &((struct sockaddr_in6*) &sa->addr)->sin6_addr;
            _pw_hash_uint64(ctx, *addr++);
            _pw_hash_uint64(ctx, *addr);
            _pw_hash_uint64(ctx, ((struct sockaddr_in6*) &sa->addr)->sin6_port);
            break;
        }
        default:
            break;
    }
    _pw_hash_uint64(ctx, sa->netmask);
}

static void dump_sockaddr(FILE* fp, PwValuePtr sockaddr)
{
    _PwSockAddrData* sa = _pw_sockaddr_data_ptr(sockaddr);

    const char* addr;
    char addr_buf[1024];
    in_port_t port = 0;
    switch (sa->addr.ss_family) {
        case AF_UNSPEC:
            addr = "[unspecified]";
            break;
        case AF_LOCAL:
            addr = ((struct sockaddr_un*) &sa->addr)->sun_path;
            break;
        case AF_INET:
            port = ntohs(((struct sockaddr_in*) &sa->addr)->sin_port);
            addr = inet_ntop(AF_INET, &((struct sockaddr_in*) &sa->addr)->sin_addr, addr_buf, sizeof(addr_buf));
            break;
        case AF_INET6:
            port = ntohs(((struct sockaddr_in6*) &sa->addr)->sin6_port);
            addr = inet_ntop(AF_INET6, &((struct sockaddr_in6*) &sa->addr)->sin6_addr, addr_buf, sizeof(addr_buf));
            break;
        default:
            addr = "<not supported>";
            break;
    }
    if (!addr) {
        addr = "<error>";
    }
    if (port) {
        if (sa->addr.ss_family == AF_INET6) {
            fprintf(fp, " [%s]:%u", addr, port);
        } else {
            fprintf(fp, " %s:%u", addr, port);
        }
    } else {
        fputc(' ', fp);
        fputs(addr, fp);
    }
    if (sa->netmask) {
        fprintf(fp, "/%u\n", sa->netmask);
    } else {
        fputc('\n', fp);
    }
}

static void sockaddr_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _pw_dump_start(fp, self, first_indent);
    _pw_dump_struct_data(fp, self);

    dump_sockaddr(fp, self);
}


/****************************************************************
 * Socket type
 */

PwTypeId PwTypeId_Socket = 0;

static PwType socket_type;

[[nodiscard]] static bool socket_init(PwValuePtr self, void* ctor_args)
{
    PwSocketCtorArgs* args = ctor_args;
    _PwSocketData* sd = _pw_socket_data_ptr(self);

    if (args) {
        sd->sock = socket(args->domain, args->type, args->protocol);
        if (sd->sock == -1) {
            pw_set_status(PwErrno(errno));
            return false;
        }

        sd->domain = args->domain;
        sd->type   = args->type;
        sd->proto  = args->protocol;

    } else {
        // special case for internal use in accept method
        sd->sock = -1;
    }

    return true;
}

static void socket_fini(PwValuePtr self)
{
    _PwSocketData* sd = _pw_socket_data_ptr(self);

    if (sd->sock != -1) {
        close(sd->sock);
        sd->sock = -1;
    }

    pw_destroy(&sd->local_addr);
    pw_destroy(&sd->remote_addr);
}

static void socket_dump(PwValuePtr self, FILE* fp, int first_indent, int next_indent, _PwCompoundChain* tail)
{
    _PwSocketData* sd = _pw_socket_data_ptr(self);

    _pw_dump_start(fp, self, first_indent);
    _pw_dump_struct_data(fp, self);

    // domain to string

    char* domain;
    char domain_other[32];
    switch (sd->domain) {
        case AF_UNSPEC:  domain = "AF_UNSPEC"; break;
        case AF_LOCAL:   domain = "AF_LOCAL"; break;
        case AF_INET:    domain = "AF_INET"; break;
        case AF_INET6:   domain = "AF_INET6"; break;
        case AF_NETLINK: domain = "AF_NETLINK"; break;
        case AF_PACKET:  domain = "AF_PACKET"; break;
        default:
            snprintf(domain_other, sizeof(domain_other), "AF_(%d)", sd->domain);
            domain = domain_other;
            break;
    }

    // socket type to string

    char* type;
    char type_other[32];
    char type_buf[80];
    int t = sd->type & ~(SOCK_CLOEXEC | SOCK_NONBLOCK);
    switch (t) {
        case SOCK_STREAM: type = "SOCK_STREAM"; break;
        case SOCK_DGRAM:  type = "SOCK_DGRAM"; break;
        case SOCK_RAW:    type = "SOCK_RAW"; break;
        default:
            snprintf(type_other, sizeof(type_other), "SOCK_(%d)", t);
            type = type_other;
            break;
    }
    if (sd->type & (SOCK_CLOEXEC | SOCK_NONBLOCK)) {
        strcpy(type_buf, type);
        strcat(type_buf, "{");
        if (sd->type & SOCK_CLOEXEC) {
            strcat(type_buf, "SOCK_CLOEXEC");
        }
        if (sd->type & SOCK_NONBLOCK) {
            if (sd->type & SOCK_CLOEXEC) {
                strcat(type_buf, ",");
            }
            strcat(type_buf, "SOCK_NONBLOCK");
        }
        strcat(type_buf, "}");
        type = type_buf;
    }

    // proto to string

    char* protocol;
    struct protoent proto;
    struct protoent *proto_result;
    char proto_buf[1024];
    if (getprotobynumber_r(sd->proto, &proto, proto_buf, sizeof(proto_buf), &proto_result) == 0) {
        protocol = proto_result->p_name;
    } else {
        snprintf(proto_buf, sizeof(proto_buf), "%d", sd->proto);
        protocol = proto_buf;
    }

    // listen backlog to string

    char* listening = "";
    char listening_buf[128];
    if (sd->listen_backlog) {
        snprintf(listening_buf, sizeof(listening_buf), ", listening (backlog=%d, new_type=%u)", sd->listen_backlog, sd->new_sock_type);
        listening = listening_buf;
    }

    // dump

    fprintf(fp, " socket(%s, %s, %s) fd=%d%s\n", domain, type, protocol, sd->sock, listening);

    if (pw_is_sockaddr(&sd->local_addr)) {
        fputs("Local address:", fp);
        _pw_print_indent(fp, next_indent);
        dump_sockaddr(fp, &sd->local_addr);
    }
    if (pw_is_sockaddr(&sd->remote_addr)) {
        fputs("Remote address:", fp);
        _pw_print_indent(fp, next_indent);
        dump_sockaddr(fp, &sd->remote_addr);
    }
}


/****************************************************************
 * Socket interface
 */

unsigned PwInterfaceId_Socket = 0;

[[nodiscard]] static bool make_address(int domain, PwValuePtr addr, socklen_t* ss_addr_size, PwValuePtr result)
/*
 * Helper function for bind and connect.
 *
 * If `addr` is SockAddr, clone it if address family matches domain.
 * If `addr` is String, try to convert it to SockAddr.
 *
 * Write size of sockaddr atructure to `ss_addr_size`.
 */
{
    *ss_addr_size = sizeof(struct sockaddr_storage);

    if (pw_is_sockaddr(addr)) {
        _PwSockAddrData* sa = _pw_sockaddr_data_ptr(addr);
        if (sa->netmask != 0) {
            pw_set_status(PwStatus(PW_ERROR_HOST_ADDRESS_EXPECTED));
            return false;
        }
        if (sa->addr.ss_family != domain) {
            pw_set_status(PwStatus(PW_ERROR_ADDRESS_FAMILY_MISMATCH));
            return false;
        }
        pw_clone2(addr, result);
        return true;
    }

    pw_expect(String, addr);

    switch (domain) {
        case AF_LOCAL: {
            if (!pw_create(PwTypeId_SockAddr, result)) {
                return false;
            }

            _PwSockAddrData* sa = _pw_sockaddr_data_ptr(result);
            sa->addr.ss_family = AF_LOCAL;

            unsigned len = pw_strlen_in_utf8(addr);
            if (len >= sizeof(((struct sockaddr_un*)0)->sun_path)) {
                pw_set_status(PwStatus(PW_ERROR_SOCKET_NAME_TOO_LONG));
                return false;
            }
            pw_string_to_utf8_buf(addr, ((struct sockaddr_un*) &sa->addr)->sun_path);
            *ss_addr_size = offsetof(struct sockaddr_un, sun_path) + len + 1;
            return true;
        }
        case AF_INET:
        case AF_INET6:
            return pw_parse_inet_address(addr, result);

        default:
            pw_panic("Address family %d is not supported yet\n", domain);
    }
}

[[nodiscard]] static bool socket_bind(PwValuePtr self, PwValuePtr local_addr)
{
    _PwSocketData* sd = _pw_socket_data_ptr(self);

    pw_destroy(&sd->local_addr);

    socklen_t addr_size;
    PwValue addr = PW_NULL;
    if (!make_address(sd->domain, local_addr, &addr_size, &addr)) {
        return false;
    }

    pw_move(&addr, &sd->local_addr);
    _PwSockAddrData* sa = _pw_sockaddr_data_ptr(&sd->local_addr);

    if (bind(sd->sock, (struct sockaddr*) &sa->addr, addr_size) == 0) {
        return true;
    } else {
        pw_set_status(PwErrno(errno));
        return false;
    }
}

[[nodiscard]] static bool socket_reuse_addr(PwValuePtr self, bool reuse)
{
    _PwSocketData* sd = _pw_socket_data_ptr(self);

    int i = reuse;
    if (setsockopt(sd->sock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0) {
        pw_set_status(PwErrno(errno));
        return false;
    } else {
        return true;
    }
}

[[nodiscard]] static bool socket_set_nonblocking(PwValuePtr self, bool mode)
{
    _PwSocketData* sd = _pw_socket_data_ptr(self);

    int flags = fcntl(sd->sock, F_GETFL, 0);
    if (mode) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    if (fcntl(sd->sock, F_SETFL, flags) == -1) {
        pw_set_status(PwErrno(errno));
        return false;
    } else {
        return true;
    }
}

[[nodiscard]] static bool socket_listen(PwValuePtr self, int backlog, PwTypeId new_sock_type)
{
    _PwSocketData* sd = _pw_socket_data_ptr(self);

    if (backlog == 0) {
        backlog = 5;
    }
    if (new_sock_type == PwTypeId_Null) {
        new_sock_type = self->type_id;
    }
    if (listen(sd->sock, backlog) == -1) {
        pw_set_status(PwErrno(errno));
        return false;
    }
    sd->listen_backlog = backlog;
    sd->new_sock_type = new_sock_type;
    return true;
}

[[nodiscard]] static bool socket_accept(PwValuePtr self, PwValuePtr result)
{
    _PwSocketData* sd_lsnr = _pw_socket_data_ptr(self);

    // create uninitialized socket
    if (!pw_create(sd_lsnr->new_sock_type, result)) {
        return false;
    }

    _PwSocketData* sd_new  = _pw_socket_data_ptr(result);

    // initialize addresses for new socket
    pw_clone2(&sd_lsnr->local_addr, &sd_new->local_addr);
    if (!pw_create(PwTypeId_SockAddr, &sd_new->remote_addr)) {
        return false;
    }

    // get pointer to remote address structure
    _PwSockAddrData* sa_remote = _pw_sockaddr_data_ptr(&sd_new->remote_addr);

    // call accept and initialize new socket and remote address
    socklen_t addrlen = sizeof(sa_remote->addr);
    sd_new->sock = accept(sd_lsnr->sock, (struct sockaddr*) &sa_remote->addr,  &addrlen);
    if (sd_new->sock < 0) {
        pw_set_status(PwErrno(errno));
        return false;
    }
    // initialize other fields (they are mainly for informational purposes)
    sd_new->domain = sd_lsnr->domain;
    sd_new->type   = sd_lsnr->type;
    sd_new->proto  = sd_lsnr->proto;

    return true;
}

[[nodiscard]] static bool socket_connect(PwValuePtr self, PwValuePtr remote_addr)
{
    _PwSocketData* sd = _pw_socket_data_ptr(self);

    pw_destroy(&sd->local_addr);

    socklen_t addr_size;
    PwValue addr = PW_NULL;
    if (!make_address(sd->domain, remote_addr, &addr_size, &addr)) {
        return false;
    }

    pw_move(&addr, &sd->remote_addr);
    _PwSockAddrData* sa = _pw_sockaddr_data_ptr(&sd->remote_addr);

    int rc;
    do {
        rc = connect(sd->sock, (struct sockaddr*) &sa->addr, sizeof(sa->addr));
    } while (rc != 0 && errno == EINTR);

    if (rc == 0) {
        return true;
    }
    if (errno == EAGAIN && sa->addr.ss_family == AF_LOCAL) {
        // make it consistent
        errno = EINPROGRESS;
    }
    pw_set_status(PwErrno(errno));
    return false;
}

[[nodiscard]] static bool socket_shutdown(PwValuePtr self, int how)
{
    _PwSocketData* sd = _pw_socket_data_ptr(self);

    if (shutdown(sd->sock, how) == 0) {
        return true;
    } else {
        pw_set_status(PwErrno(errno));
        return false;
    }
}

static PwInterface_Socket socket_interface = {
    .bind       = socket_bind,
    .reuse_addr = socket_reuse_addr,
    .set_nonblocking = socket_set_nonblocking,
    .listen     = socket_listen,
    .accept     = socket_accept,
    .connect    = socket_connect,
    .shutdown   = socket_shutdown
};

/****************************************************************
 * Reader and writer interfaces
 */

[[nodiscard]] static bool socket_read(PwValuePtr self, void* buffer, unsigned buffer_size, unsigned* bytes_read)
{
    _PwSocketData* sd = _pw_socket_data_ptr(self);

    ssize_t result;
    do {
        result = read(sd->sock, buffer, buffer_size);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        pw_set_status(PwErrno(errno));
        return false;
    } else {
        *bytes_read = (unsigned) result;
        return true;
    }
}

[[nodiscard]] static bool socket_write(PwValuePtr self, void* data, unsigned size, unsigned* bytes_written)
{
    _PwSocketData* sd = _pw_socket_data_ptr(self);

    ssize_t result;
    do {
        result = write(sd->sock, data, size);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        pw_set_status(PwErrno(errno));
        return false;
    } else {
        *bytes_written = (unsigned) result;
        return true;
    }
}

static PwInterface_Reader socket_reader_interface = {
    .read = socket_read
};

static PwInterface_Writer socket_writer_interface = {
    .write = socket_write
};

/****************************************************************
 * Initialization
 */

[[ gnu::constructor ]]
void _pw_init_socket()
{
    // interface can be already registered
    // basically. interfaces can be registered by any type in any order
    if (PwInterfaceId_Socket == 0) {
        PwInterfaceId_Socket = pw_register_interface("Socket", PwInterface_Socket);
    }

    if (PwTypeId_Socket != 0) {
        return;
    }

    // init statuses
    PW_ERROR_BAD_ADDRESS_FAMILY = pw_define_status("BAD_ADDRESS_FAMILY");
    PW_ERROR_BAD_IP_ADDRESS     = pw_define_status("BAD_IP_ADDRESS");
    PW_ERROR_BAD_PORT           = pw_define_status("BAD_PORT");
    PW_ERROR_HOST_ADDRESS_EXPECTED   = pw_define_status("HOST_ADDRESS_EXPECTED");
    PW_ERROR_ADDRESS_FAMILY_MISMATCH = pw_define_status("ADDRESS_FAMILY_MISMATCH");
    PW_ERROR_SOCKET_NAME_TOO_LONG    = pw_define_status("SOCKET_NAME_TOO_LONG");
    PW_ERROR_MISSING_NETMASK    = pw_define_status("MISSING_NETMASK");
    PW_ERROR_BAD_NETMASK        = pw_define_status("BAD_NETMASK");
    PW_ERROR_PORT_UNSPECIFIED   = pw_define_status("PORT_UNSPECIFIED");

    // init types

    PwTypeId_SockAddr = pw_struct_subtype(
        &sockaddr_type, "SockAddr", PwTypeId_Struct, _PwSockAddrData
    );
    sockaddr_type.hash = sockaddr_hash;
    sockaddr_type.dump = sockaddr_dump;

    PwTypeId_Socket = pw_struct_subtype(
        &socket_type, "Socket", PwTypeId_Struct, _PwSocketData,
        PwInterfaceId_Socket, &socket_interface,
        PwInterfaceId_Reader, &socket_reader_interface,
        PwInterfaceId_Writer, &socket_writer_interface
    );
    socket_type.dump = socket_dump;
    socket_type.init = socket_init;
    socket_type.fini = socket_fini;
}
