#include <asp/net/IpAddress.hpp>
#include <string_view>
#include <charconv>

namespace asp::net {
    Ipv4Address Ipv4Address::LOCALHOST{127, 0, 0, 1};
    Ipv4Address Ipv4Address::UNSPECIFIED{0, 0, 0, 0};
    Ipv4Address Ipv4Address::BROADCAST{255, 255, 255, 255};

    template <typename T>
    static Result<T, void> parseNum(std::string_view str) {
        T num;

        auto result = std::from_chars(&*str.begin(), &*str.end(), num);
        if (result.ec != std::errc()) {
            return Err();
        }

        return Ok(num);
    }

    Result<u16, void> detail::tryParsePort(std::string_view str) {
        return parseNum<u16>(str);
    }

    Result<u8, void> detail::tryParseOctet(std::string_view str) {
        return parseNum<u8>(str);
    }
}
