#pragma once

#include <asp/detail/config.hpp>
#include "Path.hpp"
#include "File.hpp"
#include <vector>
#include <string>
#include <stdint.h>
#include "DirectoryIterator.hpp"

namespace asp::fs {

/// Copies the contents (including permissions) from the source file to the destination
Result<void> copy(PathView origin, PathView dest);

/// Renames a file or directory to a new name, replacing the destination if it already exists.
/// Also known as "move".
Result<void> rename(PathView oldPath, PathView newPath);

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

/// Returns an iterator over the files in the given directory.
Result<DirectoryIterator> iterDir(PathView path, bool recursive = false);

/// Reads the contents of the file into a vector of bytes.
Result<std::vector<uint8_t>> read(PathView path);
/// Reads the contents of the file into a string.
Result<std::string> readToString(PathView path);

/// Writes the given data to the file at the given path, creating or overwriting it.
Result<void> write(PathView path, std::span<const uint8_t> data);
Result<void> write(PathView path, std::string_view data);
Result<void> write(PathView path, const void*, size_t size);

// TODO: create symlink, read symlink

}
