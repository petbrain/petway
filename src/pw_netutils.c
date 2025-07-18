#include <arpa/inet.h>

#include "include/pw_netutils.h"
#include "include/pw_parse.h"
#include "include/pw_socket.h"
#include "include/pw_utf.h"
#include "src/pw_struct_internal.h"


[[nodiscard]] static bool parse_addr(int domain, PwValuePtr addr, in_port_t port, PwValuePtr result)
{
    if (!pw_create(PwTypeId_SockAddr, result)) {
        return false;
    }

    _PwSockAddrData* sa = _pw_sockaddr_data_ptr(result);

    PwValue a = PW_NULL;

    if (pw_startswith(addr, '[') && pw_endswith(addr, ']')) {
        if (!pw_substr(addr, 1, pw_strlen(addr) - 1, &a)) {
            return false;
        }
    } else {
        __pw_clone(addr, &a);
    }

    if (domain == AF_UNSPEC && pw_strchr(&a, ':', 0, nullptr)) {
        domain = AF_INET6;
    } else {
        domain = AF_INET;
    }

    PW_CSTRING_LOCAL(c_addr, &a);
    int rc;
    switch (domain) {
        case AF_INET: {
            struct sockaddr_in* sin = (struct sockaddr_in*) &sa->addr;
            sin->sin_family = domain;
            sin->sin_port = htons(port);
            rc = inet_pton(AF_INET, c_addr, &sin->sin_addr);
            break;
        }
        case AF_INET6: {
            struct sockaddr_in6* sin = (struct sockaddr_in6*) &sa->addr;
            sin->sin6_family = domain;
            sin->sin6_port = htons(port);
            rc = inet_pton(AF_INET, c_addr, &sin->sin6_addr);
            break;
        }
        default:
            pw_set_status(PwStatus(PW_ERROR_BAD_ADDRESS_FAMILY));
            return false;
    }
    if (rc == -1) {
        pw_set_status(PwStatus(PW_ERROR_BAD_ADDRESS_FAMILY));
        return false;
    }
    if (rc != 1) {
        pw_set_status(PwStatus(PW_ERROR_BAD_IP_ADDRESS));
        return false;
    }
    return true;
}

[[nodiscard]] bool _pw_parse_inet_address(PwValuePtr addr, PwValuePtr result)
{
    pw_expect(String, addr);

    // try to separate address and port parts
    PwValue parts = PW_NULL;
    if (!pw_string_rsplit_chr(addr, ':', 1, &parts)) {
        return false;
    }
    if (pw_array_length(&parts) == 1) {
        // addr without port
        return parse_addr(AF_UNSPEC, addr, 0, result);
    }

    PwValue addr_part = PW_NULL;
    if (!pw_array_item(&parts, 0, &addr_part)) {
        return false;
    }
    PwValue port_part = PW_NULL;
    if (!pw_array_item(&parts, 1, &port_part)) {
        return false;
    }
    if (pw_strchr(&addr_part, ':', 0, nullptr)) {
        // IPv6 address?
        if (pw_startswith(&addr_part, '[') && pw_endswith(&addr_part, ']')) {
            // yes, with port
        } else {
            // yes, without port
            // parse original addr as IPv6
            return parse_addr(AF_INET6, addr, 0, result);
        }
    }

    // parse port
    PwValue port = PW_NULL;
    if (!pw_parse_number(&port_part, &port)) {
        return false;
    }
    if ( ! (pw_is_signed(&port) && port.signed_value > 0 && port.signed_value < 65536)) {
        pw_set_status(PwStatus(PW_ERROR_BAD_PORT));
        return false;
    }

    // parse address
    return parse_addr(AF_UNSPEC, &addr_part, port.signed_value, result);
}

