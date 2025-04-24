#include <arpa/inet.h>

#include "include/pw_netutils.h"
#include "include/pw_parse.h"
#include "include/pw_socket.h"
#include "src/pw_struct_internal.h"


static PwResult parse_addr(int domain, PwValuePtr addr, in_port_t port)
{
    PwValue result = pw_create(PwTypeId_SockAddr);
    pw_return_if_error(&result);

    _PwSockAddrData* sa = _pw_sockaddr_data_ptr(&result);

    PwValue a = PwNull();

    if (pw_startswith(addr, '[') && pw_endswith(addr, ']')) {
        a = pw_substr(addr, 1, pw_strlen(addr) - 1);
    } else {
        a = pw_clone(addr);
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
            return PwError(PW_ERROR_BAD_ADDRESS_FAMILY);
    }
    if (rc == -1) {
        return PwError(PW_ERROR_BAD_ADDRESS_FAMILY);
    }
    if (rc != 1) {
        return PwError(PW_ERROR_BAD_IP_ADDRESS);
    }
    return pw_move(&result);
}

PwResult _pw_parse_inet_address(PwValuePtr addr)
{
    // convert CharPtr to String
    PwValue addr_str = pw_clone(addr);

    if (!pw_is_string(&addr_str)) {
        return PwError(PW_ERROR_INCOMPATIBLE_TYPE);
    }

    // try to separate address and port parts
    PwValue parts = pw_string_rsplit_chr(&addr_str, ':', 1);
    pw_return_if_error(&parts);

    if (pw_array_length(&parts) == 1) {
        // addr without port
        return parse_addr(AF_UNSPEC, &addr_str, 0);
    }

    PwValue addr_part = pw_array_item(&parts, 0);
    PwValue port_part = pw_array_item(&parts, 1);

    if (pw_strchr(&addr_part, ':', 0, nullptr)) {
        // IPv6 address?
        if (pw_startswith(&addr_part, '[') && pw_endswith(&addr_part, ']')) {
            // yes, with port
        } else {
            // yes, without port
            // parse original addr as IPv6
            return parse_addr(AF_INET6, &addr_str, 0);
        }
    }

    // parse port
    PwValue port = pw_parse_number(&port_part);
    if ( ! (pw_is_signed(&port) && port.signed_value > 0 && port.signed_value < 65536)) {
        return PwError(PW_ERROR_BAD_PORT);
    }

    // parse address
    return parse_addr(AF_UNSPEC, &addr_part, port.signed_value);
}

PwResult pw_parse_subnet(PwValuePtr subnet, PwValuePtr netmask)
{
    // convert CharPtr to String
    PwValue subnet_str = pw_clone(subnet);
    pw_expect(String, subnet_str);

    // check CIDR notation
    PwValue parts = pw_string_split_chr(&subnet_str, '/', 0);
    pw_return_if_error(&parts);

    unsigned num_parts = pw_array_length(&parts);
    if (num_parts > 2) {
        return PwError(PW_ERROR_BAD_NETMASK);
    }

    // parse subnet address
    PwValue addr = pw_array_item(&parts, 0);
    PwValue result = pw_parse_inet_address(&addr);
    pw_return_if_error(&result);

    if (num_parts > 1) {

        // try CIDR netmask
        PwValue cidr_netmask = pw_array_item(&parts, 1);
        PwValue n = pw_parse_number(&cidr_netmask);
        if (!pw_is_signed(&n)) {
            return PwError(PW_ERROR_BAD_NETMASK);
        }
        if (n.signed_value == 0) {
            return PwError(PW_ERROR_BAD_NETMASK);
        }

        // check upper range depending on address family
        _PwSockAddrData* sa = _pw_sockaddr_data_ptr(&result);
        int max_bits;
        if (sa->addr.ss_family == AF_INET) {
            max_bits = 32;
        } else {
            max_bits = 128;
        }
        if (n.signed_value >= max_bits) {
            return PwError(PW_ERROR_BAD_NETMASK);
        }

        // set netmask in the result
        sa->netmask = n.signed_value;

    } else {
        // not CIDR notation, parse netmask parameter

        if (pw_is_null(netmask)) {
            return PwError(PW_ERROR_MISSING_NETMASK);
        }

        // convert CharPtr to String
        PwValue netmask_str = pw_clone(netmask);
        pw_expect(String, netmask_str);

        PwValue parsed_netmask = pw_parse_inet_address(&netmask_str);
        pw_return_if_error(&parsed_netmask);

        // check address families are same
        _PwSockAddrData* sa_result = _pw_sockaddr_data_ptr(&result);
        _PwSockAddrData* sa_netmask = _pw_sockaddr_data_ptr(&parsed_netmask);

        if (sa_result->addr.ss_family != sa_netmask->addr.ss_family) {
            return PwError(PW_ERROR_BAD_NETMASK);
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
    return pw_move(&result);
}

PwResult _pw_split_addr_port(PwValuePtr addr_port)
{
    PwValue emptystr = PwString();
    PwValue parts = pw_string_rsplit_chr(addr_port, ':', 1);
    pw_return_if_error(&parts);

    if (pw_array_length(&parts) == 1) {
        // Assume addr_port contains port (or service name) only.
        // Insert empty string for address part.
        pw_expect_ok( pw_array_insert(&parts, 0, &emptystr) );
    } else {
        PwValue addr = pw_array_item(&parts, 0);
        if (pw_strchr(&addr, ':', 0, nullptr)) {
            // IPv6 address?
            if (pw_startswith(&addr, '[') && pw_endswith(&addr, ']')) {
                // all right
            } else {
                // port is missing
                pw_array_clean(&parts);
                pw_expect_ok( pw_array_append(&parts, addr_port) );
                pw_expect_ok( pw_array_append(&parts, &emptystr) );
            }
        }
    }
    return pw_move(&parts);
}
