#include <asp/net/SocketBase.hpp>
#include <asp/time/Duration.hpp>

#include "utils.hpp"

namespace asp::net {

// Common code

SocketBase::SocketBase() : _socket(ASP_INVALID_SOCKET) {}

SocketBase::SocketBase(SocketBase&& other) : _socket(other._socket) {
    other._socket = ASP_INVALID_SOCKET;
}

SocketBase& SocketBase::operator=(SocketBase&& other) {
    if (this == &other) {
        return *this;
    }

    if (_socket != ASP_INVALID_SOCKET) {
        closesocket(_socket);
    }

    _socket = other._socket;
    other._socket = ASP_INVALID_SOCKET;

    return *this;
}

bool SocketBase::isValid() const {
    return _socket != ASP_INVALID_SOCKET;
}

Result<bool> SocketBase::pollRead(const asp::time::Duration& dur) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    return this->pollRead(dur.millis());
}

Result<bool> SocketBase::pollWrite(const asp::time::Duration& dur) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    return this->pollWrite(dur.millis());
}

// Windows implementation.

#ifdef ASP_IS_WIN

SocketBase::~SocketBase() {
    if (_socket != INVALID_SOCKET) {
        closesocket(_socket);
    }
}

Result<void> SocketBase::close() {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    if (::closesocket(_socket) == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    _socket = INVALID_SOCKET;
    return Ok();
}

Result<void> SocketBase::setReadTimeout(int ms) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    DWORD timeout = ms;
    if (setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout)) == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok();
}

Result<void> SocketBase::setWriteTimeout(int ms) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    DWORD timeout = ms;
    if (setsockopt(_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*) &timeout, sizeof(timeout)) == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok();
}

Result<void> SocketBase::setNonBlocking(bool nonBlocking) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    u_long mode = nonBlocking ? 1 : 0;
    if (ioctlsocket(_socket, FIONBIO, &mode) == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    this->nonBlocking = nonBlocking;

    return Ok();
}

bool SocketBase::isNonBlocking() const {
    return nonBlocking;
}

Result<void> SocketBase::setReuseAddr(bool reuseAddr) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    int optval = reuseAddr ? 1 : 0;
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (const char*) &optval, sizeof(optval)) == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok();
}

Result<void> SocketBase::setNoDelay(bool noDelay) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    int optval = noDelay ? 1 : 0;
    if (setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (const char*) &optval, sizeof(optval)) == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok();
}

Result<bool> SocketBase::pollRead(int msDelay) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    WSAPOLLFD fds[1];
    fds[0].fd = _socket;
    fds[0].events = POLLIN;

    int result = WSAPoll(fds, 1, msDelay);
    if (result == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok(result > 0);
}

Result<bool> SocketBase::pollWrite(int msDelay) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    WSAPOLLFD fds[1];
    fds[0].fd = _socket;
    fds[0].events = POLLOUT;

    int result = WSAPoll(fds, 1, msDelay);
    if (result == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok(result > 0);
}

Result<SocketAddress> SocketBase::localAddress() {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    // TODO: this does not support IPv6
    sockaddr_in addr;
    socklen_t namelen = sizeof(addr);

    if (getsockname(_socket, (sockaddr*) &addr, &namelen) == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok(sockaddrToAddress(addr));
}

#else // Unix implementation. TODO

SocketBase::~SocketBase() {
    if (_socket != -1) {
        close(_socket);
    }
}

#endif

}