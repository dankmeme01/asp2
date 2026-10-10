#include "path.hpp"
#include <filesystem>

namespace asp::fs {

template <typename Derived>
auto PathInterface<Derived>::toStdPath(this auto&& self) {
    GEODE_UNWRAP_INTO(auto p, self.native());
    return Ok(std::filesystem::path(p));
}

}
