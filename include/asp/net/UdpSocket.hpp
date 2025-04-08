#pragma once

#include "SocketAddress.hpp"
#include "SocketBase.hpp"
#include <asp/detail/Result.hpp>
#include <span>

namespace asp::net {
    class UdpSocket : public SocketBase {
    public:
        inline UdpSocket() {}

        Result<void> bind(const SocketAddress& addr);

        inline Result<void> bind(const IpAddress& addr, u16 port) {
            if (addr.isV4()) {
                return this->bind(SocketAddressV4{addr.asV4(), port});
            } else {
                return Err(Error::Ipv6NotSupported);
            }
        }

        Result<void> connect(const SocketAddress& addr);

        inline Result<void> connect(const IpAddress& addr, u16 port) {
            if (addr.isV4()) {
                return this->connect(SocketAddressV4{addr.asV4(), port});
            } else {
                return Err(Error::Ipv6NotSupported);
            }
        }

        Result<SocketAddress> remoteAddress() const;

        /* Send methods (sendto, for unconnected sockets) */

        // Sends data over the socket, returns the number of bytes sent.
        Result<usize> sendTo(const void* data, usize len, const SocketAddress& addr);

        // Sends data over the socket, returns the number of bytes sent.
        inline Result<usize> sendTo(std::string_view data, const SocketAddress& addr) {
            return this->sendTo(data.data(), data.size(), addr);
        }

        // Sends data over the socket, returns the number of bytes sent.
        inline Result<usize> sendTo(std::span<u8> data, const SocketAddress& addr) {
            return this->sendTo(data.data(), data.size(), addr);
        }

        template <size_t Extent>
        inline Result<usize> sendTo(std::span<u8, Extent> data, const SocketAddress& addr) {
            return this->sendTo(data.data(), data.extent, addr);
        }

        // Sends all data over the socket, returns an error if not all data was sent.
        Result<void> sendAllTo(const void* data, usize len, const SocketAddress& addr);

        // Sends all data over the socket, returns an error if not all data was sent.
        inline Result<void> sendAllTo(std::string_view data, const SocketAddress& addr) {
            return this->sendAllTo(data.data(), data.size(), addr);
        }

        // Sends all data over the socket, returns an error if not all data was sent.
        inline Result<void> sendAllTo(std::span<u8> data, const SocketAddress& addr) {
            return this->sendAllTo(data.data(), data.size(), addr);
        }

        template <size_t Extent>
        inline Result<void> sendAllTo(std::span<u8, Extent> data, const SocketAddress& addr) {
            return this->sendAllTo(data.data(), data.extent, addr);
        }

        /* Send methods (for connected sockets, simple inline impls that veridy the remote addr is set and call sendTo) */

        // Sends data over the socket, returns the number of bytes sent. connect() must have been called first.
        inline Result<usize> send(const void* data, usize len) {
            if (!_remoteAddr.port()) {
                return Err(Error::UdpNotConnected);
            }

            return this->sendTo(data, len, _remoteAddr);
        }

        // Sends data over the socket, returns the number of bytes sent. connect() must have been called first.
        inline Result<usize> send(std::string_view data) {
            return this->send(data.data(), data.size());
        }

        // Sends data over the socket, returns the number of bytes sent. connect() must have been called first.
        inline Result<usize> send(std::span<u8> data) {
            return this->send(data.data(), data.size());
        }

        template <size_t Extent>
        inline Result<usize> send(std::span<u8, Extent> data) {
            return this->send(data.data(), data.extent);
        }

        /* Receive methods (recvfrom, explicitly specified address)*/
    private:
        // although udp does not actually connect, we still provide the ability to set a specific remote address
        SocketAddress _remoteAddr{};
    };
}