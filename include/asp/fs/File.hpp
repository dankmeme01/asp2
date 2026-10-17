#pragma once
#include "Path.hpp"
#include "Metadata.hpp"
#include "OpenOptions.hpp"
#include <span>
#include <vector>

namespace asp::fs {

#ifdef _WIN32
using NativeFd = void*;
const inline auto INVALID_FD = (NativeFd)(-1);
#else
using NativeFd = int;
const inline auto INVALID_FD = -1;
#endif

static constexpr size_t FILE_BUFFER_DEFAULT_SIZE = 16384;

class BufferedFile;

class File {
public:
    ~File();

    File(const File&) = delete;
    File& operator=(const File&) = delete;
    File(File&& other) noexcept;
    File& operator=(File&& other) noexcept;

    /// Opens a file with the given options. By default, opens it in a binary read-only mode.
    static Result<File> open(PathView path, const OpenOptions& options = {});

    /// Opens a file with the given options and buffering enabled. Shorthand for creating a `BufferedFile` manually.
    static Result<BufferedFile> openBuffered(PathView path, const OpenOptions& options = {}, size_t bufSize = FILE_BUFFER_DEFAULT_SIZE);

    /// Opens a file in write-only mode, creates a file if it does not exist and truncating otherwise.
    /// This may fail if the directory does not exist. This is simply a shorthand for `open` with the appropriate `OpenOptions`.
    static Result<File> create(PathView path) {
        OpenOptions opts{};
        opts.read = false;
        opts.write = true;
        opts.truncate = true;
        opts.create = true;
        return File::open(path, opts);
    }

    /// Opens a file in write-only buffered mode, creates a file if it does not exist and truncating otherwise.
    /// This is a shorthand for using `create` and then wrapping in a `BufferedFile`.
    static Result<BufferedFile> createBuffered(PathView path, size_t bufSize = FILE_BUFFER_DEFAULT_SIZE);

    /// Like `create`, but additionally this will error if the file already exists.
    static Result<File> createNew(PathView path) {
        OpenOptions opts{};
        opts.read = false;
        opts.write = true;
        opts.truncate = true;
        opts.create = true;
        opts.createNew = true;
        return File::open(path, opts);
    }

    /// Returns the underlying file descriptor
    inline NativeFd fd() const {
        return m_fd;
    }

    /// Attempts to sync the file state to disk.
    /// Note that this is not a generic "flush" operation; `File` is unbuffered so data is immediately flushed to the kernel.
    /// This function tells the kernel to drop its cache and sync the data to disk as soon as possible.
    Result<> sync();

    /// Acquires an exclusive lock on the file, and blocks the thread until the lock can be acquired.
    /// No other file handle to this file, in this or any other process, may acquire another lock.
    ///
    /// If already holding a lock or the file isn't open for writing, the behavior is unspecified.
    ///
    /// On Unix, this uses `flock` and is advisory (another process may disregard the lock)
    /// On Windows, this uses `LockFileEx` and is mandatory (another process will be blocked from writing)
    Result<> lock();

    /// Attempts to acquire an exclusive lock on the file, returning an error if the lock cannot be acquired immediately.
    Result<> tryLock();

    /// Acquires a shared lock on the file, and blocks the thread until the lock can be acquired.
    /// Other file handles to this file may acquire a shared lock, but no file handle may acquire an exclusive lock.
    Result<> lockShared();

    /// Attempts to acquire a shared lock on the file, returning an error if the lock cannot be acquired immediately.
    Result<> tryLockShared();

    /// Releases all locks on the file.
    Result<> unlock();

    /// Truncates or extends the underlying file to the given length. If the file is extended, the new bytes are filled with zeros.
    /// This does not affect the file cursor.
    Result<> setLength(uint64_t length);

    /// Queries metadata about the file.
    Result<Metadata> metadata();

    /// Tries to clone the file handle and returns a new File instance pointing to the same file on disk.
    Result<File> clone();


    // read funcs

    // TODO: writev, readv

    /// Reads up to `buf.size()` bytes from the file into the buffer, returning the number of bytes read.
    /// If the file cursor is at the end of the file, this will return 0.
    Result<size_t> read(std::span<uint8_t> buf);

    /// Reads exactly `buf.size()` bytes and places them in a buffer. If not enough bytes can be read, this will return an error.
    Result<> readExact(std::span<uint8_t> buf);

    /// Reads all bytes until EOF and returns them in a vector.
    Result<std::vector<uint8_t>> readToEnd();

    /// Reads all bytes until EOF and returns them in a string.
    Result<std::string> readToString();


    // write funcs

    /// Writes the given bytes to the file, returning the number of bytes successfully written.
    Result<size_t> write(std::span<const uint8_t> buf);
    /// Writes the given bytes to the file, returning the number of bytes successfully written.
    Result<size_t> write(std::string_view buf) {
        return write(std::span{reinterpret_cast<const uint8_t*>(buf.data()), buf.size()});
    }

    /// Writes all bytes to the file, returning an error if not all bytes could be written.
    Result<> writeAll(std::span<const uint8_t> buf);
    /// Writes all bytes to the file, returning an error if not all bytes could be written.
    Result<> writeAll(std::string_view buf) {
        return writeAll(std::span{reinterpret_cast<const uint8_t*>(buf.data()), buf.size()});
    }

    // seek / rewind / etc.

    /// Sets the file cursor to the given position, relative to the start of the file.
    Result<> seek(uint64_t pos);

    /// Returns the current seek position
    Result<uint64_t> tell();

    /// Returns the size of the file, in bytes
    Result<uint64_t> size();

private:
    File(NativeFd fd) : m_fd(fd) {}

    NativeFd m_fd = INVALID_FD;
};

}
