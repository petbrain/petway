#pragma once

#include <pw.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parse IPv4 or IPv6 address, optionally followed by port
 * and return PwType_SockAddr.
 */
#define pw_parse_inet_address(addr, result) _Generic((addr),  \
             char*: _pw_parse_inet_address_ascii,  \
          char8_t*: _pw_parse_inet_address_utf8,   \
         char32_t*: _pw_parse_inet_address_utf32,  \
        PwValuePtr: _pw_parse_inet_address         \
    )((addr), (result))

[[nodiscard]] bool _pw_parse_inet_address(PwValuePtr addr, PwValuePtr result);

[[nodiscard]] static inline bool _pw_parse_inet_address_ascii(char*     addr, PwValuePtr result) { _PwValue a = PwStaticString(addr); return _pw_parse_inet_address(&a, result); }
[[nodiscard]] static inline bool _pw_parse_inet_address_utf8 (char8_t*  addr, PwValuePtr result) {  PwValue a = PwStringUtf8  (addr); return _pw_parse_inet_address(&a, result); }
[[nodiscard]] static inline bool _pw_parse_inet_address_utf32(char32_t* addr, PwValuePtr result) { _PwValue a = PwStaticString(addr); return _pw_parse_inet_address(&a, result); }


[[nodiscard]] bool pw_parse_subnet(PwValuePtr subnet, PwValuePtr netmask, PwValuePtr result);
/*
 * Parse IPv4 or IPv6 subnet.
 * If subnet is in CIDR notation, netmask argument is not used.
 * Return PwType_SockAddr.
 */


/*
 * Split address and port separated by colon.
 * Both parts can be empty string.
 * Port can be a service name, understood by getaddrinfo.
 */
#define pw_split_addr_port(addr_port, addr, port) _Generic((addr_port),  \
             char*: _pw_split_addr_port_ascii,  \
          char8_t*: _pw_split_addr_port_utf8,   \
         char32_t*: _pw_split_addr_port_utf32,  \
        PwValuePtr: _pw_split_addr_port         \
    )(addr_port, (addr), (port))

[[nodiscard]] bool _pw_split_addr_port(PwValuePtr addr_port, PwValuePtr addr, PwValuePtr port);

[[nodiscard]] static inline bool _pw_split_addr_port_ascii(char*     addr_port, PwValuePtr addr, PwValuePtr port) { _PwValue ap = PwStaticString(addr_port); return _pw_split_addr_port(&ap, addr, port); }
[[nodiscard]] static inline bool _pw_split_addr_port_utf8 (char8_t*  addr_port, PwValuePtr addr, PwValuePtr port) {  PwValue ap = PwStringUtf8  (addr_port); return _pw_split_addr_port(&ap, addr, port); }
[[nodiscard]] static inline bool _pw_split_addr_port_utf32(char32_t* addr_port, PwValuePtr addr, PwValuePtr port) { _PwValue ap = PwStaticString(addr_port); return _pw_split_addr_port(&ap, addr, port); }


#ifdef __cplusplus
}
#endif
