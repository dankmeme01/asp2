#pragma once
#include <asp/detail/Result.hpp>
#include <fmt/format.h>
#include <asp/ptr/BoxedString.hpp>
#include <optional>
#include <ostream>
#include <utility>

namespace asp::fs {

// NOTE (ABI): add new variants to the end

enum class ErrorKind : uint16_t {
    /// Operation was successful
    Success = 0,
    /// File/directory was not found
    NotFound,
    /// Insufficient permissions denied
    PermissionDenied,
    /// File/directory already exists
    AlreadyExists,
    /// A parameter was incorrect
    InvalidInput,
    /// Invalid or corrupted data was encountered
    InvalidData,
    /// Invalid filename for this platform
    InvalidFilename,
    /// Target is not a directory
    NotADirectory,
    /// Target is a directory
    IsADirectory,
    /// The target directory is not empty
    DirectoryNotEmpty,
    /// The filesystem is read-only
    ReadOnlyFilesystem,
    /// The filesystem is full
    StorageFull,
    /// A loop was detected (often, too many levels of symlinks)
    FilesystemLoop,
    /// Stale network file handle (NFS)
    StaleNetworkFileHandle,
    /// Filesystem quota or some other limit was exceeded
    QuotaExceeded,
    /// The file is too large than allowed or supported
    FileTooLarge,
    /// The resource is currently busy
    ResourceBusy,
    /// Executable file is busy
    ExecutableFileBusy,
    /// A file locking operation would result in deadlock
    Deadlock,
    /// An operation (hard link or rename) crosses multiple devices
    CrossesDevices,
    /// Too many hard links to the same filesystem object
    TooManyLinks,
    /// The file is not seekable
    NotSeekable,
    /// Operation would block
    WouldBlock,
    /// The operation is not supported on this platform
    Unsupported,
    /// Operation timed out
    TimedOut,
    /// The operation was interrupted by a signal
    Interrupted,
    /// The process or system has reached the limit of open files
    TooManyOpenFiles,
    /// Broken pipe
    BrokenPipe,
    /// Unexpected EOF was encountered when reading a file
    UnexpectedEof,
    /// Program argument list too long
    ArgumentListTooLong,
    /// An operation could not be completed because `write` returned 0 bytes, typically means only a smaller number of bytes can be written.
    WriteZero,
    /// An operation could not be completed because memory could not be allocated
    OutOfMemory,

    /// A custom error that is not represented by any other variant.
    Other,
};

std::string_view errorKindName(ErrorKind kind);

class Error {
public:
    Error(ErrorKind kind) noexcept;
    Error(ErrorKind kind, std::string_view message);

    Error(const Error&) = default;
    Error& operator=(const Error&) = default;
    Error(Error&&) noexcept = default;
    Error& operator=(Error&&) noexcept = default;

    template <typename... Args>
    static Error format(ErrorKind kind, fmt::format_string<Args...> fmt, Args&&... args) {
        return Error(kind, 0, asp::BoxedString::format(fmt, std::forward<Args>(args)...));
    }

    static Error fromOsError(int32_t code, std::string_view context = "");
    static Error fromLastOsError(std::string_view context = "");

    ErrorKind kind() const;

    std::string_view message() const;
    bool hasMessage() const;
    std::optional<int32_t> rawOsError() const;
    std::string toString() const;

    bool operator==(ErrorKind kind) const noexcept;
    friend bool operator==(ErrorKind kind, const Error& error) noexcept;

private:
    Error(ErrorKind kind, int32_t rawError, asp::BoxedString message);

    ErrorKind m_kind;
    int32_t m_rawError = 0;
    asp::BoxedString m_message;
};

inline std::string format_as(const Error& err) {
    return err.toString();
}

inline std::string_view format_as(ErrorKind kind) {
    return errorKindName(kind);
}

inline std::ostream& operator<<(std::ostream& os, const Error& err) {
    return os << err.toString();
}

template <typename T = void>
using Result = geode::Result<T, Error>;

}