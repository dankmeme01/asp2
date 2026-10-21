#include <asp/fs/Error.hpp>

namespace asp::fs {

std::string_view errorKindName(ErrorKind kind) {
    switch (kind) {
        case ErrorKind::Success: return "Success";
        case ErrorKind::NotFound: return "NotFound";
        case ErrorKind::PermissionDenied: return "PermissionDenied";
        case ErrorKind::AlreadyExists: return "AlreadyExists";
        case ErrorKind::InvalidInput: return "InvalidInput";
        case ErrorKind::InvalidData: return "InvalidData";
        case ErrorKind::InvalidFilename: return "InvalidFilename";
        case ErrorKind::NotADirectory: return "NotADirectory";
        case ErrorKind::IsADirectory: return "IsADirectory";
        case ErrorKind::DirectoryNotEmpty: return "DirectoryNotEmpty";
        case ErrorKind::ReadOnlyFilesystem: return "ReadOnlyFilesystem";
        case ErrorKind::StorageFull: return "StorageFull";
        case ErrorKind::FilesystemLoop: return "FilesystemLoop";
        case ErrorKind::StaleNetworkFileHandle: return "StaleNetworkFileHandle";
        case ErrorKind::QuotaExceeded: return "QuotaExceeded";
        case ErrorKind::FileTooLarge: return "FileTooLarge";
        case ErrorKind::ResourceBusy: return "ResourceBusy";
        case ErrorKind::ExecutableFileBusy: return "ExecutableFileBusy";
        case ErrorKind::Deadlock: return "Deadlock";
        case ErrorKind::CrossesDevices: return "CrossesDevices";
        case ErrorKind::TooManyLinks: return "TooManyLinks";
        case ErrorKind::NotSeekable: return "NotSeekable";
        case ErrorKind::WouldBlock: return "WouldBlock";
        case ErrorKind::Unsupported: return "Unsupported";
        case ErrorKind::TimedOut: return "TimedOut";
        case ErrorKind::Interrupted: return "Interrupted";
        case ErrorKind::TooManyOpenFiles: return "TooManyOpenFiles";
        case ErrorKind::BrokenPipe: return "BrokenPipe";
        case ErrorKind::UnexpectedEof: return "UnexpectedEof";
        case ErrorKind::ArgumentListTooLong: return "ArgumentListTooLong";
        case ErrorKind::WriteZero: return "WriteZero";
        case ErrorKind::OutOfMemory: return "OutOfMemory";
        case ErrorKind::Other: return "Other";
    }
    return "Unknown";
}

Error::Error(ErrorKind kind) noexcept : m_kind(kind) {}

Error::Error(ErrorKind kind, std::string_view message) : m_kind(kind), m_message(message) {}

Error::Error(ErrorKind kind, int32_t rawError, asp::BoxedString message)
    : m_kind(kind), m_rawError(rawError), m_message(std::move(message)) {}

ErrorKind Error::kind() const {
    return m_kind;
}

std::string_view Error::message() const {
    return m_message;
}

bool Error::hasMessage() const {
    return !m_message.empty();
}

std::optional<int32_t> Error::rawOsError() const {
    return m_rawError != 0 ? std::optional{m_rawError} : std::nullopt;
}

std::string Error::toString() const {
    if (m_kind == ErrorKind::Other && !m_rawError) {
        return std::string{m_message};
    }

    std::string out{errorKindName(m_kind)};
    if (!m_message.empty()) {
        out += ": ";
        out += m_message;
    }
    if (m_rawError) {
        out += fmt::format(" (os error {})", m_rawError);
    }

    return out;
}

bool Error::operator==(ErrorKind kind) const noexcept {
    return m_kind == kind;
}

bool operator==(ErrorKind kind, const Error& error) noexcept {
    return error == kind;
}

}