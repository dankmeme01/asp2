#include <asp/fs/BufferedFile.hpp>

namespace asp::fs {

BufferedFile::BufferedFile(File file, size_t bufSize, bool readBuf, bool writeBuf) : File(std::move(file)) {
    if (readBuf) {
        m_rbuf.resize(bufSize);
    }
    if (writeBuf) {
        m_wbuf.resize(bufSize);
    }
}

BufferedFile::~BufferedFile() {
    this->cleanup();
}

void BufferedFile::cleanup() {
    (void) this->flush();
}

BufferedFile::BufferedFile(BufferedFile&& other) noexcept : File(std::move(other)) {
    m_rbuf = std::move(other.m_rbuf);
    m_rbufPos = std::exchange(other.m_rbufPos, 0);
    m_rbufSize = std::exchange(other.m_rbufSize, 0);
    m_wbuf = std::move(other.m_wbuf);
    m_wbufPos = std::exchange(other.m_wbufPos, 0);
}

BufferedFile& BufferedFile::operator=(BufferedFile&& other) noexcept {
    if (this != &other) {
        this->cleanup();

        File::operator=(std::move(other));
        m_rbuf = std::move(other.m_rbuf);
        m_rbufPos = std::exchange(other.m_rbufPos, 0);
        m_rbufSize = std::exchange(other.m_rbufSize, 0);
        m_wbuf = std::move(other.m_wbuf);
        m_wbufPos = std::exchange(other.m_wbufPos, 0);
    }
    return *this;
}

Result<size_t> BufferedFile::read(std::span<uint8_t> buf) {
    if (buf.empty()) {
        return Ok(0);
    }

    if (m_rbuf.empty()) {
        // bypass buffering
        return File::read(buf);
    }

    GEODE_UNWRAP(this->pullIfEmpty());

    // read into buf from m_buf
    size_t bytesToRead = std::min(buf.size(), m_rbufSize);
    std::memcpy(buf.data(), m_rbuf.data() + m_rbufPos, bytesToRead);
    m_rbufPos += bytesToRead;
    m_rbufSize -= bytesToRead;

    return Ok(bytesToRead);
}

Result<> BufferedFile::readExact(std::span<uint8_t> buf) {
    while (!buf.empty()) {
        GEODE_UNWRAP_INTO(auto n, this->read(buf));
        if (n == 0) {
            return Err("Unexpected EOF");
        }
        buf = buf.subspan(n);
    }
    return Ok();
}

Result<std::vector<uint8_t>> BufferedFile::readToEnd() {
    if (m_rbuf.empty()) {
        // bypass buffering
        return File::readToEnd();
    }

    std::vector<uint8_t> out;

    while (true) {
        GEODE_UNWRAP(this->pullIfEmpty());
        if (m_rbufSize == 0) {
            break;
        }

        out.insert(out.end(), m_rbuf.data() + m_rbufPos, m_rbuf.data() + m_rbufPos + m_rbufSize);
        m_rbufPos += m_rbufSize;
        m_rbufSize = 0;
    }

    return Ok(std::move(out));
}

Result<std::string> BufferedFile::readToString() {
    if (m_rbuf.empty()) {
        // bypass buffering
        return File::readToString();
    }

    std::string out;

    while (true) {
        GEODE_UNWRAP(this->pullIfEmpty());
        if (m_rbufSize == 0) {
            break;
        }

        out.insert(out.end(), m_rbuf.data() + m_rbufPos, m_rbuf.data() + m_rbufPos + m_rbufSize);
        m_rbufPos += m_rbufSize;
        m_rbufSize = 0;
    }

    return Ok(std::move(out));
}

Result<size_t> BufferedFile::write(std::span<const uint8_t> buf) {
    if (m_wbuf.empty()) {
        // bypass buffering
        return File::write(buf);
    }

    if (m_wbufPos == m_wbuf.size()) {
        GEODE_UNWRAP(this->flush());
    }

    auto dst = std::span{m_wbuf}.subspan(m_wbufPos, m_wbuf.size() - m_wbufPos);

    size_t written = std::min(dst.size(), buf.size());
    std::memcpy(dst.data(), buf.data(), written);

    m_wbufPos += written;
    return Ok(written);
}

Result<> BufferedFile::writeAll(std::span<const uint8_t> buf) {
    while (!buf.empty()) {
        GEODE_UNWRAP_INTO(auto n, this->write(buf));
        if (n == 0) {
            return Err("failed to write any bytes");
        }
        buf = buf.subspan(n);
    }
    return Ok();
}

Result<> BufferedFile::flush() {
    if (m_wbufPos > 0) {
        GEODE_UNWRAP(File::writeAll({m_wbuf.data(), m_wbufPos}));
        m_wbufPos = 0;
    }
    return Ok();
}

Result<> BufferedFile::pullIfEmpty() {
    if (m_rbufSize == 0) {
        GEODE_UNWRAP_INTO(auto bytesRead, File::read(m_rbuf));
        m_rbufSize = bytesRead;
        m_rbufPos = 0;
    }
    return Ok();
}

}
