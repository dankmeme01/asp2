#pragma once

#include <asp/detail/config.hpp>
#include "IpAddress.hpp"
#include "Error.hpp"

namespace asp::net {
    // Resolve a domain name to an IP address. `service` argument is typically the port number.
    AddrInfoResult<IpAddress> getAddrInfo(const std::string& host, const std::string& service = "", bool udp = false, bool ipv6 = false);
}
