#if 0

#include <asp/fs/LockedFile.hpp>
#include <asp/sync/Mutex.hpp>
#include <csignal>
#include <unordered_map>
#include <stdio.h>
#include <fcntl.h>

// Unix implementation of LockedFile

namespace asp::fs {

void timeoutHandler(int sig) {}

Result<> LockedFile::open(const path& p, const LockedFileOptions& options) {
    if (this->isOpen()) {
        return Err(std::make_error_code(std::errc::invalid_argument));
    }

    int flags = O_RDWR;
    if (options.create) {
        flags |= O_CREAT;
    }

    auto str = p.u8string();
    int fd = ::open((const char*) str.c_str(), flags);

    if (fd == InvalidFd) {
        return Err(std::make_error_code(std::errc{errno}));
    }

    // acquire lock
    struct flock lock = {
        .l_type = (short) (options.shared ? F_RDLCK : F_WRLCK),
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0
    };

    int result = ::fcntl(fd, F_SETLKW, &lock);
    if (result == -1) {
        ::close(fd);
        return Err(std::make_error_code(std::errc{errno}));
    }

    // we now acquired the lock!
    m_fd = fd;

    return Ok();
}

Result<> LockedFile::openTimeout(const path& p, const LockedFileOptions& options, const time::Duration& timeout) {
    if (this->isOpen()) {
        return Err(std::make_error_code(std::errc::invalid_argument));
    }

    int flags = O_RDWR;
    if (options.create) {
        flags |= O_CREAT;
    }

    auto str = p.u8string();
    int fd = ::open((const char*) str.c_str(), flags);

    if (fd == InvalidFd) {
        return Err(std::make_error_code(std::errc{errno}));
    }

    // acquire lock
    struct flock lock = {
        .l_type = (short) (options.shared ? F_RDLCK : F_WRLCK),
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0
    };

    // set an alarm to be called in `timeout` seconds
    struct sigaction sa, old_sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = timeoutHandler;
    sigaction(SIGALRM, &sa, &old_sa);

    alarm(timeout.seconds());

    errno = 0;
    int result = ::fcntl(fd, F_SETLKW, &lock);

    // clear alarm and restore sig handlers
    alarm(0);
    sigaction(SIGALRM, &old_sa, NULL);

    if (result == -1) {
        ::close(fd);

        if (errno == EINTR) {
            return Err(std::make_error_code(std::errc::timed_out));
        } else {
            return Err(std::make_error_code(std::errc{errno}));
        }
    }

    // we now acquired the lock!
    m_fd = fd;

    return Ok();
}

Result<> LockedFile::close() {
    // release lock
    struct flock lock = {
        .l_type = (short) (F_UNLCK),
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0
    };

    int result = ::fcntl(m_fd, F_SETLKW, &lock);

    ::close(m_fd);

    m_fd = InvalidFd;

    if (result == -1) {
        return Err(std::make_error_code(std::errc{errno}));
    }

    return Ok();
}

} // namespace asp::fs

#endif