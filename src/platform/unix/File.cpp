#include <asp/fs.hpp>
#include <asp/iter.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "helpers.hpp"

namespace asp::fs {

void File::close() {
    if (m_fd != INVALID_FD) {
        ::close(m_fd);
        m_fd = INVALID_FD;
    }
}

Result<File> File::open(PathView path, const OpenOptions& options) {
    if (options.truncate && !options.write) {
        return Err("Cannot truncate a file without write access");
    }
    if (options.append && options.truncate) {
        return Err("Cannot append and truncate a file at the same time");
    }

    int flags = 0;
    if (options.read && options.write) {
        flags |= O_RDWR;
    } else if (options.read) {
        flags |= O_RDONLY;
    } else if (options.write) {
        flags |= O_WRONLY;
    }

    // creation disposition
    if (options.create) {
        flags |= O_CREAT;
        if (options.createNew) {
            flags |= O_EXCL;
        }
    }
    if (options.truncate) {
        flags |= O_TRUNC;
    }
    if (options.append) {
        flags |= O_APPEND;
    }

    if (!options.followSymlink) {
        flags |= O_NOFOLLOW;
    }

    flags |= options.customFlags;

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // rw-r--r--, default

    int fd;
    if (flags & O_CREAT) {
        fd = ::open(path.string().c_str(), flags, mode);
    } else {
        fd = ::open(path.string().c_str(), flags);
    }

    if (fd == -1) {
        return Err(fmt::format("Failed to open file: {}", strerror(errno)));
    }

    return Ok(File(fd));
}

Result<> File::sync() {
    return wrapCall("fsync", fsync(m_fd));
}

static Result<> doLock(NativeFd fd, bool shared, bool tryLock) {
    return wrapCall("flock", flock(fd, (shared ? LOCK_SH : LOCK_EX) | (tryLock ? LOCK_NB : 0)));
}

Result<> File::lock() {
    return doLock(m_fd, false, false);
}

Result<> File::tryLock() {
    return doLock(m_fd, false, true);
}

Result<> File::lockShared() {
    return doLock(m_fd, true, false);
}

Result<> File::tryLockShared() {
    return doLock(m_fd, true, true);
}

Result<> File::unlock() {
    return wrapCall("flock", flock(m_fd, LOCK_UN));
}

Result<> File::setLength(uint64_t length) {
    return wrapCall("ftruncate", ftruncate(m_fd, length));
}

Result<Metadata> File::metadata() {
    struct stat st;
    GEODE_UNWRAP(wrapCall("fstat", fstat(m_fd, &st)));

    return Ok(metadataFromStat(st));
}

Result<File> File::clone() {
    int outFd;
    GEODE_UNWRAP(wrapCall("dup", outFd = dup(m_fd)));

    return Ok(File(outFd));
}

Result<size_t> File::read(std::span<uint8_t> buf) {
    ssize_t bytesRead = ::read(m_fd, buf.data(), buf.size());
    if (bytesRead < 0) {
        return Err(fmt::format("read failed: {}", strerror(errno)));
    }
    return Ok((size_t)bytesRead);
}

Result<size_t> File::write(std::span<const uint8_t> buf) {
    ssize_t bytesWritten = ::write(m_fd, buf.data(), buf.size());
    if (bytesWritten < 0) {
        return Err(fmt::format("write failed: {}", strerror(errno)));
    }
    return Ok((size_t)bytesWritten);
}

Result<> File::seek(uint64_t pos) {
    off_t result = lseek(m_fd, pos, SEEK_SET);
    if (result == (off_t)-1) {
        return Err(fmt::format("lseek failed: {}", strerror(errno)));
    }
    return Ok();
}

Result<uint64_t> File::tell() {
    off_t result = lseek(m_fd, 0, SEEK_CUR);
    if (result == (off_t)-1) {
        return Err(fmt::format("lseek failed: {}", strerror(errno)));
    }
    return Ok((uint64_t)result);
}

Result<uint64_t> File::size() {
    struct stat st;
    GEODE_UNWRAP(wrapCall("fstat", fstat(m_fd, &st)));

    return Ok((uint64_t)st.st_size);
}

}
