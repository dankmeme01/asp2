#include <asp/fs.hpp>
#include <asp/iter.hpp>
#include "helpers.hpp"
#include <userenv.h>
#include <array>

namespace asp::fs {

Result<NativePath> _impl::native(std::string_view path) {
    return utf8ToWide(path);
}

bool _impl::isAbsolute(std::string_view path) noexcept {
    if (path.size() >= 3 && std::isalpha((uint8_t)path[0]) && path[1] == ':' && (path[2] == '/' || path[2] == '\\')) {
        return true;
    } else if (path.size() >= 2 && path[0] == '\\' && path[1] == '\\') {
        return true;
    }
    return false;
}

std::optional<std::string_view> _impl::rootName(std::string_view path) noexcept {
    if (path.size() >= 2 && std::isalpha((uint8_t)path[0]) && path[1] == ':') {
        // path like C:/...
        if (path.size() >= 3 && (path[2] == '/' || path[2] == '\\')) {
            return path.substr(0, 3);
        }
        // path like C:stuff/
        return path.substr(0, 2);
    }
    // relative?
    return std::nullopt;
}

static void unwindowsizePath(std::wstring& str) {
    if (str.starts_with(L"\\\\?\\UNC\\")) {
        str.replace(0, 8, L"\\\\");
    } else if (str.starts_with(L"\\\\?\\")) {
        str.erase(0, 4);
    }
}

Result<Path> _impl::canonicalize(std::string_view path) {
    // see https://github.com/rust-lang/rust/blob/e457a7b0d326d67b4322ef0d11bd715cfaeda48f/library/std/src/sys/fs/windows.rs#L1602
    OpenOptions opts{};
    opts.read = false;
    opts.customFlags = FILE_FLAG_BACKUP_SEMANTICS;
    auto result = File::open(path, opts);

    if (!result) {
        return Err(fmt::format("Unable to open file: {}", result.unwrapErr()));
    }

    auto file = std::move(result).unwrap();

    auto len = GetFinalPathNameByHandleW(file.fd(), nullptr, 0, VOLUME_NAME_DOS);
    if (len == 0) {
        return Err(fmt::format("Unable to get final path name: {}", formatError()));
    }

    std::wstring buf(len, L'\0');
    len = GetFinalPathNameByHandleW(file.fd(), buf.data(), (DWORD)buf.size(), VOLUME_NAME_DOS);
    buf.resize(len);

    unwindowsizePath(buf);
    return Path::fromWide(buf);
}

bool _impl::equivalent(std::string_view lhs, std::string_view rhs, bool followSymlink) noexcept {
    auto doOpen = [&](auto path) {
        OpenOptions opts{};
        opts.read = false;
        opts.followSymlink = followSymlink;
        opts.customFlags = FILE_FLAG_BACKUP_SEMANTICS;
        return File::open(path, opts);
    };

    auto r1 = doOpen(lhs);
    if (!r1) return false;
    auto r2 = doOpen(rhs);
    if (!r2) return false;

    auto file1 = std::move(r1).unwrap();
    auto file2 = std::move(r2).unwrap();

    BY_HANDLE_FILE_INFORMATION info1, info2;
    if (!GetFileInformationByHandle(file1.fd(), &info1) || !GetFileInformationByHandle(file2.fd(), &info2)) {
        return false;
    }

    return info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber
        && info1.nFileIndexHigh == info2.nFileIndexHigh
        && info1.nFileIndexLow == info2.nFileIndexLow;
}

Result<Path> _impl::absolute(std::string_view path) {
    auto p = path.empty() ? "." : path;
    GEODE_UNWRAP_INTO(auto wide, utf8ToWide(p));

    auto len = GetFullPathNameW(wide.c_str(), 0, nullptr, nullptr);
    if (len == 0) {
        return Err(fmt::format("Unable to get full path name: {}", formatError()));
    }

    std::wstring buf(len, L'\0');
    len = GetFullPathNameW(wide.c_str(), (DWORD)buf.size(), buf.data(), nullptr);
    if (len == 0) {
        return Err(fmt::format("Unable to get full path name: {}", formatError()));
    }
    buf.resize(len);

    GEODE_UNWRAP_INTO(auto utf8, wideToUtf8({buf.data(), len}));
    return Ok(Path(std::move(utf8)));
}

bool _impl::exists(std::string_view path) noexcept {
    // see https://github.com/rust-lang/rust/blob/e457a7b0d326d67b4322ef0d11bd715cfaeda48f/library/std/src/sys/fs/windows.rs#L1738
    auto res = _impl::native(path);
    if (!res) return false;

    auto h = CreateFileW(
        res.unwrap().c_str(),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );

    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        return true;
    }

