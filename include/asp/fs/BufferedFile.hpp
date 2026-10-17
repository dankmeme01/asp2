#pragma once
#include "File.hpp"

namespace asp::fs {

/// A wrapper around `File` that provides buffered reading and writing for files.
/// Small `write` and `read` calls go into internal buffers instead of issuing an IO syscall,
/// which helps with performance a lot.
class BufferedFile : public File {
public:
    BufferedFile(File file, size_t bufSize = FILE_BUFFER_DEFAULT_SIZE, bool readBuf = true, bool writeBuf = true);
    ~BufferedFile();

    BufferedFile(const BufferedFile&) = delete;
    BufferedFile& operator=(const BufferedFile&) = delete;
    BufferedFile(BufferedFile&& other) noexcept;
    BufferedFile& operator=(BufferedFile&& other) noexcept;

    File intoInner() && {
        (void) this->flush();
        return std::move(*this);
    }

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

    Result<> seek(uint64_t pos) = delete;
    Result<uint64_t> tell() = delete;

    /// Flushes all unwritten contents
    Result<> flush();

private:
    std::vector<uint8_t> m_rbuf;
    size_t m_rbufPos = 0;
    size_t m_rbufSize = 0;

    std::vector<uint8_t> m_wbuf;
    size_t m_wbufPos = 0;

    Result<> pullIfEmpty();
    void cleanup();
};

}
