#pragma once

#include <asp/data/nums.hpp>
#include <asp/data/util.hpp>
#include <string_view>
#include <string>
#include <array>

#include "Error.hpp"

namespace asp::net {
    namespace detail {
        Result<u16, void> tryParsePort(std::string_view str);
        Result<u8, void> tryParseOctet(std::string_view str);
    }

    // Represents an IPv4 address.
    class Ipv4Address {
    public:
        // Represents 127.0.0.1
        static Ipv4Address LOCALHOST;

        // Represents 0.0.0.0
        static Ipv4Address UNSPECIFIED;

        // Represents 255.255.255.255
        static Ipv4Address BROADCAST;

        constexpr inline Ipv4Address() : _octets{0, 0, 0, 0} {};
        constexpr inline Ipv4Address(const Ipv4Address&) = default;
        constexpr inline Ipv4Address(Ipv4Address&&) = default;
        constexpr inline Ipv4Address& operator=(const Ipv4Address&) = default;
        constexpr inline Ipv4Address& operator=(Ipv4Address&&) = default;

        constexpr inline Ipv4Address(u8 octet1, u8 octet2, u8 octet3, u8 octet4) : _octets{octet1, octet2, octet3, octet4} {}

        // Parse an address in format `ip:port`, throws an exception or a compile error if invoked in a constexpr context.
        constexpr static Ipv4Address fromString(std::string_view str) {
            Ipv4Address out;

            // parse the address
            size_t lastPos = 0;
            for (size_t i = 0; i < 4; i++) {
                size_t dotPos = str.find('.', lastPos);
                if (dotPos == std::string_view::npos && i != 3) {
                    if (std::is_constant_evaluated()) {
                        data::_constexprFail();
                    } else {
                        asp::detail::assertionFail("invalid IP range format (missing octet)");
                    }
                } else if (dotPos == std::string_view::npos) {
                    dotPos = str.size();
                }

                std::string_view octetStr = str.substr(lastPos, dotPos - lastPos);
                out._octets[i] = data::constexprParse<u8>(octetStr);

                lastPos = dotPos + 1;
            }

            return out;
        }

        static inline Result<Ipv4Address, AddressParseError> tryFromString(std::string_view str) {
            Ipv4Address out;

            // parse the address
            size_t lastPos = 0;
            for (size_t i = 0; i < 4; i++) {
                size_t dotPos = str.find('.', lastPos);
                if (dotPos == std::string_view::npos && i != 3) {
                    return Err(AddressParseError::MissingOctets);
                } else if (dotPos == std::string_view::npos) {
                    dotPos = str.size();
                }

                std::string_view octetStr = str.substr(lastPos, dotPos - lastPos);
                auto octetRes = detail::tryParseOctet(octetStr);
                if (!octetRes) {
                    return Err(AddressParseError::InvalidOctet);
                }

                out._octets[i] = octetRes.unwrap();

                lastPos = dotPos + 1;
            }

            return Ok(out);
        }

        constexpr inline std::string toString() const {
            if (!std::is_constant_evaluated()) {
                return _rtToString();
            } else {
                std::string str;
                str.reserve(15);

                for (size_t i = 0; i < 4; i++) {
                    str += data::constexprToString(_octets[i]);
                    if (i != 3) {
                        str += '.';
                    }
                }

                return str;
            }
        }

        constexpr inline static Ipv4Address fromBits(u32 address) {
            // assume address is big endian
            return Ipv4Address(
                (address >> 24) & 0xFF,
                (address >> 16) & 0xFF,
                (address >> 8) & 0xFF,
                address & 0xFF
            );
        }

        // Returns the address as a 32-bit integer in big endian.
        constexpr inline u32 toBits() const {
            return (_octets[0] << 24) | (_octets[1] << 16) | (_octets[2] << 8) | _octets[3];
        }

        constexpr inline std::array<u8, 4> octets() const {
            return _octets;
        }

        // Returns whether the address is unspecified (0.0.0.0)
        constexpr inline bool isUnspecified() const {
            return _octets[0] == 0 && _octets[1] == 0 && _octets[2] == 0 && _octets[3] == 0;
        }

        // Returns whether the address is a loopback address (127.0.0.0/8)
        constexpr inline bool isLoopback() const {
            return _octets[0] == 127;
        }

        // Returns whether the address is an address in a private range (as per RFC 1918)
        constexpr bool isPrivate() const {
            return (_octets[0] == 10) ||
                   (_octets[0] == 172 && _octets[1] >= 16 && _octets[1] <= 31) ||
                   (_octets[0] == 192 && _octets[1] == 168);
        }

        // Returns whether the address is a broadcast address
        constexpr inline bool isBroadcast() const {
            return _octets[0] == 255 && _octets[1] == 255 && _octets[2] == 255 && _octets[3] == 255;
        }

    private:
        std::array<u8, 4> _octets;

