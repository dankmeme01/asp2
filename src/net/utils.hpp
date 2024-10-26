#pragma once

#include <asp/net/SocketAddress.hpp>
#include <asp/detail/config.hpp>
#include <asp/detail/Result.hpp>

#ifdef ASP_IS_WIN
# include <Ws2tcpip.h>
# define ASP_INVALID_SOCKET INVALID_SOCKET
#else
# include <netinet/in.h>
# define ASP_INVALID_SOCKET (-1)
#endif

namespace asp::net {
    inline sockaddr_in addressToSockaddrIn(const SocketAddress& addr) {
        sockaddr_in out = {};
        if (addr.isV6()) {
            asp::detail::assertionFail("IPv6 is not supported");
        }

        out.sin_family = AF_INET;
        out.sin_port = asp::data::byteswap(addr.port());
        out.sin_addr.s_addr = addr.asV4().ip().toBits();

        return out;
    }

    inline SocketAddress sockaddrToAddress(const sockaddr_in& addr) {
        return SocketAddressV4{
            Ipv4Address::fromBits(addr.sin_addr.s_addr),
            asp::data::byteswap(addr.sin_port)
        };
    }
}