#include <asp/fs/Error.hpp>
#include <cerrno>

namespace asp::fs {

static std::optional<ErrorKind> kindFromErrno(int32_t code) {
    switch (code) {
        case 0: return ErrorKind::Success;
        case ENOENT: return ErrorKind::NotFound;
        case EACCES:
        case EPERM: return ErrorKind::PermissionDenied;
        case EEXIST: return ErrorKind::AlreadyExists;
        case EINVAL: return ErrorKind::InvalidInput;
        case EILSEQ: return ErrorKind::InvalidData;
        case ENAMETOOLONG: return ErrorKind::InvalidFilename;
        case ENOTDIR: return ErrorKind::NotADirectory;
        case EISDIR: return ErrorKind::IsADirectory;
        case ENOTEMPTY: return ErrorKind::DirectoryNotEmpty;
        case EROFS: return ErrorKind::ReadOnlyFilesystem;
        case ENOSPC: return ErrorKind::StorageFull;
        case ELOOP: return ErrorKind::FilesystemLoop;
        case ESTALE: return ErrorKind::StaleNetworkFileHandle;
        case EDQUOT: return ErrorKind::QuotaExceeded;
        case EFBIG: return ErrorKind::FileTooLarge;
        case EBUSY: return ErrorKind::ResourceBusy;
        case ETXTBSY: return ErrorKind::ExecutableFileBusy;
        case EDEADLK: return ErrorKind::Deadlock;
        case EXDEV: return ErrorKind::CrossesDevices;
        case EMLINK: return ErrorKind::TooManyLinks;
        case ESPIPE: return ErrorKind::NotSeekable;
        case EAGAIN:
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK:
#endif
            return ErrorKind::WouldBlock;
        case ENOSYS:
#if defined(ENOTSUP) && ENOTSUP != ENOSYS
        case ENOTSUP:
#endif
#if defined(EOPNOTSUPP) && EOPNOTSUPP != ENOSYS && EOPNOTSUPP != ENOTSUP
        case EOPNOTSUPP:
#endif
            return ErrorKind::Unsupported;
        case ETIMEDOUT: return ErrorKind::TimedOut;
        case EINTR: return ErrorKind::Interrupted;
        case EMFILE:
        case ENFILE: return ErrorKind::TooManyOpenFiles;
        case EPIPE: return ErrorKind::BrokenPipe;
        case E2BIG: return ErrorKind::ArgumentListTooLong;
        case ENOMEM: return ErrorKind::OutOfMemory;
        default: return std::nullopt;
    }
}

static const char* messageFromErrno(int32_t code) {
    return strerror(code);
}

Error Error::fromOsError(int32_t code, std::string_view context) {
    auto kind = kindFromErrno(code).value_or(ErrorKind::Other);

    if (context.empty()) {
        return Error(kind, code, messageFromErrno(code));
    } else {
        return Error(kind, code, fmt::format("{}: {}", context, messageFromErrno(code)));
    }
}

Error Error::fromLastOsError(std::string_view context) {
    return fromOsError(errno, context);
}


}
