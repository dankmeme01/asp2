#include <asp/net/UdpSocket.hpp>

#include "utils.hpp"
#include <asp/Log.hpp>

namespace asp::net {

// Result<void> UdpSocket::bind(const SocketAddress& addr);

// Windows implementation.
#ifdef ASP_IS_WIN

Result<void> UdpSocket::bind(const SocketAddress& addr) {
    if (_socket != INVALID_SOCKET) {
        return Err(Error::AlreadyBound);
    }

    _socket = ::socket(addr.isV4() ? AF_INET : AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (_socket == INVALID_SOCKET) {
        return Err(Error::lastOsError());
    }

    auto saddr = addressToSockaddrIn(addr);

    if (::bind(_socket, (const sockaddr*) &saddr, sizeof(saddr)) == SOCKET_ERROR) {
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        return Err(Error::lastOsError());
    }

    return Ok();
}

Result<void> UdpSocket::connect(const SocketAddress& addr) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    this->_remoteAddr = addr;

    return Ok();
}

Result<SocketAddress> UdpSocket::remoteAddress() const {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    return Ok(this->_remoteAddr);
}

Result<usize> UdpSocket::sendTo(const void* data, usize len, const SocketAddress& addr) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    auto saddr = addressToSockaddrIn(addr);

    int sent = ::sendto(_socket, (const char*) data, len, 0, (const sockaddr*) &saddr, sizeof(saddr));
    if (sent == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok(sent);
}

Result<void> UdpSocket::sendAllTo(const void* data, usize len, const SocketAddress& addr) {
    usize totalSent = 0;
    while (totalSent < len) {
        auto res = this->sendTo((const char*) data + totalSent, len - totalSent, addr);
        if (!res) {
            return Err(res.unwrapErr());
        }

        totalSent += res.unwrap();
    }

    return Ok();
}

#else // Unix implementation. TODO

#endif

}