#include <asp/fs/Error.hpp>
#include <Windows.h>

namespace asp::fs {

static std::string formatError(DWORD error) {
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
        return "";
    }

    std::string message(buffer, size);
    LocalFree(buffer);

    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }

    return message;
}

static std::optional<ErrorKind> kindFromWinError(DWORD code) {
    switch (code) {
        case ERROR_SUCCESS: return ErrorKind::Success;
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_BAD_NETPATH:
        case ERROR_BAD_NET_NAME:
        case ERROR_CANT_RESOLVE_FILENAME:
            return ErrorKind::NotFound;
        case ERROR_ACCESS_DENIED:
        case ERROR_NETWORK_ACCESS_DENIED:
        case ERROR_CANT_ACCESS_FILE:
            return ErrorKind::PermissionDenied;
        case ERROR_ALREADY_EXISTS:
        case ERROR_FILE_EXISTS:
            return ErrorKind::AlreadyExists;
        case ERROR_INVALID_FUNCTION:
        case ERROR_INVALID_PARAMETER:
        case ERROR_INVALID_FLAGS:
        case ERROR_BAD_ARGUMENTS:
        case ERROR_INVALID_HANDLE:
            return ErrorKind::InvalidInput;
        case ERROR_NO_UNICODE_TRANSLATION:
        case ERROR_INVALID_DATA:
        case ERROR_CRC:
            return ErrorKind::InvalidData;
        case ERROR_BAD_PATHNAME:
        case ERROR_FILENAME_EXCED_RANGE:
        case ERROR_INVALID_NAME:
            return ErrorKind::InvalidFilename;
        case ERROR_DIRECTORY:
            return ErrorKind::NotADirectory;
        case ERROR_DIR_NOT_EMPTY:
            return ErrorKind::DirectoryNotEmpty;
        case ERROR_WRITE_PROTECT:
            return ErrorKind::ReadOnlyFilesystem;
        case ERROR_DISK_FULL:
        case ERROR_HANDLE_DISK_FULL:
            return ErrorKind::StorageFull;
        case ERROR_POSSIBLE_DEADLOCK:
            return ErrorKind::Deadlock;
        case ERROR_NOT_SAME_DEVICE:
            return ErrorKind::CrossesDevices;
        case ERROR_TOO_MANY_LINKS:
            return ErrorKind::TooManyLinks;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
            return ErrorKind::OutOfMemory;
        case ERROR_TIMEOUT:
            return ErrorKind::TimedOut;
        case ERROR_BROKEN_PIPE:
        case ERROR_NO_DATA:
            return ErrorKind::BrokenPipe;
        case ERROR_NOT_SUPPORTED:
            return ErrorKind::Unsupported;
        case ERROR_HANDLE_EOF:
            return ErrorKind::UnexpectedEof;
        case ERROR_TOO_MANY_OPEN_FILES:
            return ErrorKind::TooManyOpenFiles;
        case ERROR_SHARING_VIOLATION:
        case ERROR_LOCK_VIOLATION:
            return ErrorKind::ResourceBusy;
        default: return std::nullopt;
    }
}

Error Error::fromOsError(int32_t code, std::string_view context) {
    auto kind = kindFromWinError(code).value_or(ErrorKind::Other);

    auto formatted = formatError(code);
    if (formatted.empty()) {
        formatted = fmt::format("Unknown OS error {}", code);
    }

    if (context.empty()) {
        return Error(kind, code, formatted);
    } else {
        return Error(kind, code, fmt::format("{}: {}", context, formatted));
    }
}

Error Error::fromLastOsError(std::string_view context) {
    return fromOsError(GetLastError(), context);
}


}
