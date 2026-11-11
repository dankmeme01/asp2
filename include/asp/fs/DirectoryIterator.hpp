#pragma once
#include "Path.hpp"
#include <asp/iter/Iter.hpp>

namespace asp::fs {

struct DirectoryEntry {
    Path path;
    FileType type;

    bool isFile() const {
        return type == FileType::RegularFile;
    }

    bool isDir() const {
        return type == FileType::Directory;
    }

    bool isSymlink() const noexcept {
        return type == FileType::Symlink;
    }
};

struct DirectoryIteratorOptions {
    /// Uses the `FIND_FIRST_EX_LARGE_FETCH` flag on Windows, recommended for enumerating large directories.
    /// Does nothing on other platforms.
    bool m_largeBuffer = false;
};

struct DirectoryIterator : public iter::Iter<DirectoryIterator, Result<DirectoryEntry>> {
    ~DirectoryIterator();
    DirectoryIterator(const DirectoryIterator&) = delete;
    DirectoryIterator& operator=(const DirectoryIterator&) = delete;
    DirectoryIterator(DirectoryIterator&& other) noexcept;
    DirectoryIterator& operator=(DirectoryIterator&& other) noexcept;

    std::optional<Item> next();

    static Result<DirectoryIterator> create(PathView path, DirectoryIteratorOptions options = {});

private:
    DirectoryIterator(Path path) : m_path(std::move(path)) {}

    void* m_handle = nullptr;
    Path m_path;
    std::optional<DirectoryEntry> m_nextItem;
    DirectoryIteratorOptions m_options;
};

struct RecursiveDirectoryIterator : public iter::Iter<RecursiveDirectoryIterator, Result<DirectoryEntry>> {
    std::optional<Item> next();

    static Result<RecursiveDirectoryIterator> create(PathView path, DirectoryIteratorOptions options = {});

private:
    RecursiveDirectoryIterator(Path path, DirectoryIterator first, DirectoryIteratorOptions options) : m_path(std::move(path)), m_options(options) {
        m_stack.push_back(std::move(first));
    }

    Path m_path;
    std::vector<DirectoryIterator> m_stack;
    DirectoryIteratorOptions m_options;
};

}
