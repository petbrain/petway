#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>

#include <pw.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 * Status codes
 */

extern uint16_t PW_ERROR_BAD_ADDRESS_FAMILY;
extern uint16_t PW_ERROR_BAD_IP_ADDRESS;
extern uint16_t PW_ERROR_BAD_PORT;
extern uint16_t PW_ERROR_HOST_ADDRESS_EXPECTED;
extern uint16_t PW_ERROR_ADDRESS_FAMILY_MISMATCH;
extern uint16_t PW_ERROR_SOCKET_NAME_TOO_LONG;
extern uint16_t PW_ERROR_MISSING_NETMASK;
extern uint16_t PW_ERROR_BAD_NETMASK;
extern uint16_t PW_ERROR_PORT_UNSPECIFIED;


/****************************************************************
 * SockAddr type
 */

extern PwTypeId PwTypeId_SockAddr;

#define pw_is_sockaddr(value)      pw_is_subtype((value), PwTypeId_SockAddr)
#define pw_assert_sockaddr(value)  pw_assert(pw_is_sockaddr(value))

typedef struct {
    struct sockaddr_storage addr;
    unsigned netmask;  // netmask in CIDR notation, i.e. number of network bits;
                       // nonzero for subnet address, zero for host address
} _PwSockAddrData;

#define _pw_sockaddr_data_ptr(value)  ((_PwSockAddrData*) _pw_get_data_ptr((value), PwTypeId_SockAddr))

// helper functions

static inline _PwSockAddrData* pw_sockaddr_data(PwValuePtr value)
{
    pw_assert(pw_is_subtype(value, PwTypeId_SockAddr));
    return _pw_sockaddr_data_ptr(value);
}

static inline unsigned pw_sockaddr_netmask(PwValuePtr value)
/*
 * return `netmask` (number of bits).
 */
{
    return pw_sockaddr_data(value)->netmask;
}

static inline int pw_sockaddr_family(PwValuePtr value)
/*
 * return address family (domain in terms of socket() args)
 */
{
    return pw_sockaddr_data(value)->addr.ss_family;
}

static inline in_port_t pw_sockaddr_port(PwValuePtr value)
/*
 * return port in host byte order
 */
{
    _PwSockAddrData* sa = pw_sockaddr_data(value);
    // port is at the same location for both ipv4 and ipv6, thus using sockaddr_in
    return ntohs(((struct sockaddr_in*) &sa->addr)->sin_port);
}

static inline unsigned pw_sockaddr_ipv4(PwValuePtr value)
/*
 * return IPv4 address in host byte order
 */
{
    _PwSockAddrData* sa = pw_sockaddr_data(value);
    pw_assert(sa->addr.ss_family == AF_INET);
    return ntohl(((struct sockaddr_in*) &sa->addr)->sin_addr.s_addr);
}

static inline uint8_t* pw_sockaddr_ipv6(PwValuePtr value)
/*
 * Return pointer to IPv6 address in network byte order.
 * The result should be used within lifetime of `value`.
 */
{
    _PwSockAddrData* sa = pw_sockaddr_data(value);
    pw_assert(sa->addr.ss_family == AF_INET6);
    return ((struct sockaddr_in6*) &sa->addr)->sin6_addr.s6_addr;
}

/****************************************************************
 * Socket interface.
 */

extern unsigned PwInterfaceId_Socket;

