#include <asp/net/TcpListener.hpp>

#include "utils.hpp"

namespace asp::net {

// Windows implementation.
#ifdef ASP_IS_WIN

Result<void> TcpListener::bind(const SocketAddress& addr) {
    if (_socket != INVALID_SOCKET) {
        return Err(Error::AlreadyBound);
    }

    _socket = ::socket(addr.isV4() ? AF_INET : AF_INET6, SOCK_STREAM, IPPROTO_TCP);
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

Result<void> TcpListener::listen(int backlog) {
    if (::listen(_socket, backlog) == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok();
}

Result<std::pair<TcpStream, SocketAddress>> TcpListener::accept() {
    return Err(Error::Unimplemented);
}

#else // Unix implementation. TODO

#endif

}