        friend constexpr bool operator==(const Ipv4Address&, const Ipv4Address&);
        friend constexpr bool operator!=(const Ipv4Address&, const Ipv4Address&);
        friend constexpr std::strong_ordering operator<=>(const Ipv4Address&, const Ipv4Address&);

        // Returns the address as a string, more optimized for runtime.
        std::string _rtToString() const {
            std::string str;
            str.reserve(15);

            for (size_t i = 0; i < 4; i++) {
                str += std::to_string(_octets[i]);
                if (i != 3) {
                    str += '.';
                }
            }

            return str;
        }
    };

    // comparison operators
    constexpr inline bool operator==(const Ipv4Address& lhs, const Ipv4Address& rhs) {
        return lhs._octets == rhs._octets;
    }

    constexpr inline bool operator!=(const Ipv4Address& lhs, const Ipv4Address& rhs) {
        return !(lhs == rhs);
    }

    constexpr inline std::strong_ordering operator<=>(const Ipv4Address& lhs, const Ipv4Address& rhs) {
#ifdef __APPLE__
        if (lhs._octets == rhs._octets) {
            return std::strong_ordering::equal;
        }

        return lhs._octets < rhs._octets ? std::strong_ordering::less : std::strong_ordering::greater;
#else
        return lhs._octets <=> rhs._octets;
#endif
    }

    // Represents an IPv6 address.
    class Ipv6Address {
    public:
        // TODO
    };

    // Represents an IP address, which can be either an IPv4 or IPv6 address.
    class IpAddress {
    public:
        constexpr inline IpAddress() : _isV6(false), _v4() {}
        constexpr inline IpAddress(const IpAddress&) = default;
        constexpr inline IpAddress(IpAddress&&) = default;
        constexpr inline IpAddress& operator=(const IpAddress&) = default;
        constexpr inline IpAddress& operator=(IpAddress&&) = default;

        constexpr inline IpAddress(const Ipv4Address& addr) : _isV6(false), _v4(addr) {}
        constexpr inline IpAddress(const Ipv6Address& addr) : _isV6(true), _v6(addr) {}

        constexpr inline bool isV6() const {
            return _isV6;
        }

        constexpr inline bool isV4() const {
            return !_isV6;
        }

        constexpr inline Ipv4Address asV4() const {
            if (!isV4()) {
                if (std::is_constant_evaluated()) {
                    data::_constexprFail();
                } else {
                    asp::detail::assertionFail("IP address is not an IPv4 address");
                }
            }

            return _v4;
        }

        constexpr inline Ipv6Address asV6() const {
            if (!isV6()) {
                if (std::is_constant_evaluated()) {
                    data::_constexprFail();
                } else {
                    asp::detail::assertionFail("IP address is not an IPv6 address");
                }
            }

            return _v6;
        }

    private:
        constexpr friend bool operator==(const IpAddress&, const IpAddress&);
        constexpr friend bool operator!=(const IpAddress&, const IpAddress&);
        constexpr friend std::strong_ordering operator<=>(const IpAddress&, const IpAddress&);

        bool _isV6;
        union {
            Ipv4Address _v4;
            Ipv6Address _v6;
        };
    };

    // comparison operators
    constexpr inline bool operator==(const IpAddress& lhs, const IpAddress& rhs) {
        if (lhs.isV4() && rhs.isV4()) {
            return lhs.asV4() == rhs.asV4();
        } else if (lhs.isV6() && rhs.isV6()) {
            return lhs.asV6() == rhs.asV6();
        } else {
            return false;
        }
    }

    constexpr inline bool operator!=(const IpAddress& lhs, const IpAddress& rhs) {
        return !(lhs == rhs);
    }

    constexpr inline std::strong_ordering operator<=>(const IpAddress& lhs, const IpAddress& rhs) {
        if (lhs.isV4() && rhs.isV4()) {
            return lhs.asV4() <=> rhs.asV4();
        } else if (lhs.isV6() && rhs.isV6()) {
            return lhs.asV6() <=> rhs.asV6();
        } else {
            return lhs.isV4() <=> rhs.isV4();
        }
    }
}

namespace std {
    template <>
    struct hash<asp::net::Ipv4Address> {
        size_t operator()(const asp::net::Ipv4Address& addr) const {
            return std::hash<asp::u32>{}(addr.toBits());
        }
    };

    template <>
    struct hash<asp::net::Ipv6Address> {
        size_t operator()(const asp::net::Ipv6Address& addr) const {
            // TODO
            return 0;
        }
    };

    template <>
    struct hash<asp::net::IpAddress> {
        size_t operator()(const asp::net::IpAddress& addr) const {
            if (addr.isV4()) {
                return std::hash<asp::net::Ipv4Address>{}(addr.asV4());
            } else {
                return std::hash<asp::net::Ipv6Address>{}(addr.asV6());
            }
        }
    };
}