    auto e = GetLastError();

    return e == ERROR_SHARING_VIOLATION || e == ERROR_CANT_ACCESS_FILE;
}

Result<Metadata> _impl::metadata(std::string_view path, bool followSymlink) {
    OpenOptions opts{};
    opts.followSymlink = followSymlink;
    opts.customFlags = FILE_FLAG_BACKUP_SEMANTICS;
    return File::open(path, opts).andThen([&](File file) {
        return file.metadata();
    });
}

Path& Path::makePreferred() noexcept {
    for (auto& c : m_path) {
        if (c == '/' || c == '\\') {
            c = PreferredSep;
        }
    }
    return *this;
}

Result<Path> Path::fromWide(std::wstring_view path) {
    GEODE_UNWRAP_INTO(auto utf8, wideToUtf8(path));
    return Ok(Path(std::move(utf8)));
}

Result<Path> Path::cwd() {
    std::array<wchar_t, 1024> buf;
    auto n = GetCurrentDirectoryW(buf.size(), buf.data());
    if (n == 0 || n > buf.size()) {
        return Err(formatError());
    }

    GEODE_UNWRAP_INTO(auto utf8, wideToUtf8({buf.data(), n}));
    return Ok(Path(std::move(utf8)));
}

Result<Path> Path::home() {
    std::string buf;

    size_t cnt = 0;
    getenv_s(&cnt, nullptr, 0, "USERPROFILE");
    if (cnt > 1) { // cnt includes null terminator, we want non-empty strings
        buf.resize(cnt);
        getenv_s(&cnt, buf.data(), cnt, "USERPROFILE");
        // strip away null terminator
        buf.resize(strlen(buf.c_str()));
        return Ok(Path(std::move(buf)));
    }

    auto t = GetCurrentProcessToken();

    DWORD cnt2 = 0;
    GetUserProfileDirectoryW(t, nullptr, &cnt2);
    if (cnt2 > 1) {
        std::wstring buf2(cnt2, L'\0');
        if (!GetUserProfileDirectoryW(t, buf2.data(), &cnt2)) {
            return Err(formatError());
        }
        GEODE_UNWRAP_INTO(auto utf8, wideToUtf8({buf2.data(), cnt2}));
        return Ok(Path(std::move(utf8)));
    }

    // should never happen?
    return Err("Unable to determine home directory");
}

Result<Path> Path::tmp() {
    std::array<wchar_t, 1024> buf;
    auto n = GetTempPathW(buf.size(), buf.data());
    if (n == 0 || n > buf.size()) {
        return Err(formatError());
    }
    GEODE_UNWRAP_INTO(auto utf8, wideToUtf8({buf.data(), n}));
    return Ok(Path(std::move(utf8)));
}


// -- file op (asp::fs) implementations -- //

Result<void> copy(PathView origin, PathView dest) {
    GEODE_UNWRAP_INTO(auto nativeOrigin, _impl::native(origin));
    GEODE_UNWRAP_INTO(auto nativeDest, _impl::native(dest));

    if (!CopyFileExW(nativeOrigin.c_str(), nativeDest.c_str(), nullptr, nullptr, nullptr, 0)) {
        return Err(formatError());
    }

    return Ok();
}

