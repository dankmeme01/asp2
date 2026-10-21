#include <asp/fs/fs.hpp>
#include <asp/fs/File.hpp>
#include <asp/iter.hpp>
#include "util.hpp"

#include <cstring>
#include <fmt/format.h>

namespace asp::fs {

bool exists(PathView path) {
    return path.exists();
}

static Result<void> copySymlink(PathView link, PathView destLink) {
    GEODE_UNWRAP_INTO(auto target, link.readSymlink());

    // windows is stupid and needs to distinguish file/dir symlinks
    Path resolved = target.isAbsolute() ? target : link.parent().value_or(PathView{"."}) / target;
    bool isDir = resolved.isDir();

    return isDir ? createDirSymlink(target, destLink) : createSymlink(target, destLink);
}

static Result<void> copyDir(PathView origin, PathView dest) {
    struct PendingDir {
        Path src, dst;
    };

    std::vector<PendingDir> pending;
    pending.emplace_back(origin.toPath(), dest.toPath());

    for (size_t i = 0; i < pending.size(); i++) {
        auto src = pending[i].src;
        auto dst = pending[i].dst;

        GEODE_UNWRAP(createDirAll(dst));
        GEODE_UNWRAP_INTO(auto it, iterDir(src));

        for (auto next : it) {
            GEODE_UNWRAP_INTO(auto entry, std::move(next));
            GEODE_UNWRAP_INTO(auto meta, entry.path.symlinkMetadata());

            auto name = entry.path.filename().value();
            auto child = dst / name;

            switch (meta.type) {
                case FileType::Directory: pending.push_back({entry.path, child}); break;
                case FileType::Symlink: GEODE_UNWRAP(copySymlink(entry.path, child)); break;
                case FileType::RegularFile: GEODE_UNWRAP(_impl::copyFile(entry.path, child)); break;
                default: return Err(Error::format(ErrorKind::Unsupported, "Unsupported file type for '{}'", entry.path));
            }
        }
    }

    return Ok();
}

Result<void> copy(PathView origin, PathView dest) {
    GEODE_UNWRAP_INTO(auto meta, origin.metadata());

    if (meta.isFile()) {
        return _impl::copyFile(origin, dest);
    } else if (meta.isSymlink()) {
        return copySymlink(origin, dest);
    } else {
        return copyDir(origin, dest);
    }
}

Result<void> _impl::moveManual(PathView src, PathView dst) {
    GEODE_UNWRAP_INTO(auto meta, src.symlinkMetadata());

    if (meta.isDir()) {
        GEODE_UNWRAP(copy(src, dst));
        return removeDirAll(src);
    }

    if (meta.isSymlink()) {
        GEODE_UNWRAP(copySymlink(src, dst));
        return remove(src);
    }

    GEODE_UNWRAP(_impl::copyFile(src, dst));
    return removeFile(src);
}

Result<void> createDirAll(PathView path) {
    auto view = path.view();
    while (endsWithOneOf(view, SupportedSep)) {
        view.remove_suffix(1);
    }

    if (view.empty()) {
        return Ok();
    }

    std::vector<Path> toCreate;
    Path p{view};

    while (!p.empty() && !p.exists()) {
        toCreate.push_back(p);
        p = p.parent().value_or(PathView{});
    }

    if (toCreate.empty()) {
        if (path.isDir()) {
            return Ok();
        } else {
            return Err(Error(ErrorKind::NotADirectory, fmt::format("Path '{}' exists but is not a directory", p)));
        }
    }

    for (auto& p : asp::iter::fromReverse(toCreate)) {
        GEODE_UNWRAP(createDir(p));
    }

    return Ok();
}

Result<void> remove(PathView path) {
    GEODE_UNWRAP_INTO(auto metadata, path.symlinkMetadata());

    if (metadata.isDir()) {
        return removeDir(path);
    } else {
        return removeFile(path); // symlink falls back here too
    }
}

Result<DirectoryIterator> iterDir(PathView path) {
    return DirectoryIterator::create(path);
}

Result<RecursiveDirectoryIterator> iterDirRecursive(PathView path) {
    return RecursiveDirectoryIterator::create(path);
}

Result<std::vector<uint8_t>> read(PathView path) {
    return File::open(path).andThen([](File file) {
        return file.readToEnd();
    });
}

Result<std::string> readToString(PathView path) {
    return File::open(path).andThen([](File file) {
        return file.readToString();
    });
}

Result<void> write(PathView path, std::span<const uint8_t> data) {
    GEODE_UNWRAP_INTO(auto file, File::create(path));
    return file.writeAll(data);
}

Result<void> write(PathView path, std::string_view data) {
    return write(path, std::span{reinterpret_cast<const uint8_t*>(data.data()), data.size()});
}

Result<void> write(PathView path, const void* data, size_t size) {
    return write(path, std::span{reinterpret_cast<const uint8_t*>(data), size});
}

}