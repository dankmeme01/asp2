#include <asp/fs/fs.hpp>
#include <cstring>
#include <fmt/format.h>

#ifdef _WIN32
# include <windows.h>
#else
# include <fcntl.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <unistd.h>
# include <cerrno>
#endif

template <typename T = void, typename E = asp::fs::Error>
using Result = asp::fs::Result<T, E>;

using asp::Ok;
using asp::Err;

using path = asp::fs::path;

// wraps a lambda and returns a result depending on error code
template <typename F, typename... Args, typename IRes = std::invoke_result_t<F, Args..., std::error_code&>> requires (std::invocable<F, Args..., std::error_code&>)
static Result<IRes> _fswrap(F&& function, Args&&... args) {
    std::error_code ec;
    auto result = function(std::forward<Args>(args)..., ec);

    if (ec == std::error_code{}) {
        return Ok(std::move(result));
    } else {
        return Err(ec);
    }
}

// _fswrapvoid for void functions
template <typename F, typename... Args> requires (std::invocable<F, Args..., std::error_code&>)
static Result<void> _fswrapvoid(F&& function, Args&&... args) {
    std::error_code ec{};
    function(std::forward<Args>(args)..., ec);

    if (ec == std::error_code{}) {
        return Ok();
    } else {
        return Err(ec);
    }
}

// File status

namespace asp::fs {

FileStatus::FileStatus(std::filesystem::file_status s) : m_status(s) {}

std::filesystem::file_type FileStatus::type() const {
    return m_status.type();
}

std::filesystem::perms FileStatus::permissions() const {
    return m_status.permissions();
}

bool FileStatus::isBlockFile() const {
    return std::filesystem::is_block_file(m_status);
}

bool FileStatus::isCharacterFile() const {
    return std::filesystem::is_character_file(m_status);
}

bool FileStatus::isDirectory() const {
    return std::filesystem::is_directory(m_status);
}

bool FileStatus::isFifo() const {
    return std::filesystem::is_fifo(m_status);
}

bool FileStatus::isOther() const {
    return std::filesystem::is_other(m_status);
}

bool FileStatus::isFile() const {
    return std::filesystem::is_regular_file(m_status);
}

bool FileStatus::isSocket() const {
    return std::filesystem::is_socket(m_status);
}

bool FileStatus::isSymlink() const {
    return std::filesystem::is_symlink(m_status);
}

bool FileStatus::isUnknown() const {
    return !std::filesystem::status_known(m_status);
}

} // namespace asp::fs

// Copy

Result<void> asp::fs::copy(const path& o, const path& d) {
    return _fswrapvoid([](const path& o, const path& d, std::error_code& ec) {
        std::filesystem::copy(o, d, ec);
    }, o, d);
}

Result<void> asp::fs::copy(const path& o, const path& d, std::filesystem::copy_options opts) {
    return _fswrapvoid([](const path& o, const path& d, std::filesystem::copy_options opts, std::error_code& ec) {
        std::filesystem::copy(o, d, opts, ec);
    }, o, d, opts);
}

// Rename

Result<void> asp::fs::rename(const path& oldPath, const path& newPath) {
    return _fswrapvoid([](const path& oldPath, const path& newPath, std::error_code& ec) {
        std::filesystem::rename(oldPath, newPath, ec);
    }, oldPath, newPath);
}

// Exists

bool asp::fs::exists(const path& p) {
    std::error_code ec;
    return std::filesystem::exists(p, ec);
}

// Create directory

Result<void> asp::fs::createDir(const path& p) {
    return _fswrapvoid([](const path& p, std::error_code& ec) {
        std::filesystem::create_directory(p, ec);
    }, p);
}

Result<void> asp::fs::createDirAll(const path& p) {
    return _fswrapvoid([](const path& p, std::error_code& ec) {
        std::filesystem::create_directories(p, ec);
    }, p);
}

// Query file/directory/status/equivalence

Result<asp::fs::FileStatus> asp::fs::status(const path& p) {
    return _fswrap([](const path& p, std::error_code& ec) {
        return std::filesystem::status(p, ec);
    }, p).map([](auto s) {
        return FileStatus(s);
    });
}

Result<bool> asp::fs::isFile(const path& p) {
    return asp::fs::status(p).map([](FileStatus status) {
        return status.isFile();
    });
}

Result<bool> asp::fs::isDirectory(const path& p) {
    return asp::fs::status(p).map([](FileStatus status) {
        return status.isDirectory();
    });
}

Result<bool> asp::fs::equivalent(const path& path1, const path& path2) {
    return _fswrap([](const path& path1, const path& path2, std::error_code& ec) {
        return std::filesystem::equivalent(path1, path2, ec);
    }, path1, path2);
}

// File metadata

Result<std::filesystem::file_time_type> asp::fs::lastWriteTime(const path& p) {
    return _fswrap([](const path& p, std::error_code& ec) {
        return std::filesystem::last_write_time(p, ec);
    }, p);
}

// Remove file/directory

Result<void> asp::fs::remove(const path& p) {
    return _fswrapvoid([](const path& p, std::error_code& ec) {
        std::filesystem::remove(p, ec);
    }, p);
}

Result<void> asp::fs::removeFile(const path& p) {
    auto statusr = asp::fs::status(p);
    if (!statusr) {
        return Err(std::move(statusr).unwrapErr());
    }

    auto status = std::move(statusr).unwrap();

    if (status.isDirectory()) {
        return Err(std::make_error_code(std::errc::is_a_directory));
    } else if (!status.isFile()) {
        return Err(std::make_error_code(std::errc::no_such_file_or_directory));
    }

    return _fswrapvoid([](const path& p, std::error_code& ec) {
        std::filesystem::remove(p, ec);
    }, p);
}