Result<void> rename(PathView oldPath, PathView newPath) {
    GEODE_UNWRAP_INTO(auto nativeOld, _impl::native(oldPath));
    GEODE_UNWRAP_INTO(auto nativeNew, _impl::native(newPath));

    if (!MoveFileExW(nativeOld.c_str(), nativeNew.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        return Err(formatError());
    }

    return Ok();
}

Result<void> createDir(PathView path) {
    GEODE_UNWRAP_INTO(auto native, _impl::native(path));

    if (!CreateDirectoryW(native.c_str(), nullptr)) {
        return Err(formatError());
    }

    return Ok();
}

Result<void> removeFile(PathView path) {
    OpenOptions opts{};
    opts.read = false;
    opts.followSymlink = false;
    GEODE_UNWRAP_INTO(auto file, File::open(path, opts));

    BY_HANDLE_FILE_INFORMATION data;
    GEODE_UNWRAP(wrapCall(GetFileInformationByHandle(file.fd(), &data)));

    bool reparsePoint = (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    bool directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

    if (reparsePoint && directory) {
        // symlink to directory, use removeDir
        return removeDir(path);
    }

    GEODE_UNWRAP_INTO(auto native, _impl::native(path));

    if (!DeleteFileW(native.c_str())) {
        return Err(formatError());
    }

    return Ok();
}

Result<void> removeDir(PathView path) {
    GEODE_UNWRAP_INTO(auto native, _impl::native(path));

    if (!RemoveDirectoryW(native.c_str())) {
        return Err(formatError());
    }

    return Ok();
}

static Result<> findAndRemoveAllInside(PathView path, std::vector<Path>& out) {
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

    do {
        std::wstring_view name(findData.cFileName);
        if (name == L"." || name == L"..") {
            continue;
        }

        // directory = has attribute directory but NOT a reparse point
        bool isReparsePoint = (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
        bool isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        GEODE_UNWRAP_INTO(auto childPath, Path::fromWide(name).map([&](Path child) {
            return path / child;
        }));

        if (!isDirectory) {
            // this is a regular file or symlink to a file, delete it
            GEODE_UNWRAP(removeFile(childPath));
        } else {
            // directory, queue it if not a reparse point
            if (isReparsePoint) {
                GEODE_UNWRAP(removeDir(childPath));
            } else {
                out.push_back(std::move(childPath));
            }
        }

    } while (FindNextFileW(h, &findData));

    FindClose(h);
    return Ok();
}

Result<void> removeDirAll(PathView path) {
    if (!path.isDir()) {
        return Err(fmt::format("Path '{}' is not a directory", path));
    }

    OpenOptions opts{};
    opts.accessMode = FILE_LIST_DIRECTORY;
    opts.customFlags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT;

    GEODE_UNWRAP_INTO(auto file, File::open(path, opts));

    FILE_BASIC_INFO info{};
    if (!GetFileInformationByHandleEx(file.fd(), FileBasicInfo, &info, sizeof(info))) {
        return Err(formatError());
    }

    if ((info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        return Err("Not a directory");
    }

    std::vector<Path> toDelete;
    toDelete.push_back(path.toPath());

    size_t i = 0;
    while (i < toDelete.size()) {
        // copy dir to avoid lifetime issues due to vector mutation
        auto dir = toDelete[i];
        GEODE_UNWRAP(findAndRemoveAllInside(dir, toDelete));
        i++;
    }

    // now we only have a bunch of empty directories left to delete
    for (auto& dir : asp::iter::fromReverse(toDelete)) {
        GEODE_UNWRAP(removeDir(dir));
    }

    return Ok();
}

static Result<> doCreateSymlink(PathView target, PathView link, bool isDir) {
    GEODE_UNWRAP_INTO(auto nativeTarget, _impl::native(target));
    GEODE_UNWRAP_INTO(auto nativeLink, _impl::native(link));

    DWORD flags = 0;
    if (isDir) {
        flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    }

    return wrapCall(CreateSymbolicLinkW(nativeLink.c_str(), nativeTarget.c_str(), flags));
}

Result<void> createSymlink(PathView target, PathView link) {
    return doCreateSymlink(target, link, false);
}

Result<void> createDirSymlink(PathView target, PathView link) {
    return doCreateSymlink(target, link, true);
}

Result<Path> readSymlink(PathView path) {
    OpenOptions opts{};
    opts.read = false;
    opts.followSymlink = false;
    opts.customFlags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT;
    GEODE_UNWRAP_INTO(auto file, File::open(path, opts));

    DWORD bufSize = GetFinalPathNameByHandleW(file.fd(), nullptr, 0, FILE_NAME_NORMALIZED);
    if (!bufSize) {
        return Err(formatError());
    }

    std::wstring buf(bufSize, L'\0');
    bufSize = GetFinalPathNameByHandleW(file.fd(), buf.data(), buf.size(), FILE_NAME_NORMALIZED);
    if (!bufSize) {
        return Err(formatError());
    }

    buf.resize(bufSize);

    unwindowsizePath(buf);
    return Path::fromWide(buf);
}

}
