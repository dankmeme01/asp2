#pragma once

// TODO: implement this :)

#if 0

#include "fs.hpp"
#include <asp/time/Duration.hpp>

namespace asp::fs {
    struct LockedFileOptions {
        // Whether to create a file if it does not already exist
        bool create = false;
        // Whether to use a shared lock instead of exclusive, allowing multiple concurrent readers
        bool shared = false;
    };

    class LockedFile {
    public:
        LockedFile();
        ~LockedFile();

        LockedFile(const LockedFile&) = delete;
        LockedFile& operator=(const LockedFile&) = delete;

        LockedFile(LockedFile&&);
        LockedFile& operator=(LockedFile&&);

        bool isOpen() const;
        Result<> open(const path& p, const LockedFileOptions& options);
        Result<> openTimeout(const path& p, const LockedFileOptions& options, const time::Duration& timeout);
        Result<> close();

    private:
        FdType m_fd;
    };
}

#endif
