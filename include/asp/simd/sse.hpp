#pragma once

#include "base.hpp"

#ifdef ASP_IS_X86

#include <immintrin.h>

namespace asp::simd {
    float vec128sum(__m128 vec);
}

#endif
