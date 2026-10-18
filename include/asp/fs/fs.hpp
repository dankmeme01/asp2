#pragma once

#include <asp/detail/config.hpp>
#include "Path.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <span>
#include "DirectoryIterator.hpp"

namespace asp::fs {

/// Checks if a file exists, returns `false` on failure
bool exists(PathView path);

/// If `origin` is a file or a symlink, makes a copy of the file at `dest`, including permissions.
/// If `origin` is a directory, creates a directory at `dest` and copies all files and directories inside it recursively.
/// If `dest` already exists, it will be replaced; and if a directory is copied, contents will be merged and existing files will be overwritten.
Result<void> copy(PathView origin, PathView dest);

/// Renames a file or directory to a new name, replacing the destination if it already exists.
/// This is a simple, atomic OS function, and it may fail if the source and destination are on different drives, or the source is a directory.
/// For a cross-platform 'move' operation, use `asp::fs::move`.
Result<void> rename(PathView oldPath, PathView newPath);

/// Moves a file or directory to a new location, replacing the destination if it already exists.
/// Unlike `rename`, this isn't atomic so if a simple rename fails, this will fall back to a copy + delete, which may result in partial work done upon failure.
Result<void> move(PathView oldPath, PathView newPath);

/// Creates an empty directory at the given path.
Result<void> createDir(PathView path);
/// Recursively creates a directory and all its parent directories if they do not exist.
Result<void> createDirAll(PathView path);

/// Obtains the metadata of the file or directory at this path. Returns an error in the following situations (but not limited to):
/// * Path does not exist
/// * Path is a symlink and the target does not exist
/// * Lack of permissions
inline Result<Metadata> metadata(PathView path) {
    return path.metadata();
}

/// Obtains the metadata of the file or symlink at this path. If the final component is a symlink, unlike `metadata`,
/// this does NOT follow the symlink and instead returns metadata of the actual final symlink.
/// Returns an error in the following situations (but not limited to):
/// * Path does not exist (but symlink target may or may not exist)
/// * Lack of permissions
inline Result<Metadata> symlinkMetadata(PathView path) {
    return path.symlinkMetadata();
}

/// Returns the file size of the file at the given path, shorthand for extracting it from `metadata()`
inline Result<uint64_t> fileSize(PathView path) {
    return metadata(path).andThen([](Metadata meta) -> Result<uint64_t> {
        if (meta.isFile()) {
            return Ok(meta.size);
        } else {
            return Err("not a file");
        }
    });
}

/// Removes a file or an empty directory by the given path.
/// If the path is a symlink, deletes the symlink.
Result<void> remove(PathView path);
/// Removes a file by the given path, fails if it is not a file.
/// If the path is a symlink, deletes the symlink.
Result<void> removeFile(PathView path);
/// Removes a directory by the given path, fails if it is not a directory.
Result<void> removeDir(PathView path);

/// Removes a directory by the given path, recursively including all files and directories inside. Fails if it is not a directory (including if it's a symlink).
/// This will not follow symlinks inside the directory, and will simply delete the symbolic links themselves.
Result<void> removeDirAll(PathView path);

/// Returns an iterator over the files and directories in the given directory.
/// For recursive iteration, use `iterDirRecursive`.
Result<DirectoryIterator> iterDir(PathView path);

/// Returns an iterator over the files and directories in the given directory, recursively including all subdirectories.
/// This does not follow any symlinks, and simply yields them, even if they point to a directory.
Result<RecursiveDirectoryIterator> iterDirRecursive(PathView path);

/// Reads the contents of the file into a vector of bytes.
Result<std::vector<uint8_t>> read(PathView path);
/// Reads the contents of the file into a string.
Result<std::string> readToString(PathView path);

/// Writes the given data to the file at the given path, creating or overwriting it.
Result<void> write(PathView path, std::span<const uint8_t> data);
Result<void> write(PathView path, std::string_view data);
Result<void> write(PathView path, const void*, size_t size);

/// Creates a *file* symlink at `link` that points to `target`, similar to POSIX `symlink()` call.
/// Depending on the platform, this may fail if `target` does not exist or is a directory.
Result<void> createSymlink(PathView target, PathView link);

/// Creates a *directory* symlink at `link` that points to `target`, similar to POSIX `symlink()` call.
/// Depending on the platform, this may fail if `target` does not exist or is not a directory.
Result<void> createDirSymlink(PathView target, PathView link);

/// Reads a symlink, returning the path it points to. Returns an error if the path is not a symlink.
Result<Path> readSymlink(PathView path);

namespace _impl {
    Result<void> copyFile(PathView origin, PathView dest);
    Result<void> moveManual(PathView oldPath, PathView newPath);
}
}

// TODO: impl `move` for directories