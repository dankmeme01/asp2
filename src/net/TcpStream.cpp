#include <asp/net/TcpListener.hpp>

#include "utils.hpp"
#include <asp/Log.hpp>

namespace asp::net {

// Windows implementation.
#ifdef ASP_IS_WIN

Result<void> TcpStream::connect(const SocketAddress& addr, int msTimeout) {
    if (this->isValid()) {
        return Err(Error::AlreadyConnected);
    }

    _socket = ::socket(addr.isV4() ? AF_INET : AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (_socket == INVALID_SOCKET) {
        asp::trace("socket failed with error: " + std::to_string(WSAGetLastError()));
        return Err(Error::lastOsError());
    }

    // set the socket to non-blocking mode
    if (this->setNonBlocking(true).isErr()) {
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        return Err(Error::lastOsError());
    }

    auto saddr = addressToSockaddrIn(addr);

    int code = ::connect(_socket, (const sockaddr*) &saddr, sizeof(saddr));

    if (code != 0 && WSAGetLastError() != WSAEWOULDBLOCK) {
        asp::trace("connect failed with error: " + std::to_string(WSAGetLastError()));
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        return Err(Error::lastOsError());
    }

    // disable non-blocking mode
    if (this->setNonBlocking(false).isErr()) {
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        return Err(Error::lastOsError());
    }

    // if the connection succeeded without blocking, we're done
    if (code == 0) {
        return Ok();
    }

    // otherwise, we need to wait for the connection to complete, so poll the socket
    // for some reason this polls for double the length? TODO investigate
    auto pollres = this->pollWrite(msTimeout / 2);
    if (!pollres) {
        asp::trace("pollWrite failed with error: " + std::to_string(pollres.unwrapErr().code()));
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        return Err(pollres.unwrapErr());
    }

    return Ok();
}

Result<SocketAddress> TcpStream::remoteAddress() const {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    // TODO: this does not support IPv6
    sockaddr_in addr;
    socklen_t namelen = sizeof(addr);

    if (getpeername(_socket, (sockaddr*) &addr, &namelen) == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok(sockaddrToAddress(addr));
}

Result<void> TcpStream::shutdown(ShutdownType type) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    int how;
    switch (type) {
        case ShutdownType::Read:
            how = SD_RECEIVE;
            break;
        case ShutdownType::Write:
            how = SD_SEND;
            break;
        case ShutdownType::Both:
            how = SD_BOTH;
            break;
    }

    if (::shutdown(_socket, how) == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok();
}

Result<void> TcpStream::setNoDelay(bool enable) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    int flag = enable ? 1 : 0;
    if (setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (const char*) &flag, sizeof(flag)) == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok();
}

Result<usize> TcpStream::send(const void* data, usize len) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    int sent = ::send(_socket, (const char*) data, len, 0);
    if (sent == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok(sent);
}

Result<void> TcpStream::sendAll(const void* data, usize len) {
    usize totalSent = 0;
    while (totalSent < len) {
        auto res = this->send((const char*) data + totalSent, len - totalSent);
        if (!res) {
            return Err(res.unwrapErr());
        }

        totalSent += res.unwrap();
    }

    return Ok();
}

Result<usize> TcpStream::receive(void* data, usize len) {
    if (!this->isValid()) {
        return Err(Error::SocketNotOpened);
    }

    int received = ::recv(_socket, (char*) data, len, 0);
    if (received == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    return Ok(received);
}

Result<void> TcpStream::receiveExact(void* data, usize len) {
    usize totalReceived = 0;
    while (totalReceived < len) {
        auto res = this->receive((char*) data + totalReceived, len - totalReceived);
        if (!res) {
            return Err(res.unwrapErr());
        }

        totalReceived += res.unwrap();
    }

    return Ok();
}

#else // Unix implementation. TODO

#endif

}