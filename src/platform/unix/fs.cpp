#include <asp/fs.hpp>
#include <asp/iter.hpp>
#include "helpers.hpp"

#include <fmt/format.h>
#include <array>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <pwd.h>
#include <fcntl.h>
#include <dirent.h>

namespace asp::fs {

Result<NativePath> _impl::native(std::string_view path) {
    return Ok(NativePath{std::string(path)});
}

bool _impl::isAbsolute(std::string_view path) noexcept {
    return path.size() >= 1 && path[0] == '/';
}

std::optional<std::string_view> _impl::rootName(std::string_view path) noexcept {
    if (isAbsolute(path)) {
        return "/";
    }
    return std::nullopt;
}

Result<Path> _impl::canonicalize(std::string_view path) {
    // unfortunately forced to null terminate
    std::string copy{path};
    std::unique_ptr<char, decltype(&free)> resolved{
        realpath(copy.c_str(), nullptr),
        &free
    };

    if (!resolved) {
        return Err(fmt::format("Unable to canonicalize path: {}", strerror(errno)));
    }

    return Ok(Path(resolved.get()));
}

Result<Path> _impl::absolute(std::string_view path) {
    if (_impl::isAbsolute(path)) return Ok(Path{path});

    GEODE_UNWRAP_INTO(auto cwd, Path::cwd());
    return Ok(std::move(cwd) / path);
}

bool _impl::exists(std::string_view path) noexcept {
    struct stat st;
    return stat(std::string{path}.c_str(), &st) == 0;
}

Result<Metadata> _impl::metadata(std::string_view path, bool followSymlink) {
    struct stat st;

    auto p = std::string{path};
    GEODE_UNWRAP(wrapCall(followSymlink ? "stat" : "lstat",
        followSymlink ? stat(p.c_str(), &st) : lstat(p.c_str(), &st)
    ));

    return Ok(metadataFromStat(st));
}

Path& Path::makePreferred() noexcept {
    return *this;
}

Result<Path> Path::cwd() {
    std::array<char, 1024> buf;
    auto p = getcwd(buf.data(), buf.size());
    if (!p) {
        return Err(fmt::format("getcwd failed: {}", strerror(errno)));
    }
    return Ok(Path(buf.data()));
}

Result<Path> Path::home() {
    auto home = getenv("HOME");
    if (home && *home) {
        return Ok(Path(home));
    }

    auto uid = getuid();
    auto pw = getpwuid(uid);

    if (pw && pw->pw_dir && *pw->pw_dir) {
        return Ok(Path(pw->pw_dir));
    }

    return Err("Unable to determine home directory");
}

#if defined(__linux__) && !defined(__ANDROID__) // other impls live in their directories
Result<Path> Path::tmp() {
    auto tmpdir = getenv("TMPDIR");
    if (tmpdir && *tmpdir) {
        return Ok(Path(tmpdir));
    }
    return Ok(Path{"/tmp"});
}
#endif

// -- file op (asp::fs) implementations -- //

struct ScopeFd {
    int fd;
    ScopeFd(int fd) : fd(fd) {}
    ~ScopeFd() {
        if (fd >= 0) {
            close(fd);
        }
    }

    operator int() const {
        return fd;
    }
};

// TODO: apple impl
static Result<> doCopyContents(int inFd, int outFd, size_t size) {
    off_t offset = 0;
    while (offset < size) {
        auto copied = copy_file_range(inFd, nullptr, outFd, nullptr, size - offset, 0);
        if (copied > 0) {
            offset += copied;
            continue;
        } else if (copied == -1) {
            int e = errno;
            if (e != ENOSYS && e != EXDEV && e != EINVAL) {
                return Err(fmt::format("unable to copy file: {}", strerror(e)));
            }

            // otherwise, fallback to another method
            break;
        } else if (copied == 0) {
            // this shouldn't happen unless the file was spontaneously truncated
            return Err("unable to copy file: unexpected EOF");
        }
    }

    if (offset == size) {
        return Ok();
    }

    // copy_file_range failed, do a sendfile as a fallback
    // explicitly keep offset in case the function above somehow transferred part of file, though should be impossible

    while (offset < size) {
        auto sent = ::sendfile(outFd, inFd, &offset, size - offset);
        if (sent < 0) {
            return Err(fmt::format("unable to copy file: {}", strerror(errno)));
        } else if (sent == 0) {
            // this shouldn't happen unless the file was spontaneously truncated
            return Err("unable to copy file: unexpected EOF");
        }
    }

    return Ok();
}

#ifdef __linux__
Result<void> copy(PathView origin, PathView dest) {
    ScopeFd inFd = open(origin.string().c_str(), O_RDONLY);
    if (inFd < 0) {
        return Err(fmt::format("unable to open source file: {}", strerror(errno)));
    }

    ScopeFd outFd = open(dest.string().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outFd < 0) {
        return Err(fmt::format("unable to open destination file: {}", strerror(errno)));
    }

    struct stat st;
    fstat(inFd, &st);

    GEODE_UNWRAP(doCopyContents(inFd, outFd, st.st_size));

    // copy permissions
    return wrapCall("fchmod", fchmod(outFd, st.st_mode));
}
#endif

Result<void> rename(PathView oldPath, PathView newPath) {
    return wrapCall("rename", ::rename(oldPath.string().c_str(), newPath.string().c_str()));
}

Result<void> createDir(PathView path) {
    return wrapCall("mkdir", ::mkdir(path.string().c_str(), 0755));
}

Result<void> removeFile(PathView path) {
    return wrapCall("unlink", ::unlink(path.string().c_str()));
}

Result<void> removeDir(PathView path) {
    return wrapCall("rmdir", ::rmdir(path.string().c_str()));
}

using ScopeDir = std::unique_ptr<DIR, decltype(&closedir)>;

static Result<> findAndRemoveAllInside(PathView path, std::vector<Path>& out) {
    auto dir = ScopeDir{opendir(path.string().c_str()), &closedir};
    if (!dir) {
        return Err(fmt::format("Unable to open directory '{}': {}", path, strerror(errno)));
    }

    struct dirent* entry;
    while ((entry = readdir(dir.get())) != nullptr) {
        std::string_view name{entry->d_name};
        if (name == "." || name == "..") continue;

        auto fullPath = path / name;

        struct stat st;
        if (lstat(fullPath.c_str(), &st) != 0) {
            return Err(fmt::format("Unable to stat '{}': {}", fullPath, strerror(errno)));
        }

        if (!S_ISDIR(st.st_mode)) {
            // this is a regular file or symlink, delete it
            GEODE_UNWRAP(removeFile(fullPath));
        } else {
            // directory, queue it
            out.push_back(std::move(fullPath));
        }
    }

    return Ok();
}

Result<void> removeDirAll(PathView path) {
    if (!path.isDir()) {
        return Err(fmt::format("Path is not a directory", path));
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

}
