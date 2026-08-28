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

// TODO: replace std fs here

using geode::Result;
using geode::Ok;
using geode::Err;

// wraps a lambda and returns a result depending on error code
template <typename F, typename... Args, typename IRes = std::invoke_result_t<F, Args..., std::error_code&>> requires (std::invocable<F, Args..., std::error_code&>)
static Result<IRes> _fswrap(F&& function, Args&&... args) {
    std::error_code ec;
    auto result = function(std::forward<Args>(args)..., ec);

    if (ec == std::error_code{}) {
        return Ok(std::move(result));
    } else {
        return Err(fmt::format("Filesystem operation failed: {}", ec.message()));
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
        return Err(fmt::format("Filesystem operation failed: {}", ec.message()));
    }
}

// Copy

Result<void> asp::fs::copy(PathView o, PathView d) {
    return _fswrapvoid([](PathView o, PathView d, std::error_code& ec) {
        std::filesystem::copy(o.view(), d.view(), ec);
    }, o, d);
}

Result<void> asp::fs::copy(PathView o, PathView d, std::filesystem::copy_options opts) {
    return _fswrapvoid([](PathView o, PathView d, std::filesystem::copy_options opts, std::error_code& ec) {
        std::filesystem::copy(o.view(), d.view(), opts, ec);
    }, o, d, opts);
}

// Rename

Result<void> asp::fs::rename(PathView oldPath, PathView newPath) {
    return _fswrapvoid([](PathView oldPath, PathView newPath, std::error_code& ec) {
        std::filesystem::rename(oldPath.view(), newPath.view(), ec);
    }, oldPath, newPath);
}

// Create directory

Result<void> asp::fs::createDir(PathView p) {
    return _fswrapvoid([](PathView p, std::error_code& ec) {
        std::filesystem::create_directory(p.view(), ec);
    }, p);
}

Result<void> asp::fs::createDirAll(PathView p) {
    return _fswrapvoid([](PathView p, std::error_code& ec) {
        std::filesystem::create_directories(p.view(), ec);
    }, p);
}

// Remove file/directory

Result<void> asp::fs::remove(PathView p) {
    return _fswrapvoid([](PathView p, std::error_code& ec) {
        std::filesystem::remove(p.view(), ec);
    }, p);
}

Result<void> asp::fs::removeFile(PathView p) {
    auto statusr = p.metadata();
    if (!statusr) {
        return Err(std::move(statusr).unwrapErr());
    }

    auto status = std::move(statusr).unwrap();

    if (status.isDir()) {
        return Err("Is a directory");
    } else if (!status.isFile()) {
        return Err("No such file");
    }

    return _fswrapvoid([](PathView p, std::error_code& ec) {
        std::filesystem::remove(p.view(), ec);
    }, p);
}

Result<void> asp::fs::removeDir(PathView p) {
    auto statusr = p.metadata();
    if (!statusr) {
        return Err(std::move(statusr).unwrapErr());
    }

    auto status = std::move(statusr).unwrap();

    if (!status.isDir()) {
        return Err("Not a directory");
    }

    return _fswrapvoid([](PathView p, std::error_code& ec) {
        std::filesystem::remove(p.view(), ec);
    }, p);
}

Result<uintmax_t> asp::fs::removeDirAll(PathView p) {
    auto statusr = p.metadata();
    if (!statusr) {
        return Err(std::move(statusr).unwrapErr());
    }

    auto status = std::move(statusr).unwrap();

    if (!status.isDir()) {
        return Err("Not a directory");
    }

    return _fswrap([](PathView p, std::error_code& ec) {
        return std::filesystem::remove_all(p.view(), ec);
    }, p);
}

Result<uintmax_t> asp::fs::removeAll(PathView p) {
    return _fswrap([](PathView p, std::error_code& ec) {
        return std::filesystem::remove_all(p.view(), ec);
    }, p);
}

// Iteration

Result<std::filesystem::directory_iterator> asp::fs::iterdir(PathView p) {
    return _fswrap([](PathView p, std::error_code& ec) {
        return std::filesystem::directory_iterator(p.view(), ec);
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
geode::Result<> readFileInto(asp::fs::PathView path, T& out) {
    HANDLE file = CreateFileW(
        path.native().unwrap().c_str(),
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

static Result<> writeFileFrom(asp::fs::PathView path, void* data, size_t size) {
    HANDLE file = CreateFileW(
        path.native().unwrap().c_str(),
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
geode::Result<> readFileInto(asp::fs::PathView path, T& out) {
    int file = open(path.string().c_str(), O_RDONLY);

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

static geode::Result<> writeFileFrom(asp::fs::PathView path, void* data, size_t size) {
    int file = open(path.string().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

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

geode::Result<std::vector<uint8_t>> asp::fs::read(PathView path) {
    std::vector<uint8_t> data;
    GEODE_UNWRAP(readFileInto(path, data));
    return Ok(std::move(data));
}

geode::Result<std::string> asp::fs::readToString(PathView path) {
    std::string data;
    GEODE_UNWRAP(readFileInto(path, data));
    return Ok(std::move(data));
}

// Writing

geode::Result<void> asp::fs::write(PathView path, const std::vector<uint8_t>& data) {
    return write(path, reinterpret_cast<const char*>(data.data()), data.size());
}

geode::Result<void> asp::fs::write(PathView path, const std::string& data) {
    return write(path, data.c_str(), data.size());
}

geode::Result<void> asp::fs::write(PathView path, const unsigned char* data, size_t size) {
    return write(path, reinterpret_cast<const char*>(data), size);
}

geode::Result<void> asp::fs::write(PathView path, const char* data, size_t size) {
    return writeFileFrom(path, (void*)data, size);
}
