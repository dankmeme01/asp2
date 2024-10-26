#pragma once

#include <asp/data/nums.hpp>
#include <asp/detail/config.hpp>
#include "SocketAddress.hpp"
#include "Error.hpp"

namespace asp::time {
    class Duration;
}

namespace asp::net {
    class SocketBase {
    protected:
#ifdef ASP_IS_WIN
        u64 _socket; // SOCKET is just u64
#else
        int _socket;
#endif

        bool nonBlocking = false;

        SocketBase();
        ~SocketBase();

        SocketBase(const SocketBase& other) = delete;
        SocketBase& operator=(const SocketBase& other) = delete;

        SocketBase(SocketBase&& other);
        SocketBase& operator=(SocketBase&& other);

        Result<void> close();

        bool isValid() const;

        Result<void> setReadTimeout(int ms);
        Result<void> setWriteTimeout(int ms);

        Result<void> setNonBlocking(bool nonBlocking);
        bool isNonBlocking() const;

        Result<void> setReuseAddr(bool reuseAddr);
        Result<void> setNoDelay(bool noDelay);

        Result<bool> pollRead(int msDelay);
        Result<bool> pollWrite(int msDelay);

        Result<bool> pollRead(const asp::time::Duration& dur);
        Result<bool> pollWrite(const asp::time::Duration& dur);

        Result<SocketAddress> localAddress();

    public:
        inline decltype(_socket)& rawHandle() {
            return _socket;
        }
    };
}