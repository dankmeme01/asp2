#pragma once
#include <stdint.h>

namespace asp::fs {

struct OpenOptionsBase {
    // make it non-aggregate
    OpenOptionsBase() {};

    bool read = true;
    bool write = false;
    bool append = false;
    /// Truncates the file if it already existed
    bool truncate = false;
    /// Creates a new file
    bool create = false;
    /// Creates a new file, failing if the file already exists. This is only relevant if `create` is also true.
    bool createNew = false;
    /// Whether to follow the final symlink or open the symlink file otherwise
    bool followSymlink = true;
};

#if defined __linux__ || defined __APPLE__
/// Unix-specific OpenOptions
struct OpenOptionsUnix : OpenOptionsBase {
    int customFlags = 0;
};
#endif

#ifdef __linux__
/// Linux-specific OpenOptions
struct OpenOptionsLinux : OpenOptionsUnix {};
#endif

#ifdef _WIN32

/// Windows-specific OpenOptions
struct OpenOptionsWindows : OpenOptionsBase {
    uint32_t accessMode = 0;
    uint32_t customFlags = 0;
};
using OpenOptions = OpenOptionsWindows;

#elif defined (__APPLE__)

/// Apple-specific OpenOptions
struct OpenOptionsApple : OpenOptionsUnix {};
using OpenOptions = OpenOptionsApple;

#elif defined (__ANDROID__)

/// Android-specific OpenOptions
struct OpenOptionsAndroid : OpenOptionsLinux {};
using OpenOptions = OpenOptionsAndroid;

#elif defined (__linux__)

using OpenOptions = OpenOptionsLinux;

#else

using OpenOptions = OpenOptionsBase;

#endif

}
