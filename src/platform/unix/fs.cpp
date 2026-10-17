#include <asp/fs.hpp>
#include <asp/iter.hpp>
#include "helpers.hpp"

#include <fmt/format.h>
#include <array>
#include <memory>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <dirent.h>

#ifdef __linux__
# include <sys/sendfile.h>
#endif

namespace asp::fs {

static Result<struct stat> getStat(const std::string& path, bool followSymlink) {
    struct stat st;
    GEODE_UNWRAP(wrapCall(
        followSymlink ? "stat" : "lstat",
        followSymlink ? ::stat(path.c_str(), &st) : ::lstat(path.c_str(), &st)
    ));
    return Ok(st);
}

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

bool _impl::equivalent(std::string_view lhs, std::string_view rhs, bool followSymlink) noexcept {
    auto r1 = getStat(std::string{lhs}, followSymlink);
    if (!r1) return false;
    auto r2 = getStat(std::string{rhs}, followSymlink);
    if (!r2) return false;

    auto& st1 = r1.unwrap();
    auto& st2 = r2.unwrap();

    return st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino;
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
    GEODE_UNWRAP_INTO(auto st, getStat(std::string{path}, followSymlink));
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

#ifdef __linux__
static Result<> doCopyContents(int inFd, int outFd, size_t size) {
    off_t offset = 0;
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

Result<void> copy(PathView origin_, PathView dest_) {
    auto origin = origin_.string();
    auto dest = dest_.string();

    ScopeFd inFd = open(origin.c_str(), O_RDONLY);
    if (inFd < 0) {
        return Err(fmt::format("unable to open source file: {}", strerror(errno)));
    }

    ScopeFd outFd = open(dest.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outFd < 0) {
        return Err(fmt::format("unable to open destination file: {}", strerror(errno)));
    }

    struct stat st;
    fstat(inFd, &st);

    GEODE_UNWRAP(doCopyContents(inFd, outFd, st.st_size));

    // copy ownership if possible
    if (st.st_uid != geteuid() || st.st_gid != getegid()) {
        if (fchown(outFd, st.st_uid, st.st_gid) != 0 && errno != EPERM) {
            return Err(fmt::format("unable to copy ownership: {}", strerror(errno)));
        }
    }

    // copy permissions
    GEODE_UNWRAP(wrapCall("fchmod", fchmod(outFd, st.st_mode)));

    // copy timestamps
    struct timespec times[2] = {st.st_atim, st.st_mtim};
    GEODE_UNWRAP(wrapCall("futimens", futimens(outFd, times)));

    return Ok();
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
        return Err(fmt::format("Path '{}' is not a directory", path));
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

Result<void> createSymlink(PathView target, PathView link) {
    return wrapCall("symlink", ::symlink(target.string().c_str(), link.string().c_str()));
}

Result<void> createDirectorySymlink(PathView target, PathView link) {
    return createSymlink(target, link);
}

Result<Path> readSymlink(PathView path) {
    std::array<char, 1024> buf;
    ssize_t len = ::readlink(path.string().c_str(), buf.data(), buf.size());
    if (len < 0) {
        return Err(fmt::format("readlink failed: {}", path, strerror(errno)));
    }

    return Ok(Path{std::string{buf.data(), size_t(len)}});
}

}
