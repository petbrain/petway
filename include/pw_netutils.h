#pragma once

#include <pw.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parse IPv4 or IPv6 address, optionally followed by port.
 *
 * On success return PwType_SockAddr.
 * On error return status value.
 */
#define pw_parse_inet_address(addr) _Generic((addr),    \
             char*: _pw_parse_inet_address_u8_wrapper,  \
          char8_t*: _pw_parse_inet_address_u8,          \
         char32_t*: _pw_parse_inet_address_u32,         \
        PwValuePtr: _pw_parse_inet_address              \
    )(addr)

PwResult _pw_parse_inet_address(PwValuePtr addr);

static inline PwResult _pw_parse_inet_address_u8  (char8_t*  addr) { __PWDECL_CharPtr  (a, addr); return _pw_parse_inet_address(&a); }
static inline PwResult _pw_parse_inet_address_u32 (char32_t* addr) { __PWDECL_Char32Ptr(a, addr); return _pw_parse_inet_address(&a); }

static inline PwResult _pw_parse_inet_address_u8_wrapper(char* addr)
{
    return _pw_parse_inet_address_u8((char8_t*) addr);
}


PwResult pw_parse_subnet(PwValuePtr subnet, PwValuePtr netmask);
/*
 * Parse IPv4 or IPv6 subnet.
 * If subnet is in CIDR notation, netmask argument is not used.
 *
 * On success return PwType_SockAddr.
 * On error return status value.
 */


/*
 * Split address and port separated by colon.
 * Return array containing exactly two items.
 * Both parts can be empty string.
 * Port can be a service name, understood by getaddrinfo.
 */
#define pw_split_addr_port(addr_port) _Generic((addr_port),    \
             char*: _pw_split_addr_port_u8_wrapper,  \
          char8_t*: _pw_split_addr_port_u8,          \
         char32_t*: _pw_split_addr_port_u32,         \
        PwValuePtr: _pw_split_addr_port              \
    )(addr_port)

PwResult _pw_split_addr_port(PwValuePtr addr_port);

static inline PwResult _pw_split_addr_port_u8  (char8_t*  addr_port) { __PWDECL_CharPtr  (ap, addr_port); return _pw_split_addr_port(&ap); }
static inline PwResult _pw_split_addr_port_u32 (char32_t* addr_port) { __PWDECL_Char32Ptr(ap, addr_port); return _pw_split_addr_port(&ap); }

static inline PwResult _pw_split_addr_port_u8_wrapper(char* addr_port)
{
    return _pw_split_addr_port_u8((char8_t*) addr_port);
}


#ifdef __cplusplus
}
#endif
