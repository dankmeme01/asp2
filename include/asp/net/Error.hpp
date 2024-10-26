#pragma once

#include <asp/detail/Result.hpp>
#include <asp/data/nums.hpp>

namespace asp::net {
    class Error {
    public:
        enum class Type : u8 {
            Library,
            OSError,
        };

        enum LibraryErrorType {
            SocketNotOpened,
            AlreadyBound,
            AlreadyConnected,
            SocketCreationFailed,
            BindFailed,
            Ipv6NotSupported,
            InvalidSocketPoll,
            Unimplemented,
            TimedOut,
            UdpNotConnected,
        };

        constexpr inline Error(Type t, int code) : type(t), _code(code) {}
        constexpr inline Error(LibraryErrorType e) : Error(Type::Library, static_cast<int>(e)) {}

        static Error lastOsError();

        constexpr inline int code() const {
            return _code;
        }

        constexpr inline Type errorType() const {
            return type;
        }

        std::string message() const;

        bool operator==(const Error& other) const {
            return type == other.type && _code == other._code;
        }

        bool operator!=(const Error& other) const {
            return !(*this == other);
        }

        bool operator==(LibraryErrorType e) const {
            return type == Type::Library && _code == static_cast<int>(e);
        }

        bool operator!=(LibraryErrorType e) const {
            return !(*this == e);
        }

    private:
        Type type;
        int _code;
    };

    template <typename T, typename E = Error>
    using Result = _r::Result<T, E>;

    using _r::Ok;
    using _r::Err;

    enum class AddressParseError {
        MissingOctets,
        InvalidOctet,
        InvalidPort,
        InvalidStructure,
    };

    enum class GetAddrInfoError {
        InvalidInput,
        Ipv6NotSupported,

        // EAI_ errors
        TemporaryFailure,
        InvalidFlags,
        NonRecoverableFailure,
        InvalidFamily,
        OutOfMemory,
        HostNotFound,
        ServiceNotFound,
        SocketTypeNotSupported,
        Unknown
    };

    template <typename T, typename E = GetAddrInfoError>
    using AddrInfoResult = Result<T, E>;
}
