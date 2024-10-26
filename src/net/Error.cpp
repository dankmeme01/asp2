#include <asp/net/Error.hpp>

#include <asp/detail/config.hpp>
#include <string>

#ifdef ASP_IS_WIN
# define WIN32_LEAN_AND_MEAN
# include <Windows.h>
#else
# include <errno.h>
#endif

namespace asp::net {
    Error Error::lastOsError() {
#ifdef ASP_IS_WIN
        // note: while this is used for both socket and non-socket errors,
        // WSAGetLastError is just an alias to GetLastError, so it's fine.
        return Error(Error::Type::OSError, GetLastError());
#else
        return Error(Error::Type::OSError, errno);
#endif
    }

    static std::string _libmsg(Error::LibraryErrorType t) {
        switch (t) {
        case Error::SocketNotOpened:
            return "operation attempted on a socket that is not open";
        case Error::AlreadyBound:
            return "the socket is already bound";
        case Error::AlreadyConnected:
            return "the socket is already connected";
        case Error::SocketCreationFailed:
            return "failed to create a socket";
        case Error::BindFailed:
            return "failed to bind the socket";
        case Error::Ipv6NotSupported:
            return "IPv6 is not supported";
        case Error::InvalidSocketPoll:
            return "attemptng to poll on an invalid socket";
        case Error::Unimplemented:
            return "operation is not implemented";
        case Error::TimedOut:
            return "operation timed out";
        case Error::UdpNotConnected:
            return "attempted to perform an operation on a UDP socket without supplying an address or calling connect";
        default:
            return "unknown library error";
        }
    }

    std::string Error::message() const {
        if (type == Type::Library) {
            return _libmsg(static_cast<LibraryErrorType>(_code));
        }

        // format the OS error
#ifdef ASP_IS_WIN
        char* buf = nullptr;
        if (0 == FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr, _code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&buf, 0, nullptr)
        ) {
            auto err = GetLastError();
            return "";
        }

        // sometimes the message has whitespace at the end? trim it
        for (size_t i = strlen(buf); i > 0; i--) {
            if (buf[i - 1] == '\n' || buf[i - 1] == '\r' || buf[i - 1] == ' ' || buf[i - 1] == '\t') {
                buf[i - 1] = '\0';
            } else {
                break;
            }
        }

        auto msg = std::string(buf);
        LocalFree(buf);
        return msg;
#else
        return strerror(_code);
#endif
    }
}