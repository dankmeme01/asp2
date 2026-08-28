#include <asp/fs.hpp>
#include <fmt/format.h>
#include <array>
#include <unistd.h>

namespace asp::fs {

Result<Path> _impl::fromStdPath(const std::filesystem::path& path) {
    return Ok(Path{ path.string() });
}

Result<NativePath> _impl::native(std::string_view path) {
    return Ok(NativePath{std::string(path)});
}

bool _impl::isAbsolute(std::string_view path) noexcept {
    return path.size() >= 1 && path[0] == '/';
}

std::optional<std::string_view> _impl::rootName(std::string_view path) noexcept {
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
    return stat(path.data(), &st) == 0;
}

static asp::SystemTime toSystemTime(struct timespec ts) {
    asp::Duration dur(ts.tv_sec, ts.tv_nsec);
    return asp::SystemTime::UNIX_EPOCH + dur;
}

Result<Metadata> _impl::metadata(std::string_view path) noexcept {
    struct stat st;
    if (stat(path.data(), &st) != 0) {
        return Err(fmt::format("Unable to get file metadata: {}", strerror(errno)));
    }

    Metadata meta;
    if (S_ISREG(st.st_mode)) {
        meta.type = FileType::RegularFile;
    } else if (S_ISDIR(st.st_mode)) {
        meta.type = FileType::Directory;
    } else if (S_ISLNK(st.st_mode)) {
        meta.type = FileType::Symlink;
    } else if (S_ISCHR(st.st_mode)) {
        meta.type = FileType::CharDevice;
    } else if (S_ISBLK(st.st_mode)) {
        meta.type = FileType::BlockDevice;
    } else if (S_ISFIFO(st.st_mode)) {
        meta.type = FileType::Fifo;
    } else if (S_ISSOCK(st.st_mode)) {
        meta.type = FileType::Socket;
    } else {
        meta.type = FileType::Other;
    }

    // TODO: Permissions

    meta.size = st.st_size;

    auto filename = _impl::filename(path);
    meta.hidden = filename && filename->starts_with('.') && *filename != "." && *filename != "..";

#if defined(__APPLE__)
    meta.modifiedAt = toSystemTime(st.st_mtimespec);
    meta.accessedAt = toSystemTime(st.st_atimespec);
    meta.createdAt  = toSystemTime(st.st_birthtimespec);
#else
    meta.modifiedAt = toSystemTime(st.st_mtim);
    meta.accessedAt = toSystemTime(st.st_atim);
    // TODO: no creation time, use statx?
#endif

    return Ok(meta);
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

#ifdef __linux__ // other impls live in their directories
Result<Path> Path::tmp() {
    auto tmpdir = getenv("TMPDIR");
    if (tmpdir && *tmpdir) {
        return Ok(Path(tmpdir));
    }
    return Ok(Path{"/tmp"});
}
#endif

}
