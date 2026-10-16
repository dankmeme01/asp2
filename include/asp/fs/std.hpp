#pragma once
#include "Path.hpp"
#include <filesystem>

namespace asp::fs {

template <typename Derived>
auto PathInterface<Derived>::toStdPath(this auto&& self) {
    return [&] -> Result<std::filesystem::path> {
        GEODE_UNWRAP_INTO(auto p, self.native());
        return Ok(std::filesystem::path(p));
    }();
}

Path Path::fromStdPath(const auto& stdPath) {
#ifdef _WIN32
    return Path::fromWide(stdPath.native()).unwrapOrDefault();
#else
    return Path{stdPath.string()};
#endif
}

}
