#pragma once

#include "IpAddress.hpp"
#include "Error.hpp"
#include <string>

namespace asp::net {
    // Represents an ipv4 socket address (IP address and port).
    class SocketAddressV4 {
    public:
        constexpr SocketAddressV4() : _address(), _port(0) {}
        constexpr SocketAddressV4(const Ipv4Address& address, u16 port) : _address(address), _port(port) {}
        constexpr SocketAddressV4(const SocketAddressV4& other) = default;
        constexpr SocketAddressV4(SocketAddressV4&& other) = default;
        constexpr SocketAddressV4& operator=(const SocketAddressV4& other) = default;
        constexpr SocketAddressV4& operator=(SocketAddressV4&& other) = default;

        constexpr const Ipv4Address& address() const {
            return _address;
        }

        constexpr const Ipv4Address& ip() const {
            return _address;
        }

        constexpr u16 port() const {
            return _port;
        }

        constexpr void setAddress(const Ipv4Address& address) {
            _address = address;
        }

        constexpr void setPort(u16 port) {
            _port = port;
        }

        // Parse an address in format `ip:port`, throws an exception or a compile error if invoked in a constexpr context.
        constexpr static SocketAddressV4 fromString(std::string_view str) {
            size_t colonPos = str.find(':');
            if (colonPos == std::string_view::npos) {
                if (std::is_constant_evaluated()) {
                    data::_constexprFail();
                } else {
                    ::asp::detail::assertionFail("invalid socket address format (missing colon)");
                }
            }

            Ipv4Address addr = Ipv4Address::fromString(str.substr(0, colonPos));
            u16 port = data::constexprParse<u16>(str.substr(colonPos + 1));

            return SocketAddressV4{addr, port};
        }

        inline static Result<SocketAddressV4, AddressParseError> tryFromString(std::string_view str) {
            size_t colonPos = str.find(':');
            if (colonPos == std::string_view::npos) {
                return Err(AddressParseError::InvalidStructure);
            }

            auto addrRes = Ipv4Address::tryFromString(str.substr(0, colonPos));
            if (!addrRes) {
                return Err(addrRes.unwrapErr());
            }

            auto portRes = detail::tryParsePort(str.substr(colonPos + 1));
            if (!portRes) {
                return Err(AddressParseError::InvalidPort);
            }

            return Ok(SocketAddressV4{addrRes.unwrap(), portRes.unwrap()});
        }

        constexpr inline std::string toString() const {
            if (!std::is_constant_evaluated()) {
                std::string str = _address.toString();
                str += ':';
                str += std::to_string(_port);
                return str;
            } else {
                return _address.toString() + ':' + data::constexprToString(_port);
            }
        }

    private:
        Ipv4Address _address;
        u16 _port;

        constexpr friend bool operator==(const SocketAddressV4&, const SocketAddressV4&);
        constexpr friend bool operator!=(const SocketAddressV4&, const SocketAddressV4&);
        friend struct std::hash<SocketAddressV4>;
    };

    constexpr bool operator==(const SocketAddressV4& lhs, const SocketAddressV4& rhs) {
        return lhs._address == rhs._address && lhs._port == rhs._port;
    }

    constexpr bool operator!=(const SocketAddressV4& lhs, const SocketAddressV4& rhs) {
        return !(lhs == rhs);
    }

    // Represents an ipv6 socket address (IP address and port).
    class SocketAddressV6 {
    public:
        // TODO
    private:
        friend struct std::hash<SocketAddressV4>;
    };

    // Represents a socket address, which can be either an ipv4 or ipv6 address plus a port.
    class SocketAddress {
    public:
        constexpr inline SocketAddress() : _isV6(false), _v4() {}
        constexpr inline SocketAddress(const SocketAddress&) = default;
        constexpr inline SocketAddress(SocketAddress&&) = default;
        constexpr inline SocketAddress& operator=(const SocketAddress&) = default;
        constexpr inline SocketAddress& operator=(SocketAddress&&) = default;

        constexpr inline SocketAddress(const SocketAddressV4& addr) : _isV6(false), _v4(addr) {}
        constexpr inline SocketAddress(const SocketAddressV6& addr) : _isV6(true), _v6(addr) {}
        constexpr inline SocketAddress(const IpAddress& addr, u16 port) : _isV6(addr.isV6()) {
            if (addr.isV4()) {
                _v4 = SocketAddressV4{addr.asV4(), port};
            } else {
                // _v6 = SocketAddressV6{addr.asV6(), port};
                asp::detail::assertionFail("ipv6 not supported yet");
            }
        }

        constexpr inline bool isV6() const {
            return _isV6;
        }

        constexpr inline bool isV4() const {
            return !_isV6;
        }

        constexpr inline SocketAddressV4 asV4() const {
            if (!isV4()) {
                if (std::is_constant_evaluated()) {
                    data::_constexprFail();
                } else {
                    asp::detail::assertionFail("socket address is not an ipv4 address");
                }
            }

            return _v4;
        }

        constexpr inline SocketAddressV6 asV6() const {
            if (!isV6()) {
                if (std::is_constant_evaluated()) {
                    data::_constexprFail();
                } else {
                    asp::detail::assertionFail("socket address is not an ipv6 address");
                }
            }

            return _v6;
        }

        constexpr inline u16 port() const {
            if (isV4()) {
                return _v4.port();
            } else {
                return 0; // TODO
            }
        }
    private:
        bool _isV6;
        union {
            SocketAddressV4 _v4;
            SocketAddressV6 _v6;
        };

        constexpr friend bool operator==(const SocketAddress&, const SocketAddress&);
        constexpr friend bool operator!=(const SocketAddress&, const SocketAddress&);
        friend struct std::hash<SocketAddress>;
    };

    constexpr bool operator==(const SocketAddress& lhs, const SocketAddress& rhs) {
        if (lhs.isV4() && rhs.isV4()) {
            return lhs._v4 == rhs._v4;
        } else if (lhs.isV6() && rhs.isV6()) {
            return false; // TODO
        } else {
            return false;
        }
    }

    constexpr bool operator!=(const SocketAddress& lhs, const SocketAddress& rhs) {
        return !(lhs == rhs);
    }
}

namespace std {
    template <>
    struct hash<asp::net::SocketAddressV4> {
        size_t operator()(const asp::net::SocketAddressV4& addr) const {
            auto h1 = std::hash<asp::net::Ipv4Address>{}(addr.address());
            auto h2 = std::hash<asp::u16>{}(addr.port());

            return h1 ^ (h2 << 1);
        }
    };

    template <>
    struct hash<asp::net::SocketAddressV6> {
        size_t operator()(const asp::net::SocketAddressV6& addr) const {
            return 0; // TODO
            // auto h1 = std::hash<asp::net::Ipv6Address>{}(addr.address());
            // auto h2 = std::hash<u16>{}(addr.port());

            // return h1 ^ (h2 << 1);
        }
    };

    template <>
    struct hash<asp::net::SocketAddress> {
        size_t operator()(const asp::net::SocketAddress& addr) const {
            if (addr.isV4()) {
                return std::hash<asp::net::SocketAddressV4>{}(addr.asV4());
            } else {
                return std::hash<asp::net::SocketAddressV6>{}(addr.asV6());
            }
        }
    };
}