Result<void> asp::fs::removeDir(const path& p) {
    auto statusr = asp::fs::status(p);
    if (!statusr) {
        return Err(std::move(statusr).unwrapErr());
    }

    auto status = std::move(statusr).unwrap();

    if (!status.isDirectory()) {
        return Err(std::make_error_code(std::errc::not_a_directory));
    }

    return _fswrapvoid([](const path& p, std::error_code& ec) {
        std::filesystem::remove(p, ec);
    }, p);
}

Result<uintmax_t> asp::fs::removeDirAll(const path& p) {
    auto statusr = asp::fs::status(p);
    if (!statusr) {
        return Err(std::move(statusr).unwrapErr());
    }

    auto status = std::move(statusr).unwrap();

    if (!status.isDirectory()) {
        return Err(std::make_error_code(std::errc::not_a_directory));
    }

    return _fswrap([](const path& p, std::error_code& ec) {
        return std::filesystem::remove_all(p, ec);
    }, p);
}

Result<uintmax_t> asp::fs::removeAll(const path& p) {
    return _fswrap([](const path& p, std::error_code& ec) {
        return std::filesystem::remove_all(p, ec);
    }, p);
}

// Iteration

Result<std::filesystem::directory_iterator> asp::fs::iterdir(const path& p) {
    return _fswrap([](const path& p, std::error_code& ec) {
        return std::filesystem::directory_iterator(p, ec);
    }, p);
}

#ifdef _WIN32
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
#else
static std::string formatError(int error = errno) {
    return strerror(error); // thank you posix for making it simple
}
#endif

#ifdef _WIN32
template <typename T>
geode::Result<> readFileInto(std::filesystem::path const& path, T& out) {
    HANDLE file = CreateFileW(
        path.native().c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (file == INVALID_HANDLE_VALUE) {
        return Err(fmt::format("Unable to open file: {}", formatError()));
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(file, &fileSize)) {
        CloseHandle(file);
        return Err(fmt::format("Unable to get file size: {}", formatError()));
    }

    out.resize(fileSize.QuadPart);
    DWORD read = 0;
    if (!ReadFile(file, out.data(), static_cast<DWORD>(out.size()), &read, nullptr)) {
        CloseHandle(file);
        return Err(fmt::format("Unable to read file: {}", formatError()));
    }

    CloseHandle(file);

    if (read < out.size()) {
        return Err(fmt::format("Unable to read entire file: only read {} of {}", read, out.size()));
    }

    return Ok();
}

geode::Result<> writeFileFrom(std::filesystem::path const& path, void* data, size_t size) {
    HANDLE file = CreateFileW(
        path.native().c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (file == INVALID_HANDLE_VALUE) {
        return Err(fmt::format("Unable to open file: {}", formatError()));
    }

    DWORD written = 0;
    if (!WriteFile(file, data, static_cast<DWORD>(size), &written, nullptr)) {
        CloseHandle(file);
        return Err(fmt::format("Unable to write file: {}", formatError()));
    }

    if (written < size) {
        CloseHandle(file);
        return Err(fmt::format("Unable to write entire file: only wrote {} of {}", written, size));
    }

    CloseHandle(file);

    return Ok();
}

#else

template <typename T>
geode::Result<> readFileInto(std::filesystem::path const& path, T& out) {
    int file = open(path.native().c_str(), O_RDONLY);

    if (file == -1) {
        return Err(fmt::format("Unable to open file: {}", formatError()));
    }

    struct stat fst;
    if (fstat(file, &fst) == -1) {
        close(file);
        return Err(fmt::format("Unable to get file size: {}", formatError()));
    }

    out.resize(fst.st_size);
    ssize_t bread = read(file, out.data(), out.size());
    close(file);

    if (bread < 0) {
        return Err(fmt::format("Unable to read file: {}", formatError()));
    }

    if (bread < out.size()) {
        return Err(fmt::format("Unable to read entire file: only read {} of {}", bread, out.size()));
    }

    return Ok();
}

geode::Result<> writeFileFrom(std::filesystem::path const& path, void* data, size_t size) {
    int file = open(path.native().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (file < 0) {
        return Err(fmt::format("Unable to open file: {}", formatError()));
    }

    size_t written = 0;
    while (written < size) {
        ssize_t bwrite = write(file, (uint8_t*)data + written, size - written);
        if (bwrite < 0) {
            if (errno == EINTR) continue;
            close(file);
            return Err(fmt::format("Unable to write file: {}", formatError()));
        }
        written += bwrite;
    }

    close(file);

    return Ok();
}

#endif

// Reading

geode::Result<std::vector<uint8_t>> asp::fs::read(const path& path) {
    std::vector<uint8_t> data;
    GEODE_UNWRAP(readFileInto(path, data));
    return Ok(std::move(data));
}

geode::Result<std::string> asp::fs::readToString(const path& path) {
    std::string data;
    GEODE_UNWRAP(readFileInto(path, data));
    return Ok(std::move(data));
}

// Writing

geode::Result<void> asp::fs::write(const path& path, const std::vector<uint8_t>& data) {
    return write(path, reinterpret_cast<const char*>(data.data()), data.size());
}

geode::Result<void> asp::fs::write(const path& path, const std::string& data) {
    return write(path, data.c_str(), data.size());
}

geode::Result<void> asp::fs::write(const path& path, const unsigned char* data, size_t size) {
    return write(path, reinterpret_cast<const char*>(data), size);
}

geode::Result<void> asp::fs::write(const path& path, const char* data, size_t size) {
    return writeFileFrom(path, (void*)data, size);
}
