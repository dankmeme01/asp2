#include <asp/fs.hpp>
#include <fmt/format.h>
#include <Windows.h>
#include <userenv.h>
#include <array>

namespace asp::fs {

static std::string formatError(DWORD error = GetLastError()) {
    LPSTR buffer = nullptr;
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        (LPSTR)&buffer,
        0,
        nullptr
    );

    if (size == 0 || !buffer) {
        return fmt::format("Win error {}", error);
    }

    std::string message(buffer, size);
    LocalFree(buffer);

    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }

    return message;
}

static Result<std::string> wideToUtf8(std::wstring_view wide) {
    if (wide.empty()) return Ok(std::string{});

    int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), nullptr, 0, nullptr, nullptr);
    if (size == 0) {
        return Err(fmt::format("Unable to convert string: {}", formatError()));
    }

    std::string utf8(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), utf8.data(), size, nullptr, nullptr);

    return Ok(std::move(utf8));
}

static Result<std::wstring> utf8ToWide(std::string_view utf8) {
    if (utf8.empty()) return Ok(std::wstring{});

    int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), (int)utf8.size(), nullptr, 0);
    if (size == 0) {
        return Err(fmt::format("Unable to convert string: {}", formatError()));
    }

    std::wstring wide(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), wide.data(), size);

    return Ok(std::move(wide));
}

Result<Path> _impl::fromStdPath(const std::filesystem::path& path) {
    return Path::fromWide(path.native());
}

Result<NativePath> _impl::native(std::string_view path) {
    return utf8ToWide(path);
}

bool _impl::isAbsolute(std::string_view path) noexcept {
    if (path.size() >= 3 && std::isalpha(path[0]) && path[1] == ':' && (path[2] == '/' || path[2] == '\\')) {
        return true;
    } else if (path.size() >= 2 && path[0] == '\\' && path[1] == '\\') {
        return true;
    }
    return false;
}

std::optional<std::string_view> _impl::rootName(std::string_view path) noexcept {
    if (path.size() >= 2 && std::isalpha(path[0]) && path[1] == ':') {
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

Result<Path> _impl::canonicalize(std::string_view path) {
    GEODE_UNWRAP_INTO(auto native, _impl::native(path));

    // see https://github.com/rust-lang/rust/blob/e457a7b0d326d67b4322ef0d11bd715cfaeda48f/library/std/src/sys/fs/windows.rs#L1602
    auto h = CreateFileW(
        native.c_str(),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );

    if (h == INVALID_HANDLE_VALUE) {
        return Err(fmt::format("Unable to open file: {}", formatError()));
    }

    auto len = GetFinalPathNameByHandleW(h, nullptr, 0, VOLUME_NAME_DOS);
    if (len == 0) {
        CloseHandle(h);
        return Err(fmt::format("Unable to get final path name: {}", formatError()));
    }

    std::wstring buf(len, L'\0');
    len = GetFinalPathNameByHandleW(h, buf.data(), (DWORD)buf.size(), VOLUME_NAME_DOS);
    CloseHandle(h);

    GEODE_UNWRAP_INTO(auto utf8, wideToUtf8({buf.data(), len}));
    return Ok(Path(std::move(utf8)));
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

static asp::SystemTime toSystemTime(FILETIME ft) {
    constexpr uint64_t EPOCH_DIFF = 116444736000000000ULL;

    auto ft64 = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    if (ft64 < EPOCH_DIFF) return asp::SystemTime::UNIX_EPOCH;

    auto ms = (ft64 - EPOCH_DIFF) / 10'000ULL;
    return asp::SystemTime::fromUnixMillis(ms);
}

Result<Metadata> _impl::metadata(std::string_view path, bool followSymlink) {
    GEODE_UNWRAP_INTO(auto native, _impl::native(path));

    auto h = CreateFileW(
        native.c_str(),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | (followSymlink ? 0 : FILE_FLAG_OPEN_REPARSE_POINT),
        nullptr
    );
    if (h == INVALID_HANDLE_VALUE) {
        return Err(fmt::format("Unable to open file: {}", formatError()));
    }

    BY_HANDLE_FILE_INFORMATION data;
    auto res = GetFileInformationByHandle(h, &data);
    CloseHandle(h);

    if (!res) {
        return Err(fmt::format("Unable to get file information: {}", formatError()));
    }

    Metadata meta;
    meta.size = (static_cast<uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;

    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        meta.type = FileType::Directory;
    } else if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        meta.type = FileType::Symlink;
    } else {
        meta.type = FileType::RegularFile;
    }

    meta.readOnly = (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
    meta.hidden = (data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
    meta.accessedAt = toSystemTime(data.ftLastAccessTime);
    meta.modifiedAt = toSystemTime(data.ftLastWriteTime);
    meta.createdAt = toSystemTime(data.ftCreationTime);

    // TODO: permissions

    return Ok(meta);
}

Result<Path> Path::fromWide(std::wstring_view path) {
    GEODE_UNWRAP_INTO(auto utf8, wideToUtf8(path));
    return Ok(Path(std::move(utf8)));
}

Result<Path> Path::cwd() {
    std::array<wchar_t, 1024> buf;
    auto n = GetCurrentDirectoryW(buf.size(), buf.data());
    if (n == 0) {
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
    std::array<wchar_t, 512> buf;
    auto n = GetTempPath2W(buf.size(), buf.data());
    if (n == 0) {
        return Err(formatError());
    }
    GEODE_UNWRAP_INTO(auto utf8, wideToUtf8({buf.data(), n}));
    return Ok(Path(std::move(utf8)));
}

}
