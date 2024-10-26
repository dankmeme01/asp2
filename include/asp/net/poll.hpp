#pragma once

#include "SocketBase.hpp"
#include <array>

namespace asp::net {
    enum class PollType {
        Read,
        Write,
        ReadWrite
    };

    // polls multiple sockets at once, returns the index of the first socket that is ready
    Result<usize> poll(SocketBase** sockets, usize count, PollType type, int msTimeout);

    // polls multiple sockets at once, returns the index of the first socket that is ready
    template <usize N>
    Result<usize> poll(std::array<SocketBase*, N> arr, PollType type, int msTimeout) {
        return poll(arr.data(), N, type, msTimeout);
    }

    // polls multiple sockets at once, returns the index of the first socket that is ready
    template <typename... Sock>
    Result<usize> poll(Sock&... sockets, PollType type, int msTimeout) {
        std::array<SocketBase*, sizeof...(Sock)> arr = {&sockets...};
        return poll(arr, type, msTimeout);
    }
}