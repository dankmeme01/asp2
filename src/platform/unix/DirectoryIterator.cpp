#include <asp/fs/DirectoryIterator.hpp>
#include <dirent.h>
#include "helpers.hpp"

namespace asp::fs {

DirectoryIterator::~DirectoryIterator() {
    if (m_handle) {
        closedir((DIR*)m_handle);
    }
}

DirectoryIterator::DirectoryIterator(DirectoryIterator&& other) noexcept {
    *this = std::move(other);
}

DirectoryIterator& DirectoryIterator::operator=(DirectoryIterator&& other) noexcept {
    if (this != &other) {
        if (m_handle) {
            closedir((DIR*)m_handle);
        }

        m_handle = other.m_handle;
        m_path = std::move(other.m_path);
        m_nextItem = std::move(other.m_nextItem);

        other.m_handle = nullptr;
        other.m_nextItem = std::nullopt;
    }

    return *this;
}

Result<DirectoryIterator> DirectoryIterator::create(PathView path) {
    auto h = opendir(path.string().c_str());

    if (h == nullptr) {
        return Err(fmt::format("Unable to open directory '{}': {}", path, strerror(errno)));
    }

    DirectoryIterator iter{ path.toPath() };
    iter.m_handle = h;

    return Ok(std::move(iter));
}

static FileType convertFileType(unsigned char d_type) {
    switch (d_type) {
        case DT_REG: return FileType::RegularFile;
        case DT_DIR: return FileType::Directory;
        case DT_LNK: return FileType::Symlink;
        case DT_BLK: return FileType::BlockDevice;
        case DT_CHR: return FileType::CharDevice;
        case DT_FIFO: return FileType::Fifo;
        case DT_SOCK: return FileType::Socket;
        default: return FileType::Other;
    }
}

static DirectoryEntry makeEntry(struct dirent* ent, PathView rootPath, std::string_view name) {
    return DirectoryEntry {
        rootPath / name,
        convertFileType(ent->d_type)
    };
}

std::optional<Result<DirectoryEntry>> DirectoryIterator::next() {
    while (!m_nextItem) {
        auto entry = readdir((DIR*)m_handle);
        if (!entry) {
            return std::nullopt;
        }

        std::string_view name{entry->d_name};
        if (name == "." || name == "..") continue;

        m_nextItem = makeEntry(entry, m_path, name);
    }

    auto nextItem = std::move(*m_nextItem);
    m_nextItem = std::nullopt;

    return Ok(std::move(nextItem));
}

}
