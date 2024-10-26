#include <asp/net/poll.hpp>
#include <asp/Log.hpp>

#include "utils.hpp"

namespace asp::net {

// Windows implementation.
#ifdef ASP_IS_WIN

// note: not tested yet at all, written with a lot of copilot help so it might not work at all

Result<usize> poll(SocketBase** sockets, usize count, PollType type, int msTimeout) {
    ASP_ASSERT(count <= 1024, "Cannot poll more than 1024 sockets at once");

    WSAPOLLFD* fds = (WSAPOLLFD*) alloca(count * sizeof(WSAPOLLFD));
    for (usize i = 0; i < count; i++) {
        fds[i].fd = sockets[i]->rawHandle();
        fds[i].events = type == PollType::Read ? POLLRDNORM : type == PollType::Write ? POLLWRNORM : POLLRDNORM | POLLWRNORM;
    }

    int res = WSAPoll(fds, count, msTimeout);

    if (res == SOCKET_ERROR) {
        return Err(Error::lastOsError());
    }

    for (usize i = 0; i < count; i++) {
        if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            asp::trace("poll failed with error: " + std::to_string(fds[i].revents));
            asp::trace("fd = " + std::to_string(fds[i].fd));
            return Err(Error::InvalidSocketPoll);
        }

        if ((fds[i].revents & POLLRDNORM) && type == PollType::Read) {
            return Ok(i);
        } else if ((fds[i].revents & POLLWRNORM) && type == PollType::Write) {
            return Ok(i);
        } else if ((fds[i].revents & (POLLRDNORM | POLLWRNORM)) && type == PollType::ReadWrite) {
            return Ok(i);
        }
    }

    return Err(Error::TimedOut);
}

#else // Unix implementation. TODO

#endif

}