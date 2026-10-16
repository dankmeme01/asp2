#pragma once
#include "Path.hpp"
#include <asp/iter/Iter.hpp>

namespace asp::fs {

struct DirectoryIterator : public iter::Iter<DirectoryIterator, Result<Path>> {
    ~DirectoryIterator();
    DirectoryIterator(const DirectoryIterator&) = delete;
    DirectoryIterator& operator=(const DirectoryIterator&) = delete;
    DirectoryIterator(DirectoryIterator&& other) noexcept;
    DirectoryIterator& operator=(DirectoryIterator&& other) noexcept;

    std::optional<Item> next() noexcept;

    // TODO: recursive not implemented yet
    static Result<DirectoryIterator> create(PathView path, bool recursive);

private:
    DirectoryIterator(Path path) : m_path(std::move(path)) {}

    void* m_handle = nullptr;
    Path m_path;
    std::optional<Path> m_nextItem;
};

}
