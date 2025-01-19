#pragma once
#include <Geode/Result.hpp>

namespace asp {
    template <typename T, typename E>
    using Result = geode::Result<T, E>;

    using geode::Ok;
    using geode::Err;
}