[[nodiscard]] bool pw_parse_subnet(PwValuePtr subnet, PwValuePtr netmask, PwValuePtr result)
{
    pw_expect(String, subnet);

    // check CIDR notation
    PwValue parts = PW_NULL;
    if (!pw_string_split_chr(subnet, '/', 0, &parts)) {
        return false;
    }

    unsigned num_parts = pw_array_length(&parts);
    if (num_parts > 2) {
        pw_set_status(PwStatus(PW_ERROR_BAD_NETMASK));
        return false;
    }

    // parse subnet address
    PwValue subnet_addr = PW_NULL;
    if (!pw_array_item(&parts, 0, &subnet_addr)) {
        return false;
    }
    if (!pw_parse_inet_address(&subnet_addr, result)) {
        return false;
    }

    if (num_parts > 1) {

        // try CIDR netmask
        PwValue cidr_netmask = PW_NULL;
        if (!pw_array_item(&parts, 1, &cidr_netmask)) {
            return false;
        }
        PwValue n = PW_NULL;
        if (!pw_parse_number(&cidr_netmask, &n)) {
            return false;
        }
        if (!pw_is_signed(&n)) {
            pw_set_status(PwStatus(PW_ERROR_BAD_NETMASK));
            return false;
        }
        if (n.signed_value == 0) {
            pw_set_status(PwStatus(PW_ERROR_BAD_NETMASK));
            return false;
        }

        // check upper range depending on address family
        _PwSockAddrData* sa = _pw_sockaddr_data_ptr(result);
        int max_bits;
        if (sa->addr.ss_family == AF_INET) {
            max_bits = 32;
        } else {
            max_bits = 128;
        }
        if (n.signed_value >= max_bits) {
            pw_set_status(PwStatus(PW_ERROR_BAD_NETMASK));
            return false;
        }

        // set netmask in the result
        sa->netmask = n.signed_value;

    } else {
        // not CIDR notation, parse netmask parameter

        if (pw_is_null(netmask)) {
            pw_set_status(PwStatus(PW_ERROR_MISSING_NETMASK));
            return false;
        }

        pw_expect(String, netmask);

        PwValue parsed_netmask = PW_NULL;
        if (!pw_parse_inet_address(netmask, &parsed_netmask)) {
            return false;
        }

        // check address families are same
        _PwSockAddrData* sa_result = _pw_sockaddr_data_ptr(result);
        _PwSockAddrData* sa_netmask = _pw_sockaddr_data_ptr(&parsed_netmask);

        if (sa_result->addr.ss_family != sa_netmask->addr.ss_family) {
            pw_set_status(PwStatus(PW_ERROR_BAD_NETMASK));
            return false;
        }

        // convert netmask to the number of bits
        if (sa_result->addr.ss_family == AF_INET) {
            unsigned n = ntohl(((struct sockaddr_in*) &sa_netmask->addr)->sin_addr.s_addr);
            static_assert(sizeof(n) >= 4);
            if (n == 0) {
                sa_result->netmask = 0;
            } else {
                // XXX use stdc_trailing_zeros
                sa_result->netmask = 32 - __builtin_ctz(n);
            }
        } else {
            uint8_t* addr = ((struct sockaddr_in6*) &sa_netmask->addr)->sin6_addr.s6_addr;
            sa_result->netmask = 128;
            addr += 15;
            for (unsigned i = 0; i < 16; i++) {
                uint8_t n = *addr--;
                if (n) {
                    // XXX use stdc_trailing_zeros
                    sa_result->netmask -= __builtin_ctz(n);
                    break;
                }
                sa_result->netmask -= 8;
            }
        }
    }
    return true;
}

[[nodiscard]] bool _pw_split_addr_port(PwValuePtr addr_port, PwValuePtr addr, PwValuePtr port)
{
    PwValue emptystr = PW_STRING("");
    PwValue parts = PW_NULL;
    if (!pw_string_rsplit_chr(addr_port, ':', 1, &parts)) {
        return false;
    }
    if (pw_array_length(&parts) == 1) {
        // Assume addr_port contains port (or service name) only.
        pw_move(&emptystr, addr);
        if (!pw_array_item(&parts, 0, port)) {
            return false;
        }
    } else {
        if (!pw_array_item(&parts, 0, addr)) {
            return false;
        }
        if (!pw_array_item(&parts, 1, port)) {
            return false;
        }
        if (pw_strchr(addr, ':', 0, nullptr)) {
            // IPv6 address?
            if (pw_startswith(addr, '[') && pw_endswith(addr, ']')) {
                // address is in square brackets, so port is okay
            } else {
                // port is missing
                pw_clone2(addr_port, addr);
                pw_clone2(&emptystr, port);
            }
        }
    }
    return true;
}
