#pragma once
#include <Geode/Result.hpp>

namespace asp {
    template <typename T = void, typename E = std::string>
    using Result = geode::Result<T, E>;

    using geode::Ok;
    using geode::Err;
}