typedef struct {

    [[nodiscard]] bool (*bind)(PwValuePtr self, PwValuePtr local_addr);
    /*
     * Set `local_addr` in _PwSocketData structure and call `bind` function.
     *
     * `local_addr` can be one of:
     *   - SockAddr with netmask equals to zero and address family matching socket's domain;
     *   - String, either parseable as IP address:port or local socket name.
     *
     * Local socket names are up to sizeof(sockaddr_un.sun_path)-1 characters.
     * If the string starts with null character, it is treated
     * as a name in Linux abstract namespace.
     */

    [[nodiscard]] bool (*reuse_addr)(PwValuePtr self, bool reuse);
    /*
     * Set or clear SO_REUSEADDR option for socket.
     */

    [[nodiscard]] bool (*set_nonblocking)(PwValuePtr self, bool mode);
    /*
     * Set/reset nonblocking mode for socket.
     */

    [[nodiscard]] bool (*listen)(PwValuePtr self, int backlog);
    /*
     * Call `listen`, set listen_backlog
     *
     * If backlog is 0, it is set to 5.
     */

    [[nodiscard]] bool (*accept)(PwValuePtr self, PwValuePtr result);
    /*
     * Accept incoming connection on listening socket
     * and return new socket with `remote_addr` initialized
     * in _PwSocketData structure.
     *
     * The type of returned socket is same as self,
     * which means that AsyncSocket is returned if self is AsyncSocket.
     */

    [[nodiscard]] bool (*connect)(PwValuePtr self, PwValuePtr remote_addr);
    /*
     * Set `remote_addr` in _PwSocketData structure and call `connect` function.
     *
     * `remote_addr` can be one of:
     *   - SockAddr with netmask equals to zero and address family matching socket's domain;
     *   - String, either parseable as IP address:port or local socket name.
     *
     * Host names must be explicitly resolved before calling this method.
     * If address family is not AF_LOCAL and string contains host name,
     * PW_ERROR_BAD_IP_ADDRESS is returned.
     *
     * Return EINPROGRESS errno for both local and inet non-blocking sockets
     * (originally errnos are different, see man page).
     */

    [[nodiscard]] bool (*shutdown)(PwValuePtr self, int how);
    /*
     * Call `shutdown` function.
     */

    // TODO get/setsockopt

    // TODO datagram functions: sendto, recvfrom

} PwInterface_Socket;


/****************************************************************
 * Socket data type.
 *
 * Socket data type supports the following interfaces:
 *  - Socket
 *  - Reader
 *  - Writer
 */

extern PwTypeId PwTypeId_Socket;

#define pw_is_socket(value)      pw_is_subtype((value), PwTypeId_Socket)
#define pw_assert_socket(value)  pw_assert(pw_is_socket(value))

// constructor args
typedef struct {
    int domain;
    int type;
    int protocol;

} PwSocketCtorArgs;

// socket data

typedef struct {
    int sock;
    int listen_backlog;  // 0: not listening

    // args socket() was called with
    int domain;
    int type;
    int proto;

    _PwValue local_addr;   // set when bind returns success
    _PwValue remote_addr;  // set when connect returns success or "in progress"

} _PwSocketData;

#define _pw_socket_data_ptr(value)  ((_PwSocketData*) _pw_get_data_ptr((value), PwTypeId_Socket))


/****************************************************************
 * Shorthand functions
 */

[[nodiscard]] static inline bool pw_socket(PwTypeId type_id, int domain, int sock_type, int protocol, PwValuePtr result)
{
    PwSocketCtorArgs args = {
        .domain   = domain,
        .type     = sock_type,
        .protocol = protocol
    };
    return pw_create2(type_id, &args, result);
}

[[nodiscard]] static inline bool pw_socket_bind(PwValuePtr sock, PwValuePtr local_addr)
{
    return pw_interface(sock->type_id, Socket)->bind(sock, local_addr);
}

[[nodiscard]] static inline bool pw_socket_reuse_addr(PwValuePtr sock, bool reuse)
{
    return pw_interface(sock->type_id, Socket)->reuse_addr(sock, reuse);
}

[[nodiscard]] static inline bool pw_socket_set_nonblocking(PwValuePtr sock, bool mode)
{
    return pw_interface(sock->type_id, Socket)->set_nonblocking(sock, mode);
}

[[nodiscard]] static inline bool pw_socket_listen(PwValuePtr sock, int backlog)
{
    return pw_interface(sock->type_id, Socket)->listen(sock, backlog);
}

[[nodiscard]] static inline bool pw_socket_accept(PwValuePtr sock, PwValuePtr result)
{
    return pw_interface(sock->type_id, Socket)->accept(sock, result);
}

[[nodiscard]] static inline bool pw_socket_connect(PwValuePtr sock, PwValuePtr remote_addr)
{
    return pw_interface(sock->type_id, Socket)->connect(sock, remote_addr);
}

[[nodiscard]] static inline bool pw_socket_shutdown(PwValuePtr sock, int how)
{
    return pw_interface(sock->type_id, Socket)->shutdown(sock, how);
}

#ifdef __cplusplus
}
#endif
