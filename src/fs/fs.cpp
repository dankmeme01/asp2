#include <asp/fs/fs.hpp>
#include <cstring>
#include <fmt/format.h>
#include <asp/iter.hpp>

namespace asp::fs {

static auto endsWithOneOf(std::string_view str, std::string_view chars) {
    return !str.empty() && chars.contains(str.back());
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
            return Err(fmt::format("Path '{}' already exists but is not a directory", path));
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

Result<DirectoryIterator> iterDir(PathView path, bool recursive) {
    return DirectoryIterator::create(path, recursive);
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