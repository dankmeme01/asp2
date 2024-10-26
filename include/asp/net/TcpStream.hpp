#pragma once

#include "SocketAddress.hpp"
#include "SocketBase.hpp"
#include <asp/detail/Result.hpp>
#include <span>

namespace asp::net {
    class TcpStream : public SocketBase {
    public:
        inline TcpStream() {}

        Result<void> connect(const SocketAddress& addr, int msTimeout = 10000);

        inline Result<void> connect(const IpAddress& addr, u16 port, int msTimeout = 10000) {
            if (addr.isV4()) {
                return this->connect(SocketAddressV4{addr.asV4(), port}, msTimeout);
            } else {
                return Err(Error::Ipv6NotSupported);
            }
        }

        Result<SocketAddress> remoteAddress() const;

        enum class ShutdownType {
            Read,
            Write,
            Both,
        };

        Result<void> shutdown(ShutdownType type);

        Result<void> setNoDelay(bool enable);

        /* Send methods */

        // Sends data over the socket, returns the number of bytes sent.
        Result<usize> send(const void* data, usize len);

        // Sends data over the socket, returns the number of bytes sent.
        inline Result<usize> send(std::string_view data) {
            return this->send(data.data(), data.size());
        }

        // Sends data over the socket, returns the number of bytes sent.
        inline Result<usize> send(std::span<u8> data) {
            return this->send(data.data(), data.size());
        }

        template <size_t Extent>
        Result<usize> send(std::span<u8, Extent> data) {
            return this->send(data.data(), data.extent);
        }

        // Sends all data over the socket, returns an error if not all data was sent.
        Result<void> sendAll(const void* data, usize len);

        // Sends all data over the socket, returns an error if not all data was sent.
        inline Result<void> sendAll(std::string_view data) {
            return this->sendAll(data.data(), data.size());
        }

        // Sends all data over the socket, returns an error if not all data was sent.
        inline Result<void> sendAll(std::span<u8> data) {
            return this->sendAll(data.data(), data.size());
        }

        template <size_t Extent>
        Result<void> sendAll(std::span<u8, Extent> data) {
            return this->sendAll(data.data(), data.extent);
        }

        /* Receive methods */

        // Receives data from the socket, returns the number of bytes received.
        Result<usize> receive(void* data, usize len);

        // Receives data from the socket, blocks until the specified amount of bytes are received.
        Result<void> receiveExact(void* data, usize len);
    };
}
