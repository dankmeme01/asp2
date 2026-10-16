#include <asp/fs/File.hpp>
#include <asp/fs/BufferedFile.hpp>

namespace asp::fs {

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
            return Err("Unexpected EOF");
        }
        buf = buf.subspan(n);
    }
    return Ok();
}

Result<std::vector<uint8_t>> File::readToEnd() {
    std::vector<uint8_t> out;
    std::array<uint8_t, 4096> buf;

    while (true) {
        GEODE_UNWRAP_INTO(auto n, this->read(buf));
        if (n == 0) {
            break;
        }
        out.insert(out.end(), buf.begin(), buf.begin() + n);
    }

    return Ok(std::move(out));
}

Result<std::string> File::readToString() {
    std::string out;
    std::array<uint8_t, 4096> buf;

    while (true) {
        GEODE_UNWRAP_INTO(auto n, this->read(buf));
        if (n == 0) {
            break;
        }
        out.insert(out.end(), buf.begin(), buf.begin() + n);
    }

    return Ok(std::move(out));
}

Result<> File::writeAll(std::span<const uint8_t> buf) {
    while (!buf.empty()) {
        GEODE_UNWRAP_INTO(auto n, this->write(buf));
        if (n == 0) {
            return Err("failed to write any bytes");
        }
        buf = buf.subspan(n);
    }
    return Ok();
}

}
