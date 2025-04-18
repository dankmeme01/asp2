#include <asp/fs/fs.hpp>
#include <fstream>

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

// Reading

Result<std::vector<uint8_t>> asp::fs::read(const path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Err(std::make_error_code(std::errc::no_such_file_or_directory));
    }

    return Ok(std::vector<uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));
}

Result<std::string> asp::fs::readToString(const path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return Err(std::make_error_code(std::errc::no_such_file_or_directory));
    }

    return Ok(std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));
}

// Writing

Result<void> asp::fs::write(const path& path, const std::vector<uint8_t>& data) {
    return write(path, reinterpret_cast<const char*>(data.data()), data.size());
}

Result<void> asp::fs::write(const path& path, const std::string& data) {
    return write(path, data.c_str(), data.size());
}

Result<void> asp::fs::write(const path& path, const unsigned char* data, size_t size) {
    return write(path, reinterpret_cast<const char*>(data), size);
}

Result<void> asp::fs::write(const path& path, const char* data, size_t size) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Err(std::make_error_code(std::errc::no_such_file_or_directory));
    }

    file.write(data, size);
    if (!file.good()) {
        return Err(std::make_error_code(std::errc::io_error));
    }

    return Ok();
}
