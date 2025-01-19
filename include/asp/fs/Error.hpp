#pragma once

#include <asp/detail/Result.hpp>
#include <system_error>

namespace asp::fs {
    class Error {
    public:
        inline Error(std::error_code ec) : ec(ec) {}

        inline const std::error_code& getCode() const {
            return ec;
        }

        inline std::string message() const {
            return ec.message();
        }

    private:
        std::error_code ec;
    };

    template <typename T = void, typename E = Error>
    using Result = asp::Result<T, E>;
}
