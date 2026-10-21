#include <asp/fs/File.hpp>
#include <asp/fs/BufferedFile.hpp>

namespace asp::fs {

File::~File() {
    this->close();
}

File::File(File&& other) noexcept {
    m_fd = other.m_fd;
    other.m_fd = INVALID_FD;
}

File& File::operator=(File&& other) noexcept {
    if (this != &other) {
        this->close();
        m_fd = other.m_fd;
        other.m_fd = INVALID_FD;
    }
    return *this;
}

Result<BufferedFile> File::openBuffered(PathView path, const OpenOptions& options, size_t bufSize) {
    GEODE_UNWRAP_INTO(auto file, File::open(path, options));
    return Ok(BufferedFile(std::move(file), bufSize, options.read, options.write));
}

Result<BufferedFile> File::createBuffered(PathView path, size_t bufSize) {
    GEODE_UNWRAP_INTO(auto file, File::create(path));
    return Ok(BufferedFile(std::move(file), bufSize, false, true));
}

Result<> File::readExact(std::span<uint8_t> buf) {
    while (!buf.empty()) {
        GEODE_UNWRAP_INTO(auto n, this->read(buf));
        if (n == 0) {
            return Err(ErrorKind::UnexpectedEof);
        }
        buf = buf.subspan(n);
    }
    return Ok();
}

// reads data from the file into the container without using intermediate scratch buffers
static Result<> readNoIntermediate(auto& container, File& file, size_t size) {
    container.resize(size);

    size_t totalRead = 0;
    while (totalRead < size) {
        auto dest = std::span{(uint8_t*)container.data() + totalRead, size - totalRead};
        GEODE_UNWRAP_INTO(auto n, file.read(dest));

        if (n == 0) {
            return Err(ErrorKind::UnexpectedEof);
        }

        totalRead += n;
    }

    return Ok();
}

// reads data into an intermediate buffer and then copies it into the container,
// used when the size is uncertain
static Result<> readWithBuffer(auto& container, File& file) {
    // 256kb buffer
    std::vector<uint8_t> buffer(1024 * 256);

    while (true) {
        GEODE_UNWRAP_INTO(auto n, file.read(buffer));
        if (n == 0) {
            break;
        }

        container.insert(container.end(), buffer.begin(), buffer.begin() + n);
    }

    return Ok();
}

static Result<> commonRead(auto& container, File& file) {
    GEODE_UNWRAP_INTO(auto meta, file.metadata());

    // size may be inaccurate on special files in unix, so don't always rely on it
    std::optional<size_t> size =
        meta.isFile() && meta.size > 0 ? std::optional{meta.size} : std::nullopt;

    if (size) {
        // we know the size and we want to trust it
        return readNoIntermediate(container, file, *size);
    }

    return readWithBuffer(container, file);
}

Result<std::vector<uint8_t>> File::readToEnd() {
    std::vector<uint8_t> out;
    GEODE_UNWRAP(commonRead(out, *this));
    return Ok(std::move(out));
}

Result<std::string> File::readToString() {
    std::string out;
    GEODE_UNWRAP(commonRead(out, *this));
    return Ok(std::move(out));
}

Result<> File::writeAll(std::span<const uint8_t> buf) {
    while (!buf.empty()) {
        GEODE_UNWRAP_INTO(auto n, this->write(buf));
        if (n == 0) {
            return Err(ErrorKind::WriteZero);
        }
        buf = buf.subspan(n);
    }
    return Ok();
}

}
