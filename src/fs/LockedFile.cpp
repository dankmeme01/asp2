#if 0

#include <asp/fs/LockedFile.hpp>

// Note: implementations for some functions are found in platform-dependent source files.

namespace asp::fs {

LockedFile::LockedFile() : m_fd(InvalidFd) {}

LockedFile::~LockedFile() {
    if (m_fd != InvalidFd) {
        auto _ = this->close();
    }
}

LockedFile::LockedFile(LockedFile&& other) {
    this->m_fd = other.m_fd;
    other.m_fd = InvalidFd;
}

LockedFile& LockedFile::operator=(LockedFile&& other) {
    if (this != &other) {
        this->m_fd = other.m_fd;
        other.m_fd = InvalidFd;
    }

    return *this;
}

bool LockedFile::isOpen() const {
    return m_fd != InvalidFd;
}

} // namespace asp::fs

#endif
