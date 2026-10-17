#pragma once
#include "Path.hpp"
#include <filesystem>

namespace asp::fs {

template <typename Derived>
auto PathInterface<Derived>::toStdPath(this auto&& self) {
    return [&] -> Result<std::filesystem::path> {
        return Ok(std::filesystem::path(self.native()));
    }();
}

Path Path::fromStdPath(const auto& stdPath) {
#ifdef _WIN32
    return Path::fromWide(stdPath.native());
#else
    return Path{stdPath.string()};
#endif
}

}
