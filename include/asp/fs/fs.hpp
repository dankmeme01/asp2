#pragma once

#include "Error.hpp"
#include <filesystem>

namespace asp::fs {
    using path = std::filesystem::path;

    // class Path : public std::filesystem::path {
    //     using Base = std::filesystem::path;

    // public:
    //     // using Base::Base;
    //     // using Base::operator=;
    //     // using Base::operator/=;
    //     // using Base::operator+=;
    //     // using Base::assign;
    //     // using Base::append;
    //     // using Base::clear;
    //     // using Base::concat;

    // };

    Result<void> copy(const path& origin, const path& dest);
    Result<void> copy(const path& origin, const path& dest, std::filesystem::copy_options opts);
    Result<void> rename(const path& oldPath, const path& newPath);

    bool exists(const path& path);
    Result<void> createDir(const path& path);
    Result<void> createDirAll(const path& path);

    Result<bool> isFile(const path& path);
    Result<bool> isDirectory(const path& path);
    Result<bool> equivalent(const path& path1, const path& path2);

    Result<std::filesystem::file_time_type> lastWriteTime(const path& path);

    // Removes a file or an empty directory by the given path.
    Result<void> remove(const path& path);
    // Removes a file by the given path, fails if it is not a file.
    Result<void> removeFile(const path& path);
    // Removes a directory by the given path, fails if it is not a directory.
    Result<void> removeDir(const path& path);
    // Removes a directory by the given path, recursively. Fails if it is not a directory
    Result<uintmax_t> removeDirAll(const path& path);
    // Removes a directory recursively, or a file.
    Result<uintmax_t> removeAll(const path& path);

    Result<std::filesystem::directory_iterator> iterdir(const path& path);

    Result<std::vector<uint8_t>> read(const path& path);
    Result<std::string> readToString(const path& path);

    Result<void> write(const path& path, const std::vector<uint8_t>& data);
    Result<void> write(const path& path, const std::string& data);
    Result<void> write(const path& path, const char*, size_t size);
    Result<void> write(const path& path, const unsigned char*, size_t size);
}
