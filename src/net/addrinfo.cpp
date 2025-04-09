#include <asp/net/addrinfo.hpp>

#include "utils.hpp"

#ifndef ASP_IS_WIN
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
#endif

namespace asp::net {
    AddrInfoResult<IpAddress> getAddrInfo(const std::string& host, const std::string& service, bool udp, bool ipv6) {
        if (host.empty()) {
            return Err(GetAddrInfoError::InvalidInput);
        }

        if (ipv6) {
            return Err(GetAddrInfoError::Ipv6NotSupported);
        }

        // TODO: there may be a need to make this heap allocated? or aligned?
        sockaddr_in outAddr;
        outAddr.sin_family = AF_INET;

        // check if the input is an IP address, if so, we got lucky
        if (auto addr = Ipv4Address::tryFromString(host)) {
            return Ok(addr.unwrap());
        }

        // otherwise, we need to resolve the hostname
        struct addrinfo hints = {};
        hints.ai_family = ipv6 ? AF_INET6 : AF_INET;

        // TODO: idk if these actually matter
        if (udp) {
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_protocol = IPPROTO_UDP;
        } else {
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
        }

        struct addrinfo* result;
        int code = ::getaddrinfo(host.c_str(), service.empty() ? nullptr : service.c_str(), &hints, &result);

        if (code == 0) {
            auto ip = (struct sockaddr_in*)result->ai_addr;
            auto addr = Ipv4Address::fromBits(asp::data::byteswap(ip->sin_addr.s_addr));
            ::freeaddrinfo(result);
            return Ok(addr);
        }

        GetAddrInfoError err;
        using enum GetAddrInfoError;

        switch (code) {
            case EAI_AGAIN:
                err = TemporaryFailure;
                break;
            case EAI_BADFLAGS:
                err = InvalidFlags;
                break;
            case EAI_FAIL:
                err = NonRecoverableFailure;
                break;
            case EAI_FAMILY:
                err = InvalidFamily;
                break;
            case EAI_MEMORY:
                err = OutOfMemory;
                break;
            case EAI_NONAME:
                err = HostNotFound;
                break;
            case EAI_SERVICE:
                err = ServiceNotFound;
                break;
            case EAI_SOCKTYPE:
                err = SocketTypeNotSupported;
                break;
            default:
                err = Unknown;
                break;
        }

        ::freeaddrinfo(result);
        return Err(err);
    }
}
