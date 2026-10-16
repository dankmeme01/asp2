#include <asp/fs/DirectoryIterator.hpp>
#include "helpers.hpp"

namespace asp::fs {

DirectoryIterator::~DirectoryIterator() {
    if (m_handle) {
        FindClose(m_handle);
    }
}

DirectoryIterator::DirectoryIterator(DirectoryIterator&& other) noexcept {
    *this = std::move(other);
}

DirectoryIterator& DirectoryIterator::operator=(DirectoryIterator&& other) noexcept {
    if (this != &other) {
        if (m_handle) {
            FindClose(m_handle);
        }

        m_handle = other.m_handle;
        m_path = std::move(other.m_path);
        m_nextItem = std::move(other.m_nextItem);

        other.m_handle = nullptr;
        other.m_nextItem = std::nullopt;
    }

    return *this;
}

Result<DirectoryIterator> DirectoryIterator::create(PathView path, bool recursive) {
    GEODE_UNWRAP_INTO(auto native, path.native());
    if (native.back() != L'\\') {
        native += L'\\';
    }
    native += L'*';

    WIN32_FIND_DATAW findData;
    auto h = FindFirstFileExW(
        native.c_str(),
        FindExInfoBasic,
        &findData,
        FindExSearchNameMatch,
        nullptr,
        0
    );

    if (h == INVALID_HANDLE_VALUE) {
        return Err(formatError());
    }

    DirectoryIterator iter{path.toPath()};
    iter.m_handle = h;

    std::wstring_view name(findData.cFileName);
    if (name != L"." && name != L"..") {
        GEODE_UNWRAP_INTO(auto asPath, Path::fromWide(name));
        iter.m_nextItem = path / asPath;
    }

    return Ok(std::move(iter));
}

std::optional<Result<Path>> DirectoryIterator::next() noexcept {
    while (!m_nextItem) {
        WIN32_FIND_DATAW findData;
        if (!FindNextFileW(m_handle, &findData)) {
            auto e = GetLastError();
            if (e == ERROR_NO_MORE_FILES) {
                return std::nullopt;
            } else {
                return Err(formatError(e));
            }
        }

        std::wstring_view name(findData.cFileName);
        if (name == L"." || name == L"..") {
            continue;
        }

        GEODE_UNWRAP_INTO(auto pname, Path::fromWide(name));
        m_nextItem = m_path / pname;
    }

    auto nextItem = std::move(*m_nextItem);
    m_nextItem = std::nullopt;

    return Ok(std::move(nextItem));
}

}
