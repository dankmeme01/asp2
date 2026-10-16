#include <asp/fs.hpp>
#include <asp/iter.hpp>
#include "helpers.hpp"

namespace asp::fs {

File::~File() {
    if (m_fd != INVALID_FD) {
        CloseHandle(m_fd);
    }
}

File::File(File&& other) noexcept {
    m_fd = other.m_fd;
    other.m_fd = INVALID_FD;
}

File& File::operator=(File&& other) noexcept {
    if (this != &other) {
        if (m_fd != INVALID_FD) {
            CloseHandle(m_fd);
        }
        m_fd = other.m_fd;
        other.m_fd = INVALID_FD;
    }
    return *this;
}

Result<File> File::open(PathView path, const OpenOptions& options) {
    GEODE_UNWRAP_INTO(auto native, _impl::native(path));

    if (options.truncate && !options.write) {
        return Err("Cannot truncate a file without write access");
    }
    if (options.append && options.truncate) {
        return Err("Cannot append and truncate a file at the same time");
    }

    DWORD access = options.accessMode;
    if (options.read) {
        access |= GENERIC_READ;
    }
    if (options.append) {
        // use generic write but without FILE_WRITE_DATA
        access |= (FILE_GENERIC_WRITE & ~static_cast<DWORD>(FILE_WRITE_DATA));
    } else if (options.write) {
        access |= GENERIC_WRITE;
    }

    DWORD cdisp;
    if (options.create) {
        if (options.createNew) {
            cdisp = CREATE_NEW;
        } else {
            cdisp = options.truncate ? CREATE_ALWAYS : OPEN_ALWAYS;
        }
    } else {
        // cannot create, only allow opening existing files
        cdisp = options.truncate ? TRUNCATE_EXISTING : OPEN_EXISTING;
    }

    DWORD flags = FILE_ATTRIBUTE_NORMAL | options.customFlags;
    if (!options.followSymlink) {
        flags |= FILE_FLAG_OPEN_REPARSE_POINT;
    }

    auto h = CreateFileW(
        native.c_str(),
        access,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        cdisp,
        flags,
        nullptr
    );

    if (h == INVALID_HANDLE_VALUE) {
        return Err(fmt::format("Failed to open file: {}", formatError()));
    }

    return Ok(File(h));
}

Result<> File::sync() {
    return wrapCall(FlushFileBuffers(m_fd));
}

static Result<> doLock(NativeFd fd, bool shared, bool tryLock) {
    OVERLAPPED overlapped{};
    DWORD flags = shared ? 0 : LOCKFILE_EXCLUSIVE_LOCK;
    if (tryLock) {
        flags |= LOCKFILE_FAIL_IMMEDIATELY;
    }
    return wrapCall(LockFileEx(fd, flags, 0, MAXDWORD, MAXDWORD, &overlapped));
}

Result<> File::lock() {
    return doLock(m_fd, false, false);
}

Result<> File::tryLock() {
    return doLock(m_fd, false, true);
}

Result<> File::lockShared() {
    return doLock(m_fd, true, false);
}

Result<> File::tryLockShared() {
    return doLock(m_fd, true, true);
}

Result<> File::unlock() {
    OVERLAPPED overlapped{};
    return wrapCall(UnlockFileEx(m_fd, 0, MAXDWORD, MAXDWORD, &overlapped));
}

Result<> File::setLength(uint64_t length) {
    FILE_END_OF_FILE_INFO info{};
    info.EndOfFile.QuadPart = length;

    return wrapCall(SetFileInformationByHandle(m_fd, FileEndOfFileInfo, &info, sizeof(info)));
}

Result<Metadata> File::metadata() {
    BY_HANDLE_FILE_INFORMATION data;
    GEODE_UNWRAP(wrapCall(GetFileInformationByHandle(m_fd, &data)));

    Metadata meta;
    meta.size = (static_cast<uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;

    if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        meta.type = FileType::Symlink;
    } else if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        meta.type = FileType::Directory;
    } else {
        meta.type = FileType::RegularFile;
    }

    meta.permissions = Permissions::All;
    if (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
        meta.permissions &= ~(Permissions::OwnerWrite | Permissions::GroupWrite | Permissions::OtherWrite);
    }

    meta.readOnly = (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
    meta.hidden = (data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
    meta.accessedAt = toSystemTime(data.ftLastAccessTime);
    meta.modifiedAt = toSystemTime(data.ftLastWriteTime);
    meta.createdAt = toSystemTime(data.ftCreationTime);

    return Ok(meta);
}

Result<File> File::clone() {
    HANDLE out;
    GEODE_UNWRAP(wrapCall(DuplicateHandle(
        GetCurrentProcess(),
        m_fd,
        GetCurrentProcess(),
        &out,
        0,
        FALSE,
        DUPLICATE_SAME_ACCESS
    )));

    return Ok(File(out));
}

Result<size_t> File::read(std::span<uint8_t> buf) {
    DWORD bytesRead;
    if (!ReadFile(m_fd, buf.data(), (DWORD)buf.size(), &bytesRead, nullptr)) {
        return Err(formatError());
    }

    return Ok((size_t)bytesRead);
}

Result<size_t> File::write(std::span<const uint8_t> buf) {
    DWORD bytesWritten;
    if (!WriteFile(m_fd, buf.data(), (DWORD)buf.size(), &bytesWritten, nullptr)) {
        return Err(formatError());
    }

    return Ok((size_t)bytesWritten);
}

Result<> File::seek(uint64_t pos) {
    LARGE_INTEGER li;
    li.QuadPart = pos;
    return wrapCall(SetFilePointerEx(m_fd, li, nullptr, FILE_BEGIN));
}

Result<uint64_t> File::tell() {
    LARGE_INTEGER li;
    li.QuadPart = 0;
    GEODE_UNWRAP(wrapCall(SetFilePointerEx(m_fd, li, &li, FILE_CURRENT)));
    return Ok((uint64_t)li.QuadPart);
}

Result<uint64_t> File::size() {
    LARGE_INTEGER li;
    li.QuadPart = 0;
    GEODE_UNWRAP(wrapCall(GetFileSizeEx(m_fd, &li)));
    return Ok((uint64_t)li.QuadPart);
}

}
