#pragma once

#include <asp/detail/config.hpp>
#include "path.hpp"
#include <vector>
#include <string>
#include <stdint.h>

namespace asp::fs {
    Result<void> copy(PathView origin, PathView dest);
    // TODO
    // Result<void> copy(PathView origin, PathView dest, std::filesystem::copy_options opts);
    Result<void> rename(PathView oldPath, PathView newPath);

    Result<void> createDir(PathView path);
    Result<void> createDirAll(PathView path);

    // TODO
    // Result<std::filesystem::file_time_type> lastWriteTime(PathView path);
    Result<uint64_t> fileSize(PathView path);

    // Removes a file or an empty directory by the given path.
    Result<void> remove(PathView path);
    // Removes a file by the given path, fails if it is not a file.
    Result<void> removeFile(PathView path);
    // Removes a directory by the given path, fails if it is not a directory.
    Result<void> removeDir(PathView path);
    // Removes a directory by the given path, recursively. Fails if it is not a directory
    Result<uintmax_t> removeDirAll(PathView path);
    // Removes a directory recursively, or a file.
    Result<uintmax_t> removeAll(PathView path);

    // TODO
    // Result<std::filesystem::directory_iterator> iterdir(PathView path);

    geode::Result<std::vector<uint8_t>> read(PathView path);
    geode::Result<std::string> readToString(PathView path);

    geode::Result<void> write(PathView path, const std::vector<uint8_t>& data);
    geode::Result<void> write(PathView path, const std::string& data);
    geode::Result<void> write(PathView path, const char*, size_t size);
    geode::Result<void> write(PathView path, const unsigned char*, size_t size);
}
