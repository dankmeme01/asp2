#pragma once
#include "Permissions.hpp"
#include <stdint.h>
#include <asp/time/SystemTime.hpp>

namespace asp::fs {

enum class FileType {
    RegularFile,
    Directory,
    Symlink,
    BlockDevice,
    CharDevice,
    Fifo,
    Socket,
    Other,
};

/// File/directory metadata accessible on all platforms
struct MetadataBase {
    FileType type;
    Permissions permissions;
    uint64_t size;

    std::optional<asp::SystemTime> modifiedAt;
    std::optional<asp::SystemTime> accessedAt;
    std::optional<asp::SystemTime> createdAt;

    /// Checks whether this metadata represents a regular file. This will return `false` even if it's a symlink pointing to a file.
    bool isFile() const noexcept {
        return type == FileType::RegularFile;
    }

    /// Checks whether this metadata represents a directory. This will return `false` even if it's a symlink pointing to a directory.
    bool isDir() const noexcept {
        return type == FileType::Directory;
    }

    /// Checks whether this metadata represents a symlink.
    bool isSymlink() const noexcept {
        return type == FileType::Symlink;
    }
};

#if defined __linux__ || defined __APPLE__
/// Unix-specific metadata
struct MetadataUnix : MetadataBase {};
#endif

#ifdef __linux__
/// Linux-specific metadata
struct MetadataLinux : MetadataUnix {};
#endif

#ifdef _WIN32

/// Windows-specific metadata
struct MetadataWindows : MetadataBase {
    bool readOnly;
    bool hidden;
};
using Metadata = MetadataWindows;

#elif defined (__APPLE__)

/// Apple-specific metadata
struct MetadataApple : MetadataUnix {};
using Metadata = MetadataApple;

#elif defined (__ANDROID__)

/// Android-specific metadata
struct MetadataAndroid : MetadataLinux {};
using Metadata = MetadataAndroid;

#elif defined (__linux__)

using Metadata = MetadataLinux;

#else

using Metadata = MetadataBase;

#endif

}