#pragma once

#include "TcpStream.hpp"

namespace asp::net {
    class TcpListener : SocketBase {
    public:
        inline TcpListener() {}

        // Binds the listener to the given address.
        Result<void> bind(const SocketAddress& addr);

        // Binds the listener to the given address and port, if port is 0, the OS will choose an available port number.
        inline Result<void> bind(const IpAddress& addr, u16 port) {
            if (addr.isV4()) {
                return this->bind(SocketAddressV4{addr.asV4(), port});
            } else {
                return Err(Error::Ipv6NotSupported);
            }
        }

        // Starts listening for incoming connections.
        Result<void> listen(int backlog);

        // Accepts an incoming connection, returns a TcpStream and the address of the remote peer.
        Result<std::pair<TcpStream, SocketAddress>> accept();

    private:
    };
}
