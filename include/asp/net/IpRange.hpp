#pragma once

#include <asp/data/nums.hpp>
#include <asp/data/util.hpp>
#include <asp/detail/config.hpp>
#include <asp/net/IpAddress.hpp>
#include <array>
#include <string_view>

namespace asp::net {
    // Represents a contiguous IP range (i.e. 192.168.0.0/16)
    // Someone please test this idk if any of this works and also probably it doesn't compile on msvc lol
    class IpRange {
    public:
        constexpr IpRange() : _octets{0, 0, 0, 0}, _prefixLength(0) {}
        constexpr IpRange(const IpRange&) = default;
        constexpr IpRange(IpRange&&) = default;
        constexpr IpRange& operator=(const IpRange&) = default;
        constexpr IpRange& operator=(IpRange&&) = default;

        constexpr static IpRange fromString(std::string_view str) {
            // first check if the format is correct
            size_t slashPos = str.find('/');
            if (slashPos == std::string_view::npos) {
                if (std::is_constant_evaluated()) {
                    data::_constexprFail();
                } else {
                    ::asp::detail::assertionFail("invalid IP range format (missing a slash)");
                }
            }

            std::string_view addrStr = str.substr(0, slashPos);
            std::string_view prefixStr = str.substr(slashPos + 1);

            IpRange range;
            range._prefixLength = data::constexprParse<u8>(prefixStr);

            if (range._prefixLength > 32) {
                if (std::is_constant_evaluated()) {
                    data::_constexprFail();
                } else {
                    ::asp::detail::assertionFail("invalid IP range format (prefix length too large)");
                }
            }

            range._octets = Ipv4Address::fromString(addrStr).octets();

            return range;
        }

        constexpr inline std::string toString() const {
            if (!std::is_constant_evaluated()) {
                return _rtToString();
            } else {
                std::string str;
                str.reserve(19); // 15 for the IP address, 1 for the slash, 3 for the prefix length

                for (size_t i = 0; i < 4; i++) {
                    str += data::constexprToString(_octets[i]);
                    if (i != 3) {
                        str += '.';
                    }
                }

                str += '/';
                str += data::constexprToString(_prefixLength);

                return str;
            }
        }

        constexpr inline std::array<u8, 4> octets() const {
            return _octets;
        }

        constexpr inline u8 prefixLength() const {
            return _prefixLength;
        }

        // Returns whether the given address is in this range.
        constexpr inline bool contains(const Ipv4Address& addr) const {
            u32 addrBits = addr.toBits();
            u32 rangeBits = this->toBits();

            u32 mask = 0xFFFFFFFF << (32 - _prefixLength);
            return (addrBits & mask) == (rangeBits & mask);
        }

    private:
        friend struct std::hash<IpRange>;

        std::array<u8, 4> _octets;
        u8 _prefixLength;

        constexpr inline u32 toBits() const {
            return (_octets[0] << 24) | (_octets[1] << 16) | (_octets[2] << 8) | _octets[3];
        }

        // Returns the range as a string, more optimized for runtime.
        std::string _rtToString() const {
            std::string str;
            str.reserve(19); // 15 for the IP address, 1 for the slash, 3 for the prefix length

            for (size_t i = 0; i < 4; i++) {
                str += std::to_string(_octets[i]);
                if (i != 3) {
                    str += '.';
                }
            }

            str += '/';
            str += std::to_string(_prefixLength);

            return str;
        }
    };
}

namespace std {
    template <>
    struct hash<asp::net::IpRange> {
        size_t operator()(const asp::net::IpRange& range) const {
            return std::hash<asp::u32>{}(range.toBits());
        }
    };
}